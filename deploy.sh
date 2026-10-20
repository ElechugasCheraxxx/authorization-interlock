#!/bin/zsh

##############################################################################
# Production Deployment Script for Interlock Assessment IDE
# 
# Este script configura el entorno de producción, inicia servicios,
# y verifica la salud de todos los componentes.
##############################################################################

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Variables
DEPLOY_ROOT="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="${DEPLOY_ROOT}/dist"
LOG_DIR="${DEPLOY_ROOT}/logs"
PID_DIR="${DEPLOY_ROOT}/pids"
CONFIG_DIR="${DEPLOY_ROOT}/backend/config"
ABB_DIR="${DEPLOY_ROOT}/build/abb"

echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Interlock Assessment IDE - Production Deployment v1.0   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"

##############################################################################
# FUNCIONES AUXILIARES
##############################################################################

log_info() {
    echo -e "${BLUE}[DEPLOY]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

##############################################################################
# VERIFICACIÓN PRE-DEPLOYMENT
##############################################################################

check_prerequisites() {
    log_info "Verificando prerequisitos de deployment..."
    
    # Verificar que el build se completó
    if [ ! -d "$INSTALL_DIR" ]; then
        log_error "Directorio de instalación no encontrado: $INSTALL_DIR"
        log_info "Ejecutar: ./build.sh"
        exit 1
    fi
    
    # Verificar ejecutables principales
    for exe in interlock_core atlas_orchestrator; do
        if [ ! -f "$INSTALL_DIR/bin/$exe" ]; then
            log_warning "Ejecutable no encontrado: $exe"
        fi
    done
    
    log_success "Prerequisitos verificados"
}

##############################################################################
# CONFIGURACIÓN DEL ENTORNO
##############################################################################

setup_environment() {
    log_info "Configurando entorno de producción..."
    
    # Crear directorios necesarios (Locales para evitar problemas de permisos en macOS)
    mkdir -p "$LOG_DIR"
    mkdir -p "$PID_DIR"
    mkdir -p "${DEPLOY_ROOT}/data/cache"
    mkdir -p "${DEPLOY_ROOT}/data/evidence"
    mkdir -p "$ABB_DIR"
    
    # Configurar permisos restrictivos
    chmod 700 "$LOG_DIR"
    chmod 700 "$PID_DIR"

    # Cargar variables de entorno
    export INTERLOCK_HOME="$DEPLOY_ROOT"
    export INTERLOCK_CONFIG="$CONFIG_DIR/production.toml"
    export INTERLOCK_LOG_DIR="$LOG_DIR"
    export INTERLOCK_PID_DIR="$PID_DIR"
    export INTERLOCK_ABB_PATH="$ABB_DIR/interlock_abb_prod"

    # Security: No mostrar paths en error
    export INTERLOCK_PRODUCTION=1
    
    log_success "Entorno configurado"
}

##############################################################################
# INICIALIZACIÓN DE SERVICIOS
##############################################################################

start_core() {
    log_info "Iniciando Core C++ (Inferencia RDT)..."
    
    "$INSTALL_DIR/bin/interlock_core" \
        --config "$CONFIG_DIR/production.toml" \
        --log-dir "$LOG_DIR" \
        --daemon \
        > "$LOG_DIR/core.log" 2>&1 &
    
    local core_pid=$!
    echo $core_pid > "$PID_DIR/interlock_core.pid"
    
    sleep 2
    
    if kill -0 $core_pid 2>/dev/null; then
        log_success "Core iniciado (PID: $core_pid)"
    else
        log_error "Core falló al iniciar"
        cat "$LOG_DIR/core.log" | tail -20
        exit 1
    fi
}

start_atlas() {
    log_info "Iniciando Atlas Orchestrator (Go)..."
    
    "$INSTALL_DIR/bin/atlas_orchestrator" \
        --config "$CONFIG_DIR/production.toml" \
        --log-dir "$LOG_DIR" \
        > "$LOG_DIR/atlas.log" 2>&1 &
    
    local atlas_pid=$!
    echo $atlas_pid > "$PID_DIR/atlas_orchestrator.pid"
    
    sleep 2
    
    if kill -0 $atlas_pid 2>/dev/null; then
        log_success "Atlas iniciado (PID: $atlas_pid)"
    else
        log_warning "Atlas falló al iniciar (puede reintentarse)"
    fi
}

start_services() {
    log_info "Iniciando servicios de infraestructura..."
    
    start_core
    start_atlas
    
    # Esperar a que todos los servicios estén listos
    sleep 3
    
    log_success "Todos los servicios iniciados"
}

##############################################################################
# HEALTH CHECKS
##############################################################################

health_check() {
    log_info "Ejecutando verificaciones de salud..."
    
    local all_healthy=true
    
    # Verificar Core
    if [ -f "$PID_DIR/interlock_core.pid" ]; then
        local core_pid=$(cat "$PID_DIR/interlock_core.pid")
        if kill -0 $core_pid 2>/dev/null; then
            log_success "Core está activo (PID: $core_pid)"
        else
            log_error "Core está muerto"
            all_healthy=false
        fi
    fi
    
    # Verificar Atlas
    if [ -f "$PID_DIR/atlas_orchestrator.pid" ]; then
        local atlas_pid=$(cat "$PID_DIR/atlas_orchestrator.pid")
        if kill -0 $atlas_pid 2>/dev/null; then
            log_success "Atlas está activo (PID: $atlas_pid)"
        else
            log_warning "Atlas no está activo"
        fi
    fi
    
    # Verificar ABB
    if [ -f "$ABB_DIR/interlock_abb_prod" ]; then
        log_success "ABB (Breadcrumb Buffer) disponible"
    else
        log_warning "ABB no inicializado"
    fi
    
    if [ "$all_healthy" = true ]; then
        log_success "Health check completado - todos los servicios OK"
        return 0
    else
        log_error "Health check falló - algunos servicios no están activos"
        return 1
    fi
}

##############################################################################
# ESTADÍSTICAS DE DEPLOYMENT
##############################################################################

print_deployment_info() {
    echo ""
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  DEPLOYMENT EXITOSO${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    echo ""
    echo "Versión: Interlock v1.0.0-production"
    echo "Fecha: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
    echo "Hostname: $(hostname)"
    echo ""
    echo "Servicios activos:"
    ls -1 "$PID_DIR"/*.pid 2>/dev/null | xargs -I {} sh -c 'basename {} .pid | sed "s/^/  - /"'
    echo ""
    echo "Directorios importantes:"
    echo "  Instalación: $INSTALL_DIR"
    echo "  Configuración: $CONFIG_DIR"
    echo "  Logs: $LOG_DIR"
    echo "  PIDs: $PID_DIR"
    echo ""
    echo "Próximos pasos:"
    echo "  1. Monitorear logs: tail -f $LOG_DIR/*.log"
    echo "  2. Verificar IPC: ls -la /tmp/interlock_ipc_*"
    echo "  3. Ver ABB: hexdump -C $ABB_DIR/interlock_abb_prod | head -20"
    echo ""
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    echo ""
}

##############################################################################
# STOP/CLEANUP
##############################################################################

stop_services() {
    log_info "Deteniendo servicios..."
    
    for pid_file in "$PID_DIR"/*.pid; do
        if [ -f "$pid_file" ]; then
            local pid=$(cat "$pid_file")
            local name=$(basename "$pid_file" .pid)
            
            if kill -0 $pid 2>/dev/null; then
                log_info "Deteniendo $name..."
                kill -TERM $pid
                
                # Esperar graceful shutdown
                sleep 2
                
                # Si aún está vivo, forzar kill
                if kill -0 $pid 2>/dev/null; then
                    kill -9 $pid
                fi
            fi
        fi
    done
    
    # Limpiar PIDs
    rm -f "$PID_DIR"/*.pid
    
    log_success "Servicios detenidos"
}

##############################################################################
# MAIN
##############################################################################

main() {
    case "${1:-start}" in
        start)
            check_prerequisites
            setup_environment
            start_services
            sleep 2
            health_check
            print_deployment_info
            ;;
        stop)
            stop_services
            ;;
        restart)
            stop_services
            sleep 2
            "$0" start
            ;;
        status)
            health_check
            ;;
        *)
            echo "Uso: $0 {start|stop|restart|status}"
            exit 1
            ;;
    esac
}

main "$@"
