#!/bin/zsh

##############################################################################
# Interlock Production Verification Suite
# Verifica la integridad completa de la instalación productiva
##############################################################################

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0

##############################################################################
# FUNCIONES
##############################################################################

check() {
    local name=$1
    local cmd=$2
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    echo -n "  ⏳ $name... "
    
    if eval "$cmd" > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    else
        echo -e "${RED}✗${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
    fi
}

check_file() {
    local name=$1
    local path=$2
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    echo -n "  ⏳ $name... "
    
    if [ -f "$path" ]; then
        echo -e "${GREEN}✓${NC} ($path)"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    else
        echo -e "${RED}✗${NC} (faltante: $path)"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
    fi
}

check_dir() {
    local name=$1
    local path=$2
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    echo -n "  ⏳ $name... "
    
    if [ -d "$path" ]; then
        echo -e "${GREEN}✓${NC} ($path)"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    else
        echo -e "${RED}✗${NC} (faltante: $path)"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
    fi
}

print_section() {
    echo ""
    echo -e "${BLUE}▶ $1${NC}"
}

##############################################################################
# VERIFICACIONES
##############################################################################

verify_build_artifacts() {
    print_section "Build Artifacts"
    
    local base_dir="/Users/Cheraxxx/Interlock Assessment IDE"
    
    check_dir "Build directory" "$base_dir/build"
    check_file "Main CMakeLists.txt" "$base_dir/CMakeLists.txt"
    check_file "Build script" "$base_dir/build.sh"
    check_file "Deploy script" "$base_dir/deploy.sh"
}

verify_configuration() {
    print_section "Configuration Files"
    
    local config_dir="/Users/Cheraxxx/Interlock IDE/backend/config"
    
    check_file "Production config" "$config_dir/production.toml"
    check_file "IDE config" "$config_dir/ide.toml"
    check_file "Agents config" "$config_dir/agents.toml"
}

verify_source_modules() {
    print_section "Source Modules"
    
    local backend="/Users/Cheraxxx/Interlock IDE/backend"
    
    check_dir "C++ Core" "$backend/core-cpp"
    check_dir "Rust Cipher" "$backend/cipher-rs"
    check_dir "Go Atlas" "$backend/atlas-go"
    check_dir "Zig Payloads" "$backend/transfer-zig"
    check_dir "C Shield" "$backend/shield-c"
    check_dir "ASM Stubs" "$backend/stubs-asm"
    check_dir "Headers" "$backend/include"
    check_dir "Formal Proofs" "$backend/proofs"
}

verify_infrastructure() {
    print_section "Infrastructure Components"
    
    local include_dir="/Users/Cheraxxx/Interlock IDE/backend/include"
    
    check_file "ABB Header" "$include_dir/AutonomousBreadcrumbBuffer.hpp"
    check_file "IPC Header" "$include_dir/IPCBridge.hpp"
    
    local core_dir="/Users/Cheraxxx/Interlock IDE/backend/core-cpp"
    
    check_file "ABB Implementation" "$core_dir/AutonomousBreadcrumbBuffer.cpp"
    check_file "IPC Implementation" "$core_dir/IPCBridge.cpp"
}

verify_tools() {
    print_section "Tools & Scripts"
    
    local tools_dir="/Users/Cheraxxx/Interlock IDE/backend/tools"
    
    check_file "ABB Inspector" "$tools_dir/abb_inspector.py"
    check "ABB Inspector executable" "[ -x \"$tools_dir/abb_inspector.py\" ]"
    check_file "Migration Tool" "$tools_dir/polyglot_migration_deploy.py"
}

verify_system_requirements() {
    print_section "System Requirements"
    
    check "CMake" "command -v cmake"
    check "Clang++" "command -v clang++"
    check "Swift Compiler" "command -v swift"
    check "Cargo (Rust)" "command -v cargo"
    check "Go Compiler" "command -v go"
    check "Zig Compiler" "command -v zig"
    check "Perl" "command -v perl"
    check "Python3" "command -v python3"
    check "Zsh" "command -v zsh"
}

verify_hardware() {
    print_section "Hardware Configuration"
    
    echo -n "  ℹ macOS Version: "
    sw_vers -productVersion
    
    echo -n "  ℹ Architecture: "
    uname -m
    
    echo -n "  ℹ CPU Cores: "
    sysctl -n hw.ncpu
    
    echo -n "  ℹ Memory: "
    vm_stat | grep "Pages free" | awk '{print ($3 * 4096 / 1024 / 1024 / 1024) "GB"}'
}

##############################################################################
# MAIN
##############################################################################

main() {
    # clear # Eliminamos clear para evitar errores en entornos sin TTY
    
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║     Interlock Assessment IDE - Production Verification         ║"
    echo "║            Build System v1.0 (Polyglot)                   ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    
    # Ejecutar verificaciones
    verify_build_artifacts
    verify_configuration
    verify_source_modules
    verify_infrastructure
    verify_tools
    verify_system_requirements
    verify_hardware
    
    # Resumen
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    echo -e "Resumen de Verificación:"
    echo -e "  Total de verificaciones: ${TOTAL_CHECKS}"
    echo -e "  ${GREEN}Aprobadas: ${PASSED_CHECKS}${NC}"
    
    if [ $FAILED_CHECKS -gt 0 ]; then
        echo -e "  ${RED}Fallidas: ${FAILED_CHECKS}${NC}"
    else
        echo -e "  ${RED}Fallidas: 0${NC}"
    fi
    
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    
    if [ $FAILED_CHECKS -eq 0 ]; then
        echo -e "\n${GREEN}✓ Sistema listo para compilación y deployment${NC}\n"
        echo "Próximos pasos:"
        echo "  1. cd /Users/Cheraxxx/Interlock\\ Assessment\\ IDE"
        echo "  2. ./build.sh"
        echo "  3. ./deploy.sh start"
        echo ""
        return 0
    else
        echo -e "\n${RED}✗ Se encontraron problemas - revisar arriba${NC}\n"
        return 1
    fi
}

main "$@"
