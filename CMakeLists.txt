cmake_minimum_required(VERSION 3.24)
project(InterlockIDE LANGUAGES CXX C OBJC OBJCXX ASM)

# ============================================================================
# CONFIGURACIÓN GLOBAL DE PRODUCCIÓN
# ============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(INTERLOCK_VERSION "1.0.0-production")
set(INTERLOCK_BUILD_TYPE "Release")

# Soporte para Apple Silicon (M4 Pro)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    add_compile_options(-O3)
    # Habilitar RPATH para portabilidad del Bundle
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
endif()

# ============================================================================
# CONFIGURACIÓN DE MÓDULOS POLÍGLOTAS
# ============================================================================
option(BUILD_CPP_CORE "Compilar motor C++ (RDT/GPU)" ON)
option(BUILD_SWIFT_MIDDLEWARE "Compilar middleware Swift" OFF)
option(BUILD_RUST_CIPHER "Compilar módulo Cipher en Rust" ON)
option(BUILD_GO_ATLAS "Compilar orquestación Go (Atlas)" ON)
option(BUILD_ZIG_PAYLOADS "Compilar orquestador Zig" ON)
option(BUILD_CSHARP_AGENTS "Compilar agentes C#/.NET" OFF)
option(BUILD_FORMAL_PROOFS "Compilar verificación Coq/Isabelle" OFF)
option(ENABLE_OLLVM "Activar ofuscación OLLVM" ON)
option(ENABLE_ABB "Activar Autonomous Breadcrumb Buffer" ON)
option(ENABLE_DTRACE_PROBES "Activar DTrace Boundary Probes" ON)

# ============================================================================
# DETECCIÓN DE HERRAMIENTAS
# ============================================================================
find_program(CARGO cargo REQUIRED)
find_program(GO go REQUIRED)
find_program(ZIG zig REQUIRED)
find_program(SWIFT swift REQUIRED)
find_program(PERL perl REQUIRED)

# Verificar Coq/Isabelle si BUILD_FORMAL_PROOFS está activo
if(BUILD_FORMAL_PROOFS)
    find_program(COQC coqc)
    if(NOT COQC)
        message(WARNING "coqc no encontrado. Skipping formal proofs.")
        set(BUILD_FORMAL_PROOFS OFF)
    endif()
endif()

# ============================================================================
# DIRECTORIOS DE INCLUSIÓN GLOBAL
# ============================================================================
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/backend/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/backend/core-cpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/backend/bridge")

# ============================================================================
# SUBDIRECTORIOS PRINCIPALES
# ============================================================================
add_subdirectory(backend/core-cpp)

if(BUILD_SWIFT_MIDDLEWARE)
    add_subdirectory(backend/middleware-swift)
endif()

# ============================================================================
# MÓDULOS EXTERNOS (Compilación cruzada)
# ============================================================================

# --- RUST (cipher-rs) ---
if(BUILD_RUST_CIPHER)
    add_custom_target(cipher_rust
        COMMAND ${CARGO} build --release
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/backend/cipher-rs"
        COMMENT "Compilando Cipher Module (Rust)..."
    )
    add_dependencies(interlock_core cipher_rust)
endif()

# --- GO (atlas-go) ---
if(BUILD_GO_ATLAS)
    add_custom_target(atlas_go
        COMMAND ${GO} build -o "${CMAKE_CURRENT_BINARY_DIR}/bin/atlas_orchestrator" ./main.go
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/backend/atlas-go"
        COMMENT "Compilando Atlas Swarm Orchestrator (Go)..."
    )
    add_dependencies(interlock_core atlas_go)
endif()

# --- ZIG (transfer-zig) ---
if(BUILD_ZIG_PAYLOADS)
    add_custom_target(payloads_zig
        COMMAND ${ZIG} build -Doptimize=ReleaseFast
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/backend/transfer-zig"
        COMMENT "Compilando Artifact Orchestrator (Zig)..."
    )
    add_dependencies(interlock_core payloads_zig)
endif()

# ============================================================================
# INFRAESTRUCTURA IPC Y ABB (Shared Memory)
# ============================================================================
if(ENABLE_ABB)
    message(STATUS "Habilitando Autonomous Breadcrumb Buffer (ABB)...")
    add_compile_definitions(ENABLE_ABB=1)
    # El ABB se inicializa en InterlockCore.cpp
endif()

if(ENABLE_DTRACE_PROBES)
    if(APPLE)
        message(STATUS "Habilitando DTrace Boundary Probes (macOS)...")
        add_compile_definitions(ENABLE_DTRACE=1)
    endif()
endif()

# ============================================================================
# CONFIGURACIÓN DE PRODUCCIÓN
# ============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "BUILD PRODUCTION: Interlock v${INTERLOCK_VERSION}")
    add_compile_definitions(
        INTERLOCK_PRODUCTION=1
        INTERLOCK_VERSION="${INTERLOCK_VERSION}"
    )

    # Striping de símbolos en Release
    if(APPLE)
        add_link_options(-Wl,-S)
    endif()
endif()

# ============================================================================
# DIRECTORIOS DE SALIDA
# ============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ============================================================================
# CONFIGURACIÓN DE TESTS (Opcional)
# ============================================================================
enable_testing()
add_subdirectory(backend/tests)

# ============================================================================
# INSTALACIÓN
# ============================================================================
install(TARGETS interlock_core DESTINATION lib)
install(DIRECTORY backend/include/ DESTINATION include/interlock)
install(FILES backend/config/ide.toml backend/config/agents.toml DESTINATION etc/interlock)

# ============================================================================
# RESUMEN DE COMPILACIÓN
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Interlock Assessment IDE - Build Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${INTERLOCK_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Polyglot Modules:")
message(STATUS "  C++ Core (RDT): ${BUILD_CPP_CORE}")
message(STATUS "  Swift Middleware: ${BUILD_SWIFT_MIDDLEWARE}")
message(STATUS "  Rust Cipher: ${BUILD_RUST_CIPHER}")
message(STATUS "  Go Atlas: ${BUILD_GO_ATLAS}")
message(STATUS "  Zig Payloads: ${BUILD_ZIG_PAYLOADS}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  OLLVM Obfuscation: ${ENABLE_OLLVM}")
message(STATUS "  Breadcrumb Buffer (ABB): ${ENABLE_ABB}")
message(STATUS "  DTrace Probes: ${ENABLE_DTRACE_PROBES}")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# FRONTEND NATIVO (Qt6 / QML) - High-Performance IDE UI
# ============================================================================
if(APPLE OR LINUX)
    # 1. Buscar dependencias de Qt6
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Widgets)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    set(APP_NAME "InterlockIDE_Qt")

    # 2. Definir recursos y fuentes
    file(GLOB_RECURSE QT_SOURCES
        "frontend-qt/*.cpp"
        "frontend-qt/*.hpp"
    )

    # 3. Recursos QML (Imágenes, Estilos, Componentes)
    set(QML_RESOURCES "frontend-qt/resources.qrc")

    # 4. Crear el Ejecutable como un Bundle de macOS
    add_executable(${APP_NAME} MACOSX_BUNDLE
        ${QT_SOURCES}
        ${QML_RESOURCES}
    )

    # 5. Configuración del Bundle (Info.plist metadata)
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.abiterated.interlock.ide"
        MACOSX_BUNDLE_BUNDLE_NAME "Interlock Assessment IDE"
        MACOSX_BUNDLE_BUNDLE_VERSION "${INTERLOCK_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${INTERLOCK_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2024 Abiterated Systems"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/frontend-qt/Info.plist"
    )

    # 6. Vincular con Qt6 y el Core C++
    target_link_libraries(${APP_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::Widgets
        interlock_core
    )

    # 7. Empaquetado Exhaustivo del Backend en el Bundle
    # Configuraciones
    file(GLOB CONFIG_FILES "backend/config/*.toml")
    foreach(CFG ${CONFIG_FILES})
        target_sources(${APP_NAME} PRIVATE "${CFG}")
        set_source_files_properties("${CFG}" PROPERTIES MACOSX_BUNDLE_LOCATION "Resources/backend/config")
    endforeach()

    # Data Surgery (Perl)
    file(GLOB_RECURSE SURGERY_FILES "backend/artifact-pl/*.pl" "backend/artifact-pl/*.pm")
    foreach(FILE ${SURGERY_FILES})
        target_sources(${APP_NAME} PRIVATE "${FILE}")
        set_source_files_properties("${FILE}" PROPERTIES MACOSX_BUNDLE_LOCATION "Resources/backend/artifact-pl")
    endforeach()

    # Web Intel (TypeScript/JS)
    file(GLOB_RECURSE INTEL_FILES "backend/discovery-ts/src/*.ts" "backend/discovery-ts/package.json")
    foreach(FILE ${INTEL_FILES})
        target_sources(${APP_NAME} PRIVATE "${FILE}")
        set_source_files_properties("${FILE}" PROPERTIES MACOSX_BUNDLE_LOCATION "Resources/backend/discovery-ts")
    endforeach()

    # 8. Binarios Críticos (Go, Rust, Zig) -> Van a Contents/MacOS
    set(BINARY_MODULES
        "${CMAKE_BINARY_DIR}/bin/atlas_orchestrator"
        "${CMAKE_BINARY_DIR}/bin/interlock_payload_orchestrator"
        "${CMAKE_BINARY_DIR}/lib/libcipher_rs.dylib"
    )

    foreach(BIN ${BINARY_MODULES})
        if(EXISTS "${BIN}")
            target_sources(${APP_NAME} PRIVATE "${BIN}")
            set_source_files_properties("${BIN}" PROPERTIES MACOSX_BUNDLE_LOCATION "MacOS")
        endif()
    endforeach()

    # 9. Modelos GGUF (IA) -> Resources/backend/models
    file(GLOB GGUF_MODELS "${CMAKE_CURRENT_SOURCE_DIR}/backend/models/*.gguf")
    foreach(MODEL ${GGUF_MODELS})
        target_sources(${APP_NAME} PRIVATE "${MODEL}")
        set_source_files_properties("${MODEL}" PROPERTIES MACOSX_BUNDLE_LOCATION "Resources/backend/models")
    endforeach()

    # 10. Post-Build: macdeployqt y Sanación de Bundle
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        # Desplegar dependencias de Qt
        COMMAND "${_qt_bin_dir}/macdeployqt" "$<TARGET_FILE_DIR:${APP_NAME}>/../.."
                -qmldir="${CMAKE_CURRENT_SOURCE_DIR}/frontend-qt"
                -libpath=${BREW_LIB_PATH}
                -executable="$<TARGET_FILE_DIR:${APP_NAME}>/atlas_orchestrator"

        # Asegurar carpetas de datos escribibles (se inicializarán en Resources o AppSupport)
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${APP_NAME}>/../Resources/backend/data/db"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${APP_NAME}>/../Resources/backend/data/logs"

        # Firmado final para ejecución en M4 Pro
        COMMAND codesign --force --deep --sign - "$<TARGET_FILE_DIR:${APP_NAME}>/../.."

        COMMENT "Generando Bundle unificado InterlockIDE.app..."
    )

    message(STATUS "Frontend Qt6 configurado como macOS Bundle: ${APP_NAME}.app")
endif()
