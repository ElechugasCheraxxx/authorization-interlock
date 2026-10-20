#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

MODE="${1:-install}"

# FIX: Resolve Python 3.12 explicitly — system python3 may be 3.14+
# pydantic-core uses PyO3 which requires exactly Python 3.12 due to ABI incompatibility across minor versions
PYTHON312=""
for candidate in \
    /opt/homebrew/opt/python@3.12/bin/python3.12 \
    /usr/local/opt/python@3.12/bin/python3.12 \
    /usr/bin/python3.12 \
    python3.12; do
    if command -v "$candidate" &>/dev/null 2>&1; then
        PYTHON312="$candidate"
        break
    fi
done

require_cmd() {
    if ! command -v "$1" &>/dev/null; then
        error "Required tool not found: $1. $2"
        exit 1
    fi
}

check_python() {
    if [[ -z "$PYTHON312" ]]; then
        error "Python 3.12 not found. Run: brew install python@3.12"
        exit 1
    fi
    PY_VER=$("$PYTHON312" -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
    info "Python $PY_VER ✓ ($PYTHON312)"
}

check_node() {
    require_cmd node "Install Node.js 20+ from https://nodejs.org"
    NODE_VER=$(node --version)
    info "Node $NODE_VER ✓"
}

check_hardware() {
    info "Checking hardware requirements..."
    OS="$(uname -s)"
    if [[ "$OS" == "Darwin" ]]; then
        ARCH="$(uname -m)"
        if [[ "$ARCH" == "arm64" ]]; then
            info "Apple Silicon (arm64) detected — MLX backend available ✓"
            if command -v mlx_lm.server &>/dev/null; then
                info "mlx-lm ✓"
            else
                warn "mlx-lm not found. Run: pip install mlx-lm"
                warn "[REQUIRES_RUNTIME_VALIDATION: mlx-lm installation]"
            fi
        else
            warn "Intel Mac detected — MLX not available. Use llama.cpp Metal."
        fi
    elif [[ "$OS" == "Linux" ]]; then
        if command -v nvidia-smi &>/dev/null; then
            GPU_INFO=$(nvidia-smi --query-gpu=name,memory.total --format=csv,noheader 2>/dev/null || echo "N/A")
            info "NVIDIA GPU detected: $GPU_INFO ✓"
            warn "[REQUIRES_RUNTIME_VALIDATION: llama.cpp CUDA build]"
        else
            warn "No NVIDIA GPU detected. CPU-only inference will be slow."
        fi
    fi
}

install_backend() {
    info "Installing backend dependencies..."
    cd "$SCRIPT_DIR/backend"
    if [[ ! -d ".venv" ]]; then
        "$PYTHON312" -m venv .venv   # FIX: use Python 3.12 explicitly
        info "Virtual environment created (Python 3.12)"
    fi
    .venv/bin/pip install --upgrade pip -q
    .venv/bin/pip install -r requirements.txt -q
    info "Backend dependencies installed ✓"
    cd "$SCRIPT_DIR"
}

install_frontend_windows() {
    if [[ ! -d "$SCRIPT_DIR/frontend-windows" ]]; then
        warn "frontend-windows not found — skipping"
        return
    fi
    info "Installing Windows frontend dependencies..."
    cd "$SCRIPT_DIR/frontend-windows"
    npm install --silent
    info "Windows frontend dependencies installed ✓"
    cd "$SCRIPT_DIR"
}

create_data_dirs() {
    info "Creating data directories..."
    mkdir -p data/{sessions,evidence,models}
    if [[ ! -f .gitignore ]]; then
        echo "data/" > .gitignore
        echo "backend/.venv/" >> .gitignore
        echo "frontend-windows/node_modules/" >> .gitignore
        echo "*.gguf" >> .gitignore
        echo "*.mlx" >> .gitignore
        info ".gitignore created ✓"
    fi
    info "Data directories ready ✓"
}

run_tests() {
    info "Running backend tests..."
    cd "$SCRIPT_DIR/backend"
    .venv/bin/python3 -m pytest tests/ -v --tb=short 2>&1 | tail -30
    cd "$SCRIPT_DIR"
}

start_backend() {
    info "Starting backend server..."
    cd "$SCRIPT_DIR/backend"
    source .venv/bin/activate 2>/dev/null || { error "Run ./install.sh install first"; exit 1; }
    exec .venv/bin/python3 -m uvicorn main:app --host 127.0.0.1 --port 8765 --reload
}

upgrade_backend() {
    info "Upgrading backend dependencies..."
    cd "$SCRIPT_DIR/backend"
    .venv/bin/pip install --upgrade -r requirements.txt -q
    info "Backend upgraded ✓"
    cd "$SCRIPT_DIR"
}

deploy_migrations() {
    local input_xml="${1:-$SCRIPT_DIR/new-migrations.xml}"
    local enhanced_xml="${2:-$SCRIPT_DIR/new-migrations-enhanced.xml}"
    local runtime_json="${3:-$SCRIPT_DIR/config/new-migrations.runtime.json}"
    local deploy_script="$SCRIPT_DIR/tools/polyglot_migration_deploy.py"

    check_python
    if [[ ! -f "$deploy_script" ]]; then
        error "Deployment script not found: $deploy_script"
        exit 1
    fi
    if [[ ! -f "$input_xml" ]]; then
        error "Input migration XML not found: $input_xml"
        exit 1
    fi

    info "Deploying migration plan for production runtime..."
    "$PYTHON312" "$deploy_script" deploy \
        --input "$input_xml" \
        --enhanced-output "$enhanced_xml" \
        --runtime-output "$runtime_json"
    info "Migration deployment artifact ready ✓"
    info "Enhanced XML: $enhanced_xml"
    info "Runtime JSON: $runtime_json"
}

case "$MODE" in
    install)
        info "=== Assessment IDE Install ==="
        check_python
        check_node
        check_hardware
        create_data_dirs
        install_backend
        install_frontend_windows
        info "=== Installation complete ==="
        info "Start backend:   ./install.sh start"
        info "Run tests:       ./install.sh test"
        ;;
    start)
        start_backend
        ;;
    test)
        run_tests
        ;;
    upgrade)
        check_python
        upgrade_backend
        info "Run ./install.sh start to restart"
        ;;
    deploy-migrations)
        deploy_migrations "${2:-}" "${3:-}" "${4:-}"
        ;;
    *)
        echo "Usage: $0 [install|start|test|upgrade|deploy-migrations]"
        exit 1
        ;;
esac