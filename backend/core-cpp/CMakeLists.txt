# Core C++ Module (RDT & GPU Inference)
cmake_minimum_required(VERSION 3.24)
project(interlock_core LANGUAGES C CXX ASM OBJC OBJCXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python and nanobind
find_package(Python 3.14 COMPONENTS Interpreter Development REQUIRED)

execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_VARIABLE nanobind_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
list(APPEND CMAKE_PREFIX_PATH "${nanobind_DIR}")
find_package(nanobind CONFIG REQUIRED)

execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --include_dir
    OUTPUT_VARIABLE NANOBIND_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Support for PostgreSQL, OpenSSL, Protobuf and gRPC
set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/libpqxx/lib/pkgconfig:/opt/homebrew/opt/libpq/lib/pkgconfig:/opt/homebrew/opt/abseil/lib/pkgconfig:/opt/homebrew/opt/openssl@3/lib/pkgconfig:/opt/homebrew/opt/protobuf/lib/pkgconfig:/opt/homebrew/opt/grpc/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX libpqxx REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(ABSL absl_log absl_hash absl_status absl_strings absl_synchronization REQUIRED)
pkg_check_modules(PROTOBUF protobuf REQUIRED)
pkg_check_modules(GRPC grpc++ REQUIRED)

# Nanobind static build from source
set(NANOBIND_SRC_DIR "${nanobind_DIR}/..")
add_library(nanobind-static STATIC
    "${NANOBIND_SRC_DIR}/src/common.cpp"
    "${NANOBIND_SRC_DIR}/src/error.cpp"
    "${NANOBIND_SRC_DIR}/src/implicit.cpp"
    "${NANOBIND_SRC_DIR}/src/nb_enum.cpp"
    "${NANOBIND_SRC_DIR}/src/nb_func.cpp"
    "${NANOBIND_SRC_DIR}/src/nb_internals.cpp"
    "${NANOBIND_SRC_DIR}/src/nb_ndarray.cpp"
    "${NANOBIND_SRC_DIR}/src/nb_static_property.cpp"
    "${NANOBIND_SRC_DIR}/src/nb_type.cpp"
    "${NANOBIND_SRC_DIR}/src/trampoline.cpp"
)
target_include_directories(nanobind-static PUBLIC
    "${NANOBIND_SRC_DIR}/include"
    "${NANOBIND_SRC_DIR}/ext/robin_map/include"
    "${Python_INCLUDE_DIRS}"
)
target_compile_definitions(nanobind-static PRIVATE NB_STATIC)

# Define llama.cpp paths
set(LLAMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/llama_sourcecode/llama.cpp-master")

# --- Dual-Backend Configuration (MLX + Llama) ---
if(APPLE)
    message(STATUS "[Interlock Core] Configurando Dual-Backend (MLX + Llama) para macOS")
    add_definitions(-DUSE_MLX)
    add_definitions(-DUSE_LLAMA)

    # MLX Frameworks y dependencias nativas
    find_library(MLX_LIB mlx PATHS /opt/homebrew/lib REQUIRED)

    # Configure llama.cpp
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama: build examples" FORCE)
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama: build tests" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama: build server" FORCE)
    set(LLAMA_BUILD_COMMON ON CACHE BOOL "llama: build common utils library" FORCE)
    add_subdirectory(${LLAMA_DIR} llama.cpp)

    set(INFERENCE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine_MLX.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine_Llama.mm"
    )

    set(INFERENCE_LIBS
        ${MLX_LIB}
        llama
        ggml
        llama-common
        "-framework Metal"
        "-framework Foundation"
        "-framework Accelerate"
        "-framework MetalPerformanceShaders"
    )
else()
    message(STATUS "[Interlock Core] Configurando Backend GGUF (Windows/Universal)")
    add_definitions(-DUSE_LLAMA)

    # Configure llama.cpp options before including it
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama: build examples" FORCE)
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama: build tests" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama: build server" FORCE)
    set(LLAMA_BUILD_COMMON ON CACHE BOOL "llama: build common utils library" FORCE)

    # Add llama.cpp as a subdirectory
    add_subdirectory(${LLAMA_DIR} llama.cpp)

    set(INFERENCE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine_Llama.mm"
    )
    set(INFERENCE_LIBS llama ggml llama-common)
endif()

# --- Mach IPC (MIG) Generation ---
set(MIG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/mach_gen")
file(MAKE_DIRECTORY "${MIG_OUTPUT_DIR}")

set(MIG_DEFS
    "${CMAKE_CURRENT_SOURCE_DIR}/mach/interlock_ipc.defs"
    "${CMAKE_CURRENT_SOURCE_DIR}/mach/anti_debug.defs"
)

set(MIG_GENERATED_SOURCES "")
foreach(DEF ${MIG_DEFS})
    get_filename_component(DEF_NAME ${DEF} NAME_WE)
    set(USER_C "${MIG_OUTPUT_DIR}/${DEF_NAME}User.c")
    set(SERVER_C "${MIG_OUTPUT_DIR}/${DEF_NAME}Server.c")
    set(HEADER "${MIG_OUTPUT_DIR}/${DEF_NAME}.h")

    add_custom_command(
        OUTPUT "${USER_C}" "${SERVER_C}" "${HEADER}"
        COMMAND mig -header "${HEADER}" -user "${USER_C}" -server "${SERVER_C}" "${DEF}"
        DEPENDS "${DEF}"
        WORKING_DIRECTORY "${MIG_OUTPUT_DIR}"
        COMMENT "Generating Mach IPC interfaces from ${DEF_NAME}.defs"
    )
    list(APPEND MIG_GENERATED_SOURCES "${USER_C}" "${SERVER_C}")
endforeach()

# Collect core sources
file(GLOB CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.metal"
    "${CMAKE_CURRENT_SOURCE_DIR}/mach/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mach/*.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/agents/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/security/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Math/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Memory/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Streaming/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../stubs-asm/darwin_arm64/*.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/../shield-c/shield_core.c"
)
list(REMOVE_ITEM CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/TrustedCoreFFI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mach/MachInjector.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine_Llama.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/InferenceEngine_MLX.mm"
)
list(APPEND CORE_SOURCES ${INFERENCE_SOURCES})

# --- MachInjector target ---
add_executable(interlock_mach_injector "${CMAKE_CURRENT_SOURCE_DIR}/mach/MachInjector.cpp" "${MIG_OUTPUT_DIR}/interlock_ipcUser.c")
add_dependencies(interlock_mach_injector interlock_core_bin) # Ensure MIG runs
target_include_directories(interlock_mach_injector PRIVATE ${MIG_OUTPUT_DIR})
target_include_directories(interlock_mach_injector PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/mach")
target_link_libraries(interlock_mach_injector PRIVATE "-framework Foundation")

# Build standalone executable
add_executable(interlock_core_bin ${CORE_SOURCES} ${PB_SOURCES} ${MIG_GENERATED_SOURCES})
set_target_properties(interlock_core_bin PROPERTIES
    OUTPUT_NAME interlock_core
    LINK_FLAGS "-Wl,-dead_strip"
)
target_include_directories(interlock_core_bin PRIVATE ${MIG_OUTPUT_DIR})
target_include_directories(interlock_core_bin PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/agents"
    "${CMAKE_CURRENT_SOURCE_DIR}/pb"
    "${CMAKE_CURRENT_SOURCE_DIR}/../shield-c"
    "/opt/homebrew/include"
    "/opt/homebrew/lib/ocaml"
    "${LLAMA_DIR}/include"
    "${LLAMA_DIR}/ggml/include"
    "${LLAMA_DIR}/common"
    "${LLAMA_DIR}/vendor"
    "${NANOBIND_INCLUDE_DIR}"
    "${Python_INCLUDE_DIRS}"
    "${LIBPQXX_INCLUDE_DIRS}"
    "${ABSL_INCLUDE_DIRS}"
    "${PROTOBUF_INCLUDE_DIRS}"
    "${GRPC_INCLUDE_DIRS}"
)

# Link requirements for executable
target_link_libraries(interlock_core_bin PRIVATE
    ${INFERENCE_LIBS}
    nanobind-static
    Python::Python
    ${LIBPQXX_LDFLAGS}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ABSL_LDFLAGS}
    ${PROTOBUF_LDFLAGS}
    ${GRPC_LDFLAGS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../cipher-rs/target/release/libcipher_rs.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/../payloads-zig/zig-out/lib/libinterlock_payloads.a"
    "-framework Foundation"
    "-framework Metal"
    "-framework MetalPerformanceShaders"
    "-framework Accelerate"
    "-framework IOKit"
    "-framework Security"
    "-framework CoreFoundation"
    "-framework Network"
)

# Crear librería ESTÁTICA
add_library(interlock_core STATIC ${CORE_SOURCES} ${AGENT_SOURCES} ${PB_SOURCES} ${MIG_GENERATED_SOURCES})
target_include_directories(interlock_core PRIVATE ${MIG_OUTPUT_DIR})

target_include_directories(interlock_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/agents"
    "${CMAKE_CURRENT_SOURCE_DIR}/pb"
    "${CMAKE_CURRENT_SOURCE_DIR}/../shield-c"
    "/opt/homebrew/include"
    "/opt/homebrew/lib/ocaml"
    "${LLAMA_DIR}/include"
    "${LLAMA_DIR}/ggml/include"
    "${LLAMA_DIR}/common"
    "${LLAMA_DIR}/vendor"
    "${NANOBIND_INCLUDE_DIR}"
    "${Python_INCLUDE_DIRS}"
    "${LIBPQXX_INCLUDE_DIRS}"
    "${ABSL_INCLUDE_DIRS}"
    "${PROTOBUF_INCLUDE_DIRS}"
    "${GRPC_INCLUDE_DIRS}"
)

set_target_properties(interlock_core PROPERTIES
    FRAMEWORK TRUE
    MACOSX_FRAMEWORK_IDENTIFIER com.interlock.core
    VERSION 1.0.0
    SOVERSION 1
)

# Link requirements
target_link_libraries(interlock_core PRIVATE
    ${INFERENCE_LIBS}
    nanobind-static
    Python::Python
    ${LIBPQXX_LDFLAGS}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ABSL_LDFLAGS}
    ${PROTOBUF_LDFLAGS}
    ${GRPC_LDFLAGS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../cipher-rs/target/release/libcipher_rs.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/../payloads-zig/zig-out/lib/libinterlock_payloads.a"
    "-framework Foundation"
    "-framework Metal"
    "-framework MetalPerformanceShaders"
    "-framework Accelerate"
    "-framework IOKit"
    "-framework Security"
    "-framework CoreFoundation"
    "-framework Network"
)
