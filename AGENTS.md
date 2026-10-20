# AGENTS.md — Authorization Interlock (v8.5, M4 Pro edition)

Engineering notes for contributors and coding agents working on this
repository. The platform targets 10k+ concurrent agents with O(1)
dispatch latency on Apple Silicon.

## Core performance engineering

Critical components tuned for the M4 Pro (48 GB unified memory):

- **Math compression** — `CompressedVector` and `CompressedQuaternion`
  (in-house implementation). Cuts network bandwidth by 50–75% by
  compressing 32-bit floats to 16-bit shorts. Certified in Coq to
  prevent precision divergence.
- **Hybrid pool allocator** — hierarchical pool allocator with a
  segmented free list. Strict **64-byte** alignment (M4 Pro cache line)
  to avoid false sharing. Uses InstanceIDs for O(1) safety and debug
  fills (`0xCD` / `0xDD`) for traceability.
- **Priority discovery streamer** — **4 parallel channels** ordered by
  **logical priority (LSN)**. Allows prioritized, asynchronous telemetry
  delivery without degrading the main thread.
- **Task hierarchy** — sub-task hierarchy for complex agent
  orchestration and deterministic sequencing.
- **ReferenceTracker** — lightweight GC guaranteeing reference safety
  and preventing dangling pointers across FFI boundaries.

## Repository shape and tech stack

- **Core engine**: `backend/core-cpp/` (C++23, RDT looped engine, GPU/MPS,
  Mach IPC, tamper detection).
- **Native frontend**: `frontend-qt/` (Qt6 / QML operations console).
- **Cryptographic core (Rust)**: `backend/cipher-rs/`
  (ML-KEM-768, ML-DSA-65, hybrid RSA-4096).
- **Agent orchestration (Go)**: `backend/atlas-go/`
  (mass orchestration, gRPC channels, 5k+ concurrent agents).
- **Process integrity (C/ASM)**: `backend/shield-c/` (macOS),
  `backend/shield-win/` (Windows).
- **Artifact transfer (Zig)**: `backend/transfer-zig/`
  (sandboxed transfer, Mach VM remapping).
- **Artifact processing (Perl)**: `backend/artifact-pl/`
  (high-speed log and artifact processing via Mojo-Async).
- **Asset discovery (Bun/TS)**: `backend/discovery-ts/`
  (browser automation and asset correlation).
- **Task DSL compiler (Rust)**: `backend/chrx-compiler/`
  (`.ail` DSL, Rust-based compiler, formally isomorphic syntax).
- **Task dispatcher (Ruby)**: `backend/ruby-dispatcher/`
  (mruby DSL and project generation).
- **Formal verification (dual engine)**: `backend/proofs/`.
  - **Coq 8.20** — certification of `MathValidator` (fixed-point
    determinism) and cryptographic protocols.
  - **Isabelle/HOL 2024** — verification of `StateMachine`, `PDG`, and
    memory safety (`PoolAllocator` invariants).
  - **Extraction** — OCaml to native `.dylib`, integrated through
    `FormalBridge.cpp`.

## Architecture

1. **Direct bridge** — Qt6 QML → `NativeBridge` (C++23) → core engine.
2. **Certified dual-path oracle**:
   - **Path A (fast path)** — C++23 implementation tuned for extreme
     performance on M4 Pro.
   - **Path B (trusted core)** — native execution of the certified
     logic. Isabelle acts as the oracle for **symbolic logic**
     (state machine / PDG) and **concurrency**, while Coq assures
     **cryptographic integrity**.
   - **Cross-check** — runtime validation of the formal invariants:
     - **PDG acyclicity** — mathematical guarantee of no deadlock in
       dependency chains.
     - **Transition legality** — critical phases are unreachable
       without their preconditions.
     - **ABB linearizability** — absolute consistency of the lock-free
       buffer in multi-threaded environments.
     - **Artifact integrity** — no-corruption verification during
       artifact extraction.
   - **Runtime initialization** — the OCaml GC is started in `main.cpp`
     via `init_ocaml_runtime(argv)` before any critical operation.
3. **Polyglot hub (IPC)** — Unix Domain Sockets and FlatBuffers for
   runtime isolation (Go / Bun / Perl / Ruby).
4. **Memory guard (PQC armour)**:
   - **Control-flow flattening** in C++23.
   - **Hybrid PQC** — ML-KEM-768 + RSA-4096 for key encapsulation.
   - **IR obfuscation** — Obscura-LLVM 19.1.5 to collapse control graphs.
5. **Audit and telemetry**:
   - **ABB (Autonomous Breadcrumb Buffer)** — lock-free circular buffer,
     64-byte aligned, with Jenkins integrity (`0x04c11db7`) per entry.
   - **Shadow stack telemetry** — parallel logical stack for real-time
     flow auditing on ASM crashes.

## Task DSL (`.ail`)

**Purpose.** In a polyglot ecosystem spanning a dozen languages, the
critical risk is loss of traceability, data races and memory leaks at FFI
boundaries (C++ ↔ Rust ↔ Go ↔ Zig). Each language excels in isolation,
but none holds the global context needed to keep a multi-runtime
operation free of artefacts.

The `.ail` DSL operates as a **cross-runtime choreography language**. Its
sole function is to coordinate atomic state transitions, manage the
Autonomous Breadcrumb Buffer, and enforce deterministic audit policy
across runtimes without ambiguity.

**Build-system integration.** The `.ail` compiler does not execute code
at runtime; it runs during the compilation pipeline to generate the
low-level glue:

1. **Cross-cutting RAII management** — generates automatic RAII wrappers
   in C++ and `unsafe` bridges in Rust to destroy opaque pointers with
   absolute temporal precision.
2. **FlatBuffers generation** — converts buffer definitions into
   latency-optimized IPC schemas.
3. **Formal mathematical validation** — extracts audit assertions and
   generates Coq/Isabelle specifications proving the absence of deadlocks
   or data exposure before deployment.

## Agent hierarchy

The swarm's internal language is `.ail`. Every agent generates, validates
and consumes `.ail` blocks to guarantee mathematical coherence and full
auditability.

### Level 1 — Strategic direction

- **Cipher (orchestration agent)** — the swarm's brain. Thinks and emits
  high-abstraction `.ail` directives. Defines the engagement
  "constitution" and the PDG.
  - *Native thought*: generates `directive_core.ail` (PQC-signed).
- **Chronos (temporal analyst)** — modulates `@temporal_drift` and
  `@jitter` parameters in `.ail` flows to keep operation timing coherent.

### Level 2 — Dispatch and verification

- **Overseer (compiler)** — compiles and validates the swarm's intent.
  Checks every `.ail` block against `FormalBridge` (Isabelle/Coq) before
  execution.
- **Grader** — semantic audit of received `.ail` reports, comparing
  original intent against obtained result.

### Level 3 — Supervisors

- **Apex (technical lead)** — transpiles technical objectives into
  operational `.ail` blocks.
- **Vantage** — translates external intelligence into data structures
  natively processable by `.ail`.
- **Beacon** — centralizes communications, aggregating system state into
  unified `.ail` situation reports.

### Level 4 — Specialized execution

- **Coordinator / Executor** — operational executors. Interpret and run
  `.ail` `task` blocks directly on authorized target hardware.
- **Minimizer** — uses `.ail` introspection to dynamically adjust its
  operational footprint and memory parameters.
- **Infrastructure** — manages persistence and transport of `.ail`
  messages across the swarm's infrastructure.

## Data and evidence infrastructure

- **The Blackboard (intelligence bus)** — asynchronous system on Redis
  Streams with an event-sourcing architecture.
- **EvidenceVault (chain of custody)** — SHA-256 signed storage with
  microsecond ISO8601 synchronization. Includes high-efficiency
  side-by-side triage in the Qt6 frontend.
- **Primitive Dependency Graph (PDG)** — dependency graph over
  assessment primitives, modelling zero-error chains. Acyclic by formal
  verification.

## Inference engine (RDT looped)

- **Neural evolution v12.0** — autonomous evolution engine with
  multi-hop reasoning in latent space.
- **RDT reasoning engine**:
  - **Recurrent-Depth Transformer** — recurrent block with Parcae
    injection for deep reasoning without parameter scaling.
  - **ACT (Adaptive Computation Time)** — dynamic halving to optimize
    reasoning cycles by complexity.
  - **MLA (Multi-Latent Attention)** — KV-cache compression for
    repository analysis beyond 100k lines.
  - **Speculative decoding** — draft models (Qwen-0.5B) for sub-10 ms
    latency.
- **GPU allocator and VRAM pools (48 GB unified memory)**:
  - Pool 1 `STRATEGIC` — ~18.2 GB (Cipher, DeepSeek-R1-32B).
  - Pool 2 `VALIDATION` — ~8.9 GB (Overseer, Grader, Chronos).
  - Pool 3 `EXECUTION` — ~9.4 GB (Apex, Coordinator, Executor, Minimizer).
  - Pool 4 `INTEL` — ~4.5 GB (Vantage, Reporter).
- **Metal Performance Shaders (MPS)** — extreme kernel tuning to
  maximize throughput on M4 Pro.

## Security layer

1. **Mach exception guard** — exception-port control to block external
   debuggers and prevent audit-trail tampering.
2. **Hardware breakpoint scan** — scans ARM64 `DBGWVR` registers to
   detect physical breakpoints.
3. **Jenkins integrity shield** — ultra-fast memory integrity
   verification (Jenkins hash `0x04c11db7`).
4. **Hardened gRPC** — SHA-256 integrity verification on orchestration
   messages to prevent man-in-the-middle injection.
5. **Encrypted heap** — sensitive data (keys, tokens, credentials) held
   in an encrypted heap with `mlock()` and guard pages.
6. **Hardware-derived runtime keys** — keys derived from authorized
   hardware; a memory dump on another machine yields unreadable data.

## Commands

- **Build app bundle**:
  `mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(sysctl -n hw.ncpu)`
- **Run**:
  `open build/bin/AuthorizationInterlock_Qt.app`

## Ownership and use

This software is the exclusive property of **José Miguel Tirado Quiroz**.
It automates advanced authorized assessment operations under contract.

The platform binds cryptographically to the authorized operator's
hardware and does not run on unregistered machines. All operations are
recorded with a full chain of custody, and the system limits itself if
it detects operation outside the authorized scope.
