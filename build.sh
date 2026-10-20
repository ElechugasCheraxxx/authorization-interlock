#!/bin/zsh

##############################################################################
# Build Script for Interlock Assessment IDE (Polyglot Architecture)
# 
# Este script automatiza la compilación de todos los módulos políglotas
# en el orden correcto, con manejo de dependencias y verificación de errores.
##############################################################################

set -e  # Exit on error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'  # No Color

# Configuración
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
INSTALL_DIR="${PROJECT_ROOT}/dist"
LOG_DIR="${BUILD_DIR}/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BUILD_LOG="${LOG_DIR}/build_${TIMESTAMP}.log"

# Flags de compilación
BUILD_TYPE="${BUILD_TYPE:-Release}"
ENABLE_PRODUCTION="${ENABLE_PRODUCTION:-ON}"
ENABLE_OLLVM="${ENABLE_OLLVM:-ON}"
ENABLE_ABB="${ENABLE_ABB:-ON}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(sysctl -n hw.ncpu)}"

##############################################################################
# FUNCIONES AUXILIARES
##############################################################################

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$BUILD_LOG"
}

log_success() {
    echo -e "${GREEN}[OK]${NC} $1" | tee -a "$BUILD_LOG"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1" | tee -a "$BUILD_LOG"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$BUILD_LOG"
}

check_prerequisite() {
    local cmd=$1
    local name=$2
    
    if ! command -v "$cmd" &> /dev/null; then
        log_error "Prerequisito faltante: $name (comando: $cmd)"
        return 1
    fi
    log_success "Encontrado: $name"
    return 0
}

##############################################################################
# VERIFICACIÓN DE PREREQUISITOS
##############################################################################

verify_prerequisites() {
    log_info "Verificando requisitos previos..."
    
    local all_ok=true
    
    # Herramientas esenciales
    check_prerequisite "cmake" "CMake (>= 3.24)" || all_ok=false
    check_prerequisite "clang++" "Clang++" || all_ok=false
    check_prerequisite "swift" "Swift Compiler" || all_ok=false
    check_prerequisite "cargo" "Cargo (Rust)" || all_ok=false
    check_prerequisite "go" "Go Compiler" || all_ok=false
    check_prerequisite "zig" "Zig Compiler" || all_ok=false
    check_prerequisite "perl" "Perl" || all_ok=false

    if [ "$all_ok" = false ]; then
        log_error "Algunos requisitos previos no están instalados"
        return 1
    fi
    
    log_success "Todos los requisitos previos verificados"
    return 0
}

##############################################################################
# CREACIÓN DE DIRECTORIOS
##############################################################################

setup_directories() {
    mkdir -p "$BUILD_DIR"
    mkdir -p "$LOG_DIR"
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$BUILD_DIR/bin"
    mkdir -p "$BUILD_DIR/lib"
    
    log_info "Configurando directorios de compilación..."
    log_success "Directorios creados"
}

##############################################################################
# COMPILACIÓN DE MÓDULOS RUST
##############################################################################

build_rust_modules() {
    log_info "Compilando módulos Rust (Cipher, PQC)..."
    
    cd "$PROJECT_ROOT/backend/cipher-rs"
    
    if [ ! -f "Cargo.toml" ]; then
        log_warning "Cargo.toml no encontrado en cipher-rs, saltando..."
        return 0
    fi
    
    RUSTFLAGS="-C opt-level=3" cargo build --release 2>&1 | tee -a "$BUILD_LOG"
    
    # Copiar librerías compiladas
    cp target/release/libcipher_rs.a "$BUILD_DIR/lib/" 2>/dev/null || true
    
    log_success "Módulos Rust compilados"
}

##############################################################################
# COMPILACIÓN DE MÓDULOS GO
##############################################################################

build_go_modules() {
    log_info "Compilando módulos Go (Atlas Orchestrator)..."
    
    cd "$PROJECT_ROOT/backend/atlas-go"
    
    if [ ! -f "go.mod" ]; then
        log_warning "go.mod no encontrado, inicializando..."
        go mod init github.com/interlock/atlas-go 2>&1 | tee -a "$BUILD_LOG"
    fi
    
    CGO_ENABLED=0 GOOS=darwin GOARCH=arm64 \
        go build -ldflags="-s -w" -o "$BUILD_DIR/bin/atlas_orchestrator" ./main.go 2>&1 | tee -a "$BUILD_LOG"
    
    log_success "Atlas Orchestrator compilado"
}

##############################################################################
# COMPILACIÓN DE MÓDULOS ZIG
##############################################################################

build_zig_modules() {
    log_info "Compilando módulos Zig (Artifact Orchestrator)..."
    
    cd "$PROJECT_ROOT/backend/transfer-zig"
    
    if [ ! -f "build.zig" ]; then
        log_warning "build.zig no encontrado, saltando..."
        return 0
    fi
    
    zig build -Doptimize=ReleaseFast -Dtarget=aarch64-macos 2>&1 | tee -a "$BUILD_LOG"
    
    # Copiar binarios compilados
    cp zig-out/bin/* "$BUILD_DIR/bin/" 2>/dev/null || true
    
    log_success "Artifact Orchestrator compilado"
}


##############################################################################
# COMPILACIÓN DEL CORE C++
##############################################################################

build_cpp_core() {
    log_info "Compilando Core C++ y Middleware Swift (Inferencia RDT, GPU)..."
    
    # Crear directorio de build
    mkdir -p "$BUILD_DIR/cmake"
    cd "$BUILD_DIR/cmake"
    
    # Configurar CMake con generador Unix Makefiles (estándar)
    cmake -G "Unix Makefiles" \
          -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DENABLE_OLLVM=$ENABLE_OLLVM \
          -DENABLE_ABB=$ENABLE_ABB \
          -DBUILD_CPP_CORE=ON \
          -DBUILD_RUST_CIPHER=ON \
          -DBUILD_GO_ATLAS=ON \
          -DBUILD_ZIG_PAYLOADS=ON \
          "$PROJECT_ROOT" 2>&1 | tee -a "$BUILD_LOG"
    
    # Compilar Core usando CMake
    cmake --build . --target interlock_core --config $BUILD_TYPE -j "$PARALLEL_JOBS" 2>&1 | tee -a "$BUILD_LOG"

    log_success "Core y Middleware compilados"
}

build_swift_modules() {
    log_info "Saltando compilación manual de Swift (Manejado por CMake)..."
    return 0
}

##############################################################################
# COMPILACIÓN DE TESTS
##############################################################################

build_tests() {
    log_info "Compilando tests..."
    
    cd "$BUILD_DIR/cmake"
    cmake --build . --target core_tests --config $BUILD_TYPE -j "$PARALLEL_JOBS" 2>&1 | tee -a "$BUILD_LOG" || true
    
    log_success "Tests compilados"
}

##############################################################################
# INSTALACIÓN
##############################################################################

install_artifacts() {
    log_info "Instalando artefactos en $INSTALL_DIR..."
    
    cd "$BUILD_DIR/cmake"
    cmake --install . --config $BUILD_TYPE --prefix "$INSTALL_DIR" 2>&1 | tee -a "$BUILD_LOG" || true
    
    # Copiar binarios compilados adicionales (Go, Zig, etc.)
    mkdir -p "$INSTALL_DIR/bin"
    cp "$BUILD_DIR/bin"/* "$INSTALL_DIR/bin/" 2>/dev/null || true
    
    mkdir -p "$INSTALL_DIR/lib"
    cp "$BUILD_DIR/lib"/* "$INSTALL_DIR/lib/" 2>/dev/null || true
    
    # Copiar configuración
    mkdir -p "$INSTALL_DIR/etc"
    cp "$PROJECT_ROOT/backend/config"/*.toml "$INSTALL_DIR/etc/" 2>/dev/null || true
    
    log_success "Artefactos instalados"
}

##############################################################################
# VERIFICACIÓN POST-BUILD
##############################################################################

verify_build() {
    log_info "Verificando integridad de la compilación..."
    
    local errors=0
    
    # Verificar framework principal
    local core_framework="$BUILD_DIR/cmake/interlock_core.framework/interlock_core"
    if [ ! -f "$core_framework" ]; then
        log_error "Framework interlock_core no encontrado en $core_framework"
        ((errors++))
    fi
    
    if [ ! -f "$BUILD_DIR/bin/atlas_orchestrator" ]; then
        log_warning "atlas_orchestrator no compilado (Go module)"
    fi
    
    if [ ! -d "$INSTALL_DIR" ]; then
        log_error "Directorio de instalación no existe"
        ((errors++))
    fi
    
    if [ $errors -eq 0 ]; then
        log_success "Verificación de build completada exitosamente"
        return 0
    else
        log_error "Build verification falló con $errors errores"
        return 1
    fi
}

##############################################################################
# LIMPIAR BUILD
##############################################################################

clean_build() {
    log_info "Limpiando directorio de build..."
    rm -rf "$BUILD_DIR"
    log_success "Build limpiado"
}

##############################################################################
# RESUMEN FINAL
##############################################################################

print_summary() {
    echo ""
    echo "=========================================="
    echo -e "${GREEN}BUILD COMPLETADO EXITOSAMENTE${NC}"
    echo "=========================================="
    echo "Versión: Interlock v1.0.0-production"
    echo "Build Type: $BUILD_TYPE"
    echo "Parallelismo: $PARALLEL_JOBS threads"
    echo ""
    echo "Artefactos:"
    echo "  Binarios: $INSTALL_DIR/bin"
    echo "  Librerías: $INSTALL_DIR/lib"
    echo "  Configuración: $INSTALL_DIR/etc"
    echo ""
    echo "Logs: $BUILD_LOG"
    echo "=========================================="
    echo ""
}

##############################################################################
# FIRMA Y EMPAQUETADO (Hardened Runtime)
##############################################################################

sign_and_package() {
    log_info "Firmando binarios con Hardened Runtime..."

    local ENTITLEMENTS="${PROJECT_ROOT}/entitlements.plist"

    if [ ! -f "$ENTITLEMENTS" ]; then
        log_warning "entitlements.plist no encontrado, saltando firma..."
        return 0
    fi

    # Firmar el ejecutable principal y librerías dinámicas
    find "$INSTALL_DIR/bin" -type f -exec codesign --force --options runtime --entitlements "$ENTITLEMENTS" --sign - {} \;
    find "$INSTALL_DIR/lib" -name "*.dylib" -exec codesign --force --options runtime --entitlements "$ENTITLEMENTS" --sign - {} \;

    log_success "Binarios firmados correctamente"
}

##############################################################################
# COMPILACIÓN DEL FRONTEND Y EMPAQUETADO FINAL
##############################################################################

build_frontend_and_bundle() {
    log_info "Compilando Frontend Qt6 y generando Bundle..."

    cd "$BUILD_DIR/cmake"

    # Compilar el target del App Bundle
    cmake --build . --target InterlockIDE_Qt --config $BUILD_TYPE -j "$PARALLEL_JOBS" 2>&1 | tee -a "$BUILD_LOG"

    log_success "Frontend y Bundle generados"
}

move_to_final_product() {
    log_info "Moviendo el producto final a 'Final Product'..."

    local BUNDLE_PATH="$BUILD_DIR/cmake/InterlockIDE_Qt.app"
    local DEST_DIR="$PROJECT_ROOT/Final Product"

    if [ -d "$BUNDLE_PATH" ]; then
        rm -rf "$DEST_DIR/Interlock Assessment IDE.app"
        cp -R "$BUNDLE_PATH" "$DEST_DIR/Interlock Assessment IDE.app"
        log_success "App bundle copiada a: $DEST_DIR/Interlock Assessment IDE.app"
    else
        log_error "No se encontró el bundle en $BUNDLE_PATH"
        return 1
    fi
}

##############################################################################
# MAIN
##############################################################################

main() {
    # Preparar directorios antes de loguear
    setup_directories

    log_info "=========================================="
    log_info "Interlock Assessment IDE - Build System v1.0"
    log_info "=========================================="
    verify_prerequisites || exit 1
    
    build_rust_modules
    build_go_modules
    build_zig_modules
    build_cpp_core
    build_frontend_and_bundle
    build_swift_modules
    build_tests
    install_artifacts
    sign_and_package
    move_to_final_product
    verify_build || exit 1
    
    print_summary
    
    log_success "Build completado exitosamente y exportado a 'Final Product'"
    exit 0
}

# Ejecutar main
main "$@"
