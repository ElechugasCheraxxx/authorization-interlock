cmake_minimum_required(VERSION 3.24)
project(Interlock_Backend_CPP LANGUAGES C CXX OBJCXX)

# Estándar C++23 para máximo rendimiento y características modernas
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Integrity Armor Configuration (Hito 4.1) ---
# Ahora basado en C++23 Metaprogramming (IntegrityArmor.hpp)
add_definitions(-DSTEALTH_ENABLED=1)

# --- OLLVM / Obscura Integration (Hito 4.7) ---
option(ENABLE_OLLVM "Activar ofuscación de nivel IR (Obscura-LLVM)" ON)
if(ENABLE_OLLVM AND APPLE)
    set(OBSCURA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/core-cpp/obfuscator/obscura-llvm19.1.5-darwin-universal-release")
    if(EXISTS "${OBSCURA_PATH}/lib/ld.sh")
        message(STATUS "Obscura-LLVM detectado (Persistencia Local). Aplicando protección de nivel IR.")
        set(CMAKE_LINKER "${OBSCURA_PATH}/lib/ld.sh")
        # Flags para colapsar el grafo de control y sustitución de instrucciones
        # Note: Discrete arguments are used to prevent Clang interpretation errors.
        # Fixed syntax: OLLVM flags should be passed as -mllvm <flag>
        # Re-disabling temporarily to isolate build issues if still failing
        # add_compile_options("-mllvm" "-fla")
        # add_compile_options("-mllvm" "-sub")
        # add_compile_options("-mllvm" "-bcf")
    endif()
endif()

# Optimizaciones para Apple Silicon (M4 Pro)
if(APPLE)
    add_compile_options(-O3 -Wno-elaborated-enum-base -fno-stack-check)
endif()

# Intentar encontrar Python para el Bridge
find_package(Python 3.9 COMPONENTS Interpreter Development REQUIRED)

# Configuración de nanobind
execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_VARIABLE nanobind_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
find_package(nanobind CONFIG REQUIRED PATHS "${nanobind_DIR}")

# Soporte para PostgreSQL y OpenSSL
set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/libpqxx/lib/pkgconfig:/opt/homebrew/opt/libpq/lib/pkgconfig:/opt/homebrew/opt/abseil/lib/pkgconfig:/opt/homebrew/opt/openssl@3/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX libpqxx REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(ABSL absl_log absl_hash absl_status absl_strings absl_synchronization absl_log_internal_check_op absl_log_internal_message REQUIRED)

# Soporte para llama.cpp (Integración Nativa)
set(LLAMA_DIR "/Users/Cheraxxx/Interlock IDE/backend/core-cpp/llama_sourcecode/llama-b9388")
set(LLAMA_SOURCE_DIR "/Users/Cheraxxx/Interlock IDE/backend/core-cpp/llama_sourcecode/llama.cpp-master")

include_directories("${LLAMA_SOURCE_DIR}/include")
include_directories("${LLAMA_SOURCE_DIR}/ggml/include")
include_directories("${LLAMA_SOURCE_DIR}/common")

link_directories(${LLAMA_DIR})

# --- Rust Integration (Cipher Module) ---
find_program(CARGO_EXECUTABLE cargo REQUIRED)
set(CIPHER_RS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cipher-rs")
set(CIPHER_RS_LIB "${CIPHER_RS_DIR}/target/release/libcipher_rs.a")

add_custom_command(
    OUTPUT "${CIPHER_RS_LIB}"
    COMMAND ${CARGO_EXECUTABLE} build --release
    WORKING_DIRECTORY "${CIPHER_RS_DIR}"
    DEPENDS "${CIPHER_RS_DIR}/src/lib.rs" "${CIPHER_RS_DIR}/Cargo.toml"
    COMMENT "Compilando Módulo Cipher en Rust (PQC)..."
)

add_custom_target(cipher_rs_target DEPENDS "${CIPHER_RS_LIB}")

# --- Zig Integration (Artifact Orchestrator - Hito 9) ---
find_program(ZIG_EXECUTABLE zig REQUIRED)
set(ZIG_PAYLOAD_LIB "${CMAKE_CURRENT_SOURCE_DIR}/transfer-zig/zig-out/lib/libinterlock_payloads.a")

add_custom_command(
    OUTPUT "${ZIG_PAYLOAD_LIB}"
    COMMAND ${ZIG_EXECUTABLE} build -Doptimize=ReleaseFast
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/transfer-zig"
    # Assuming main file is in src/
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/transfer-zig/build.zig"
    COMMENT "Compilando Zig Integrity Artifact Orchestrator..."
)
add_custom_target(payload_zig_target DEPENDS "${ZIG_PAYLOAD_LIB}")

# --- C# NativeAOT Integration (Operations Engine - Hito 10) ---
set(TACTICAL_CS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/operations-cs")
set(TACTICAL_CS_LIB "${TACTICAL_CS_DIR}/bin/Release/net10.0/osx-arm64/publish/OperationsEngine.a")

add_custom_command(
    OUTPUT "${TACTICAL_CS_LIB}"
    COMMAND dotnet publish -r osx-arm64 -c Release /p:NativeLib=Static
    WORKING_DIRECTORY "${TACTICAL_CS_DIR}"
    DEPENDS "${TACTICAL_CS_DIR}/OperationsEngine.csproj" "${TACTICAL_CS_DIR}/Models.cs" "${TACTICAL_CS_DIR}/MissionOrchestrator.cs"
    COMMENT "Compilando Operations Engine C# via NativeAOT..."
)
add_custom_target(tactical_cs_target DEPENDS "${TACTICAL_CS_LIB}")

# --- gRPC / Protobuf Integration (Atlas Module) ---
set(GRPC_PATH "/opt/homebrew/opt/grpc")
set(PROTOBUF_PATH "/opt/homebrew/opt/protobuf")

include_directories("${GRPC_PATH}/include")
include_directories("${PROTOBUF_PATH}/include")
include_directories("core-cpp/pb")

link_directories("${GRPC_PATH}/lib")
link_directories("${PROTOBUF_PATH}/lib")

# Archivos generados por protoc
set(ATLAS_PROTO_SOURCES
    core-cpp/pb/atlas.pb.cc
    core-cpp/pb/atlas.grpc.pb.cc
)

# Módulo Shield (C Puro para sigilo)
set(SHIELD_SOURCES
    shield-c/shield_core.c
)

# Definir la librería shield_static para asegurar que los símbolos C estén disponibles
add_library(shield_static STATIC ${SHIELD_SOURCES})
set_target_properties(shield_static PROPERTIES LINKER_LANGUAGE C)

# Módulo Assembly (ARM64 Stubs)
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(ASM_SOURCES
        stubs-asm/darwin_arm64/syscalls.s
    )
    enable_language(ASM)
endif()

# Directorios de inclusión
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(include)
include_directories(include/interop)
include_directories(core-cpp)
include_directories(core-cpp/mach_gen)
include_directories(/opt/homebrew/include)
include_directories(${Python_INCLUDE_DIRS})
include_directories(${LIBPQXX_INCLUDE_DIRS})
include_directories(${ABSL_INCLUDE_DIRS})

# Directorios de bibliotecas
link_directories(${LIBPQXX_LIBRARY_DIRS})
link_directories(${ABSL_LIBRARY_DIRS})

# Configuración de nanobind (Obtener rutas de inclusión adicionales)
execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --include_dir
    OUTPUT_VARIABLE NANOBIND_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(${NANOBIND_INCLUDE_DIR})

# --- OCaml / Coq Trusted Core Integration ---
set(OCAML_INCLUDE_DIR "/opt/homebrew/lib/ocaml")
include_directories(${OCAML_INCLUDE_DIR})

add_subdirectory(proofs)
set(TRUSTED_CORE_LIB "${CMAKE_CURRENT_BINARY_DIR}/proofs/libtrusted_core.dylib")

# Definición de archivos fuente - CORE
set(CORE_SOURCES
    ${ATLAS_PROTO_SOURCES}
    ${SHIELD_SOURCES}
    ${ASM_SOURCES}
    core-cpp/InterlockCore.cpp
    core-cpp/MessageRouter.cpp
    core-cpp/OrchestrationEngine.cpp
    core-cpp/PromptEngine.cpp
    core-cpp/ToolDispatcher.cpp
    core-cpp/ContextManager.cpp
    core-cpp/NativeAgent.cpp
    core-cpp/NativeAgentActions.cpp
    core-cpp/AgentEngine.cpp
    core-cpp/Blackboard.cpp
    core-cpp/MoERouter.cpp
    core-cpp/MythosTokenizer.cpp
    core-cpp/RecurrentEngine.cpp
    core-cpp/DepthCacheManager.cpp
    agents-cs/Coordinator.cpp
    agents-cs/Chronos.cpp
    agents-cs/Cipher.cpp
    agents-cs/Vantage.cpp
    agents-cs/Atlas.cpp
    agents-cs/Apex.cpp
    agents-cs/Beacon.cpp
    agents-cs/Overseer.cpp
    agents-cs/Grader.cpp
    agents-cs/Minimizer.cpp
    agents-cs/Archive.cpp
    core-cpp/DatabaseManager.cpp
    core-cpp/InferenceEngine.mm
    core-cpp/StateMachine.cpp
    core-cpp/SessionManager.cpp
    core-cpp/PythonBridge.cpp
    core-cpp/ProcessManager.mm
    core-cpp/GPUAllocator.mm
    core-cpp/RAGEngine.cpp
    core-cpp/PDGEngine.cpp
    core-cpp/EvolutionEngine.cpp
    core-cpp/RiskManager.mm
    core-cpp/C2Manager.cpp
    core-cpp/PivotingManager.cpp
    core-cpp/ArtifactManager.cpp
    core-cpp/SecureContainer.cpp
    core-cpp/SandboxManager.cpp
    core-cpp/SurfaceMapper.cpp
    core-cpp/EventSystem.cpp
    core-cpp/DetectionModule.cpp
    core-cpp/CognitiveEngines.cpp
    core-cpp/TokenDispatcher.cpp
    core-cpp/HealthMonitor.cpp
    core-cpp/AutomationModule.cpp
    core-cpp/ScopeGuard.cpp
    core-cpp/PQCAssessor.cpp
    core-cpp/SovereignBridge.cpp
    core-cpp/ArtifactEngine.cpp
    core-cpp/FormalBridge.cpp
    core-cpp/agents/Cipher.cpp
    core-cpp/agents/Chronos.cpp
    core-cpp/agents/Overseer.cpp
    core-cpp/agents/Grader.cpp
    core-cpp/agents/Apex.cpp
    core-cpp/agents/Vantage.cpp
    core-cpp/agents/Beacon.cpp
    core-cpp/agents/Coordinator.cpp
    core-cpp/agents/Probe.cpp
    core-cpp/agents/Minimizer.cpp
    core-cpp/agents/Atlas.cpp
    core-cpp/agents/Archive.cpp
    core-cpp/ExecutionManager.cpp
    core-cpp/GhidraBridge.cpp
    core-cpp/NetworkBridge.mm
    core-cpp/TransferBridge.cpp
    core-cpp/mach/MachServer.mm
    core-cpp/mach/MachPriority.mm
    core-cpp/mach/AntiDebuggerMachMonitor.mm
)

# Definición de archivos fuente - INFRAESTRUCTURA (ABB, IPC)
set(INFRA_SOURCES
    core-cpp/AutonomousBreadcrumbBuffer.cpp
    core-cpp/IPCBridge.cpp
)

# Librería estática para integración con Swift
add_library(interlock_static STATIC ${CORE_SOURCES} ${INFRA_SOURCES})
set_target_properties(interlock_static PROPERTIES
    PUBLIC_HEADER "include/InterlockCore.h"
)

# Ejecutable principal
add_executable(interlock_core core-cpp/main.cpp ${CORE_SOURCES} ${INFRA_SOURCES})

# Módulo de nanobind (C++ a Python)
nanobind_add_module(interlock_core_bind bridge/bindings.cpp ${CORE_SOURCES} ${INFRA_SOURCES})

# --- Unit Tests (C++) ---
enable_testing()
add_executable(core_tests tests/core_tests.cpp ${CORE_SOURCES} ${INFRA_SOURCES})
add_executable(stability_tests tests/stability_tests.cpp ${CORE_SOURCES} ${INFRA_SOURCES})
add_executable(e2e_mission_tests tests/e2e_mission_tests.cpp ${CORE_SOURCES} ${INFRA_SOURCES})
add_executable(benchmark_rdt tests/benchmark_rdt.cpp ${CORE_SOURCES} ${INFRA_SOURCES})
add_executable(test_data_surgery_stress tests/test_data_surgery_stress.cpp ${CORE_SOURCES} ${INFRA_SOURCES})
add_executable(formal_pbt_tests tests/formal_pbt_tests.cpp ${CORE_SOURCES} ${INFRA_SOURCES})

# Vincular con Python, libpqxx, LLaMA Native, Rust Cipher, Zig Artifact, C# Operations, gRPC y Trusted Core (OCaml)
set(LLAMA_LIBS llama llama-common ggml ggml-base ggml-cpu ggml-metal)
set(RUST_LIBS "${CIPHER_RS_LIB}")
set(ZIG_LIBS "${ZIG_PAYLOAD_LIB}")
set(CS_LIBS "${TACTICAL_CS_LIB}")
set(OCAML_LIBS "${TRUSTED_CORE_LIB}" "/opt/homebrew/lib/ocaml/libasmrun_pic.a")
set(GRPC_LIBS grpc++ grpc gpr protobuf)
set(ABSL_LIBS ${ABSL_LIBRARIES})

target_link_libraries(interlock_static PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(interlock_core PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(interlock_core_bind PRIVATE shield_static nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(core_tests PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(stability_tests PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(e2e_mission_tests PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(benchmark_rdt PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(test_data_surgery_stress PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})
target_link_libraries(formal_pbt_tests PRIVATE shield_static Python::Python nanobind-static ${LIBPQXX_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ${LLAMA_LIBS} ${RUST_LIBS} ${ZIG_LIBS} ${CS_LIBS} ${OCAML_LIBS} ${GRPC_LIBS} ${ABSL_LIBS})

add_dependencies(interlock_static cipher_rs_target payload_zig_target tactical_cs_target trusted_core_dylib)
add_dependencies(interlock_core cipher_rs_target payload_zig_target tactical_cs_target trusted_core_dylib)
add_dependencies(interlock_core_bind cipher_rs_target payload_zig_target tactical_cs_target trusted_core_dylib)
add_dependencies(test_data_surgery_stress cipher_rs_target payload_zig_target tactical_cs_target trusted_core_dylib)
add_dependencies(formal_pbt_tests cipher_rs_target payload_zig_target tactical_cs_target trusted_core_dylib)

# Soporte para Metal (Apple Silicon)
if(APPLE)
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(QUARTZ_FRAMEWORK QuartzCore)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(interlock_static PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(interlock_core PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(interlock_core_bind PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(core_tests PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(stability_tests PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(e2e_mission_tests PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(benchmark_rdt PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(test_data_surgery_stress PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
    target_link_libraries(formal_pbt_tests PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_FRAMEWORK} ${IOKIT_FRAMEWORK})
endif()

message(STATUS "Configuración de C++ Backend para M4 Pro completada (Integrity Armor: ACTIVE).")
