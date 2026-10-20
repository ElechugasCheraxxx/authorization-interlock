# Interlock Assessment IDE - Production Polyglot Architecture

## Descripción General

**Interlock Assessment IDE** es una plataforma de orquestación ofensiva de próxima generación construida con arquitectura poliglota. Integra el mejor de cada lenguaje de programación:

- **C++ 23**: Motor de inferencia RDT con aceleración GPU (Metal Performance Shaders)
- **Rust**: Criptografía post-cuántica (PQC) segura en memoria
- **Go**: Orquestación masiva de swarms (5000+ agentes concurrentes)
- **Swift**: Middleware seguro con actores e interoperabilidad bidireccional con C++
- **Zig**: Polimorfismo de payloads y generación de shellcode
- **C**: Shield engine y syscalls directos (bypass EDR)
- **Assembly (ARM64)**: Stubs de bajo nivel, anti-RE, anti-VM
- **TypeScript/Bun**: Web intelligence y automatización de objetivos
- **Perl**: Data Surgery para procesamiento ultra-rápido de artefactos
- **Coq/Isabelle/HOL**: Verificación formal de algoritmos criptográficos

## Arquitectura de Infraestructura

### Autonomous Breadcrumb Buffer (ABB)
Buffer circular lock-free persistente en memoria compartida (`/dev/shm/interlock_abb_prod`). Registra todas las transiciones entre módulos políglotas para auditoría y depuración post-crash.

```
[Timestamp] [Source ID] [Target ID] [Func Pointer] [Stack Depth]
64 bytes por entrada × 4M entradas = 256 MB
```

### IPC Bridge Unificado
Sistema de comunicación inter-lenguajes mediante:
- **Unix Domain Sockets** (UDS) - baja latencia, aislamiento de runtimes
- **FlatBuffers** - serialización sin parseo
- **Timeout de 5s** - para request/response

### Swift-C++ Bidirectional Interop
Integración nativa sin wrappers `extern "C"`, aprovechando Swift 5.9+ para importación directa de código C++.

## Quick Start

### 1. Compilar Todo

```bash
cd /Users/Cheraxxx/Interlock\ Assessment\ IDE

# Compilación limpia
./build.sh

# O compilación incremental
cd build && make -j8
```

**Tiempo estimado**: 15-30 minutos (depende del hardware)

**Requisitos previos**:
- Xcode Command Line Tools (clang++, swift)
- CMake >= 3.24
- Rust (cargo)
- Go 1.20+
- Zig 0.13+
- Perl

### 2. Deployment en Producción

```bash
# Preparar entorno
chmod +x ./build.sh ./deploy.sh

# Compilar
./build.sh

# Desplegar
./deploy.sh start

# Verificar salud
./deploy.sh status

# Ver logs
tail -f /var/log/interlock/*.log

# Detener
./deploy.sh stop
```

### 3. Verificar Componentes

```bash
# Listar procesos activos
ps aux | grep interlock

# Verificar IPC sockets
ls -la /tmp/interlock_ipc_*

# Inspeccionar ABB
hexdump -C /dev/shm/interlock_abb_prod | head -50

# Ver logs del core
tail -100 /var/log/interlock/core.log
```

## Estructura de Directorios

```
/Interlock Assessment IDE/
├── backend/
│   ├── core-cpp/                 # Motor C++23 + RDT
│   │   ├── InterlockCore.cpp
│   │   ├── AutonomousBreadcrumbBuffer.cpp
│   │   ├── IPCBridge.cpp
│   │   └── ... (60+ módulos)
│   ├── middleware-swift/         # Orquestación segura
│   ├── cipher-rs/                # Criptografía PQC (Rust)
│   ├── atlas-go/                 # Swarm orchestrator
│   ├── transfer-zig/             # Polimorfismo de payloads
│   ├── shield-c/                 # Kernel Shield
│   ├── stubs-asm/                # Syscalls ARM64
│   ├── discovery-ts/                 # Web intelligence
│   ├── artifact-pl/               # Data surgery (Perl)
│   ├── proofs/                   # Verificación formal
│   ├── tests/                    # Test suite
│   ├── tools/                    # Herramientas (abb_inspector.py)
│   ├── config/                   # Configuración de producción
│   │   ├── production.toml
│   │   ├── ide.toml
│   │   └── agents.toml
│   ├── data/                     # Runtime data (cache, logs, etc.)
│   ├── services/                 # Microservicios
│   ├── platform/                 # Infraestructura
│   ├── models/                   # ML models
│   ├── include/                  # Headers públicos
│   │   ├── AutonomousBreadcrumbBuffer.hpp
│   │   ├── IPCBridge.hpp
│   │   └── ...
│   └── CMakeLists.txt
├── build/
│   ├── bin/                      # Ejecutables compilados
│   ├── lib/                      # Librerías estáticas/dinámicas
│   └── cmake/                    # Configuración CMake
├── dist/
│   └── (Artefactos de deployment)
├── CMakeLists.txt                # Build system modular
├── build.sh                      # Script de compilación
└── deploy.sh                     # Script de deployment
```

## Variables de Entorno (Producción)

```bash
# Rutas
export INTERLOCK_HOME="/Users/Cheraxxx/Interlock Assessment IDE"
export INTERLOCK_BACKEND="$INTERLOCK_HOME/backend"
export INTERLOCK_CONFIG="$INTERLOCK_BACKEND/config/production.toml"
export INTERLOCK_LOG_DIR="/var/log/interlock"
export INTERLOCK_PID_DIR="/var/run/interlock"

# Credenciales (usar siempre desde env)
export INTERLOCK_DB_PASSWORD="<base64-encoded>"
export INTERLOCK_REDIS_PASSWORD="<base64-encoded>"

# Modo producción
export INTERLOCK_PRODUCTION=1
export INTERLOCK_BUILD_TYPE=Release
```

## Componentes Principales

### Core C++ (Inferencia RDT)
```cpp
#include "InterlockCore.h"

// Inicializar ABB
interlock::AutonomousBreadcrumbBuffer::getInstance(true);

// Inicializar IPC Bridge
interlock::IPCBridge::getInstance().initializeServers();

// Ejecutar inferencia
interlock::InterlockCore core;
core.initialize();
```

### Comunicación IPC

```cpp
// Cliente que envía a Go Atlas
interlock::IPCClient client(COMPONENT_CPP_CORE);
client.connect("/tmp/interlock_ipc_4");

interlock::IPCMessage msg{};
msg.sender = COMPONENT_CPP_CORE;
msg.receiver = COMPONENT_GO_ATLAS;
msg.message_id = 0x1001;

interlock::IPCMessage response;
client.sendRequest(msg, response, 5000);
```

### Swift Middleware

```swift
// Integración directa con C++
import InterlockCore

// Usar el Core desde Swift (sin wrappers)
let core = InterlockCore()
core.initialize()

// Actor para orquestación segura
actor MissionOrchestrator {
    func deployAgent(_ config: AgentConfig) async throws {
        // Swift-C++ interop seguro
    }
}
```

## Tácticas de Interoperabilidad

### 1. Zero-Copy Shared Memory
```cpp
// Rust y C++ comparten datos sin copia
// Ambos acceden al mismo mmap buffer
```

### 2. Cross-Language LTO
```bash
# CMake habilita LTO global vía LLVM
# Inlinea llamadas FFI en rutas críticas
```

### 3. DTrace Boundary Probes (macOS)
```bash
# Instrumentación automática en puntos de transición
sudo dtrace -P 'pid$target:::entry' -p <core_pid>
```

### 4. DWARF-Linker Unificado
```bash
# Simbolización global: Assembly → TypeScript
# Maps 0x4A2B10... → MyFunction:123
```

## Verificación de Producción

### Checklist Pre-Deployment

- [ ] Todos los módulos compilados correctamente
- [ ] ABB inicializado en `/dev/shm/interlock_abb_prod`
- [ ] IPC sockets creados en `/tmp/interlock_ipc_*`
- [ ] Configuración de producción cargada desde `backend/config/production.toml`
- [ ] Logs con nivel "warning" (mínimo noise)
- [ ] Base de datos conectada y replicada
- [ ] Redis cache sincronizado
- [ ] Health checks pasando

### Monitoreo Continuo

```bash
# 1. Logs en tiempo real
tail -f /var/log/interlock/*.log

# 2. Métricas de ABB
./tools/inspect_abb.sh

# 3. Estado de IPC
lsof | grep interlock_ipc

# 4. Performance
top -p $(cat /var/run/interlock/interlock_core.pid)
```

## Troubleshooting

### Build falla en CMake
```bash
# Borrar cache
rm -rf build/cmake/CMakeCache.txt

# Reintentar
./build.sh
```

### IPC Timeout
```bash
# Verificar que los sockets existan
ls -la /tmp/interlock_ipc_*

# Si no existen, el servidor no inició
journalctl -u interlock-core -n 50
```

### ABB no accesible
```bash
# Verificar permisos de /dev/shm
ls -la /dev/shm/interlock_abb_prod

# Limpiar y reiniciar
rm /dev/shm/interlock_abb_prod
./deploy.sh restart
```

## Seguridad en Producción

- ✅ OLLVM Obfuscation habilitada (IR-level)
- ✅ ABB con persistencia mmap (auditabilidad)
- ✅ IPC con timeouts (DoS protection)
- ✅ Anti-forensics: secure delete en shutdown
- ✅ Memory safety: Rust para cipher, Swift para middleware
- ✅ Formal verification: Coq para algoritmos críticos

## Performance (M4 Pro)

| Métrica | Valor |
|---------|-------|
| Latencia IPC | <100µs |
| Throughput ABB | 1M writes/sec |
| Agents concurrentes | 5000+ |
| Context length (RDT) | 8192 tokens |
| GPU memory (Metal MPS) | ~14GB |
| Startup time | ~3s |

## Documentación Adicional

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - Diseño de alto nivel
- [HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md) - Configuración de hardware
- [AGENT_PROTOCOLS.md](docs/AGENT_PROTOCOLS.md) - Protocolos de agentes
- [ROADMAP.md](docs/ROADMAP.md) - Hoja de ruta futura

## Licencia

Privada - Interlock Assessment

## Soporte

Para issues:
1. Revisar logs en `/var/log/interlock/`
2. Ejecutar `./deploy.sh status`
3. Inspeccionar ABB: `hexdump -C /dev/shm/interlock_abb_prod`
4. Recompila si es necesario: `./build.sh clean && ./build.sh`

---

**Última actualización**: 30 de mayo de 2026
**Versión**: 1.0.0 (Production)
