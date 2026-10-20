<div align="center">

# Authorization Interlock

### AI-Native Orchestration Platform with Formally Verified Authorization Gates

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C++-23-5.4%2B%20Metal%20MPS-orange.svg?)](#)
[![Coq](https://img.shields.io/badge/formal%20verification-Coq%208.20%20%7C%20Isabelle%2FHOL%202024-purple.svg?)](#)

**Polyglot by design · Formal verification · Local inference on Apple Silicon**

[Module map](#module-map) · [Why formal verification](#formal-verification) · [Architecture](#architecture) · [Build](#build--deployment) · [License](#license)

</div>

> ⚠️ **Scope of use**
> This platform performs **authorized** security assessment work only. It
> operates exclusively against systems for which the target system owner
> has given explicit, written authorization.
> Use outside a contracted engagement is illegal in most jurisdictions and
> is outside the purpose of this project. See [`SECURITY.md`](SECURITY.md).

---

## What it is

Authorization Interlock is a **multi-agent orchestration platform** for
authorized security assessment engagements.

What separates it from other tools in the category is **formal
verification**: the engagement lifecycle is defined as a state machine in
Coq and Isabelle/HOL, and it is **mathematically impossible** for the
system to reach an active-testing phase without first completing every
authorization phase. This is not a runtime check — it is a proof.

On top of that runs an agent hierarchy driven by local language models
(no cloud egress), orchestrated on Apple Silicon.

**Operating prerequisites:**
- Signed services contract with the client
- Statement of Work (SoW) with an explicit scope
- Written rules of engagement and legal authorization from the system owner
- Approval from the client's security lead (CISO / security operations)
- Engagement registered in EvidenceVault before any work begins

The name reflects the design intent: an *interlock* is the industrial
safety device that makes a machine physically incapable of running until
every guard is in place. This platform applies the same principle to
security assessment work.

---

## Module map

| Module | Language | Role |
|---|---|---|
| `core-cpp/` | C++23 · Metal | Inference engine, agents, IPC |
| `cipher-rs/` | Rust | ML-KEM-768 · ML-DSA-65 · FIPS 203/204 |
| `chrx-compiler/` | Rust | Artifact compiler |
| `proofs/` | Coq · Isabelle/HOL | State machine and invariants |
| `transfer-zig/` | Zig | Sandboxed artifact transfer |
| `atlas-go/` | Go · gRPC | Agent orchestration |
| `shield-c/` · `stubs-asm/` | C · ASM | Process integrity |
| `shield-win/` | KMDF | Process integrity (Windows) |
| `artifact-pl/` | Perl | Artifact processing |
| `ruby-dispatcher/` | Ruby (mruby) | Task DSL |
| `discovery-ts/` | Bun · TypeScript | Asset discovery |
| `frontend-qt/` | Qt6 · QML | Operations console |

---

## Architecture

### Data flow

```
Qt6 QML Frontend
    └──► NativeBridge (C++23)
             └──► InterlockCore
                      ├── Path A (Fast Path): C++23 tuned for M4 Pro
                      └── Path B (Trusted Core): Certified logic
                               Coq/Isabelle → OCaml/.dylib → TrustedCoreFFI
```

### Certified dual-path execution

The system runs **two parallel implementations** of all critical logic:

- **Path A**: a high-performance C++23 implementation.
- **Path B**: code extracted directly from the Coq/Isabelle proofs,
  compiled to a native `.dylib` and integrated through `TrustedCoreFFI`.

Both paths validate each other at runtime. On any divergence the system
panics immediately (`0xDEAD`) and aborts in a controlled way. This
guarantees that production behaviour is **mathematically identical** to
the formally certified behaviour.

### Inter-module communication

- **Unix Domain Sockets (UDS)** + **FlatBuffers** for runtime isolation
  (Go / Bun / Perl / Ruby).
- **gRPC over TCP** for distributed agents on the engagement network.
- **5 s timeout** on every channel, preventing deadlocks and DoS.

### Autonomous Breadcrumb Buffer (ABB)

A lock-free circular buffer backed by `mmap` in `/dev/shm`. It records
**every inter-module transition** with nanosecond timestamps, source and
destination IDs, function pointer and stack depth.

Purpose: complete post-mortem reconstruction of the execution flow for
audit and forensic analysis of the platform itself.

---

## Formal verification

> **This is the fundamental differentiator against any other tool in the
> assessment category.**

### Engagement state machine (Isabelle/HOL)

The lifecycle of an operation is defined formally:

```
INIT → AUTHORIZATION → PLANNING → DISCOVERY → ANALYSIS
    → VALIDATION → VERIFICATION → REPORTING → CLEANUP → CLOSED
```

**Property proved mathematically:**

```isabelle
theorem no_skip_to_validation:
  "is_legal_transition AUTHORIZATION VALIDATION = False"
  by simp
```

It is **impossible by construction** to reach an active-testing phase
without completing the preceding authorization phases. This is not
software validation — it is a mathematical proof.

### Additional certified invariants

| Invariant | Description |
|------------|-------------|
| PDG Acyclicity | The dependency graph is always a DAG — no infinite loops |
| Message Router Safety | Agents only communicate with authorized peers |
| Artifact Integrity | Every processed artifact has a verified non-zero hash |
| ABB Linearizability | The audit buffer stays consistent under concurrency |

### Runtime cross-check

At every phase transition the dual-path oracle validates that both the
C++ and the OCaml/Coq paths produce the same result. On divergence the
system aborts. This means **a bug in the C++ implementation cannot
silently suppress a violation of the formal invariants**.

---

## Agent hierarchy (4 levels)

The system runs under a command hierarchy designed to guarantee human
oversight at every critical decision point.

### Level 1 — Strategic direction

**Cipher (Orchestration Agent)**
- Role: global strategy, operational risk monitoring, kill switch
- Authority: may pause or terminate any operation on scope deviation
- Model: DeepSeek-R1-32B (Q4_K_M)
- VRAM allocated: ~18.2 GB

**Chronos (Temporal Analyst)**
- Role: operation TTL management and temporal coherence
- Model: DeepSeek-R1-14B (Q4_K_M)

### Level 2 — Audit and validation

**Overseer (Operations Director)**
- Role: final mission validation, supervision of the dual-path oracle
- Bound to `FormalBridge` for real-time Coq invariant validation before
  approving any significant action
- Model: DeepSeek-R1-14B (Q4_K_M)

**Grader (Logic Validator)**
- Role: internal logic audit, reasoning loop
- Model: DeepSeek-R1-14B (Q4_K_M)

### Level 3 — Specialized supervisors

**Apex (Technical Validation)**
- Role: deep technical validation and vulnerability analysis
- Model: WhiteRabbitNeo-V3-7B (security-specialized)

**Vantage (Intelligence)**
- Role: asset discovery and semantic correlation
- Model: Qwen2.5-7B-Instruct (Q4_K_M)

**Beacon (Operations Monitor)**
- Role: operational status and situation reports
- Model: Phi-3.5-Mini (Q4)

### Level 4 — Specialized execution

| Agent | Specialization | Model |
|--------|----------------|--------|
| Coordinator | Task coordination | — |
| Executor | Authorized validation steps | — |
| Minimizer | Operational footprint reduction | Llama-3.1-8B (Q4_K_M) |
| Infrastructure | Infrastructure and directory services | DeepSeek-Coder-V2-Lite |
| Reporter | Forensics and reporting | Mistral-7B-Instruct |

---

## Inference engine (M4 Pro native)

### Engine components

**Recurrent-Depth Transformer (RDT)**
- Recurrent block with Parcae injection for deep reasoning
- No additional parameter scaling

**Adaptive Computation Time (ACT)**
- Dynamic halting based on task complexity
- Optimizes compute cycles in complex scenarios (race conditions,
  multi-hop logic)

**Multi-Latent Attention (MLA)**
- KV-cache compression for repository analysis beyond 100k lines

**Speculative Decoding**
- Draft model (Qwen-0.5B) targeting sub-10 ms latency

**Grammar-Constrained Outputs (GBNF)**
- Every agent emits output under a specific formal BNF grammar
- Malformed output is impossible — the constraint operates at the
  sampler level, not as post-hoc validation

### VRAM management (48 GB unified memory)

```
Pool STRATEGIC   ~18.2 GB  → Cipher (DeepSeek-R1-32B)
Pool VALIDATION   ~8.9 GB  → Overseer, Grader, Chronos
Pool EXECUTION    ~9.4 GB  → Apex, Coordinator, Executor, Minimizer
Pool INTEL        ~4.5 GB  → Vantage, Reporter
```

Automatic LRU eviction under memory pressure. Less-used models are
unloaded to make room for those the active operation requires.

---

## Data infrastructure and chain of custody

### The Blackboard (intelligence bus)

An asynchronous system built on **Redis Streams** with an event-sourcing
architecture. Every system event is immutable and auditable.

### EvidenceVault (chain of custody)

- SHA-256 signature of every artifact at ingest time
- ISO8601 timestamps at microsecond precision
- Side-by-side triage in the Qt6 frontend
- **Every finding is traceable back to the agent that produced it and
  the action that produced it**

### Primitive Dependency Graph (PDG)

A dependency graph over assessment primitives. It models
"zero-error" chains where logical sequencing can lead to full
compromise of a target. Acyclic by formal verification.

---

## Process integrity and anti-tampering

> **Purpose**: guarantee that the platform cannot be analysed, copied or
> reused outside an authorized context, and that its audit trail cannot
> be silently disabled. These protections exist to protect the
> platform's intellectual property and access control.

### Runtime memory protection

- Encrypted heap for sensitive data (keys, tokens, credentials)
- `mlock()` + guard pages: critical pages pinned in RAM, never swapped
  to disk
- Hardware-derived runtime keys: keys derived from authorized hardware;
  a memory dump taken on another machine yields unreadable data
- Memory integrity monitoring every 500 ms

### Binary and process integrity

- SHA3-256 verification of the `.text` section at startup
- Tamper detection on the running process, with controlled abort
- External debugger attachment is refused, so the audit trail cannot be
  altered from outside the process

---

## Attestation and licensing (zero trust)

```
Principle: Never Trust, Always Verify
```

The platform does not run without cryptographic verification of the
authorized hardware. On every start:

```
1. verifyBinaryIntegrity()     → SHA3-256 hash of the .text section
2. validateAttestationSignature() → ML-DSA-65 signature on the .lic
3. matchHardwareFingerprint()  → Serial + UUID + MAC + Board-ID
4. checkEngagementExpiry()     → TTL bound to the contract
5. setAllowedPhases()          → Phases available under the licence
```

Any failure means the platform fails closed and refuses to start.

### Hardware fingerprint

```rust
struct HardwareFingerprint {
    serial:        IOPlatformSerialNumber,
    platform_uuid: IOPlatformUUID,
    mac_en0:       Primary interface MAC,
    board_id:      EFI board-id,
    cpu_threads:   Thread count,
}
// Hash:   SHA3-256(serialized fingerprint)
// Signature: ML-DSA-65 (NIST FIPS 204)
```

### Licence server (zero trust, internal network)

- Mutual mTLS: client and server verify each other
- Certificate pinning: the server certificate is embedded in the binary
- Single-use nonces preventing replay
- Periodic verification before each engagement phase
- Immediate revocation available from the internal server

---

## Behavioural risk scoring and containment

The system monitors its own behaviour in real time. Any agent can report
a deviation with a defined impact:

```cpp
void reportDeviation(agent, description, impact_1_to_20);
// Accumulated score ≥ 1.0 → triggerContainment()
```

The main loop checks the score every second. At CRITICAL level the
system shuts down immediately and preserves the audit trail.

**This guarantees the platform limits itself if it detects it is
operating outside expected parameters.**

---

## Post-quantum cryptography

All inter-agent channels and the licensing system use cryptography
resistant to quantum computation:

| Algorithm | Standard | Use |
|-----------|---------|-----|
| ML-KEM-768 | NIST FIPS 203 | Key encapsulation (Kyber) |
| ML-DSA-65 | NIST FIPS 204 | Digital signatures (Dilithium) |
| Hybrid RSA-4096 | Transitional | Backward compatibility |

Implemented in Rust (`cipher-rs`) for memory-safety guarantees.

---

## Build & deployment

```bash
# Full build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)

# Run the platform
open build/bin/AuthorizationInterlock_Qt.app

# Verify the production bundle
./verify_production_bundle.sh
```

**Hardware requirements:**
- Apple Silicon M4 Pro or higher
- 48 GB unified memory (36 GB minimum)
- macOS Tahoe 26.x

**External dependencies** (not vendored in the repo, ~77 MB):
[llama.cpp](https://github.com/ggml-org/llama.cpp) via
`backend/core-cpp/llama_sourcecode/` — instructions in that folder's
README.

---

## Typical engagement

**Reference scenario:**

A client operating corporate infrastructure suspects weaknesses in its
network implementation and commissions an authorized assessment
engagement with the following elements:

- A signed SoW defining the exact scope of the target network
- Authorization from the client's CISO / security operations lead
- A specific objective: determine whether network segments that should
  be isolated (mock/simulated financial data used as evidence of access)
  can be reached
- A defined time window for the operation

**Role of the platform:**

1. Cipher receives the engagement brief and generates the initial PDG
2. Vantage performs passive discovery within the defined scope
3. Overseer validates every proposed action against the formal invariants
4. Executor performs authorized validation steps against in-scope
   targets only
5. Reporter documents every finding with a complete chain of custody
6. ReportForge generates the executive and technical report at the end

**Scope guarantee:** the formally verified state machine makes it
impossible for the system to take active-testing actions without first
completing the authorization phases. Every action is recorded in the ABB
with a nanosecond timestamp.

---

## Ownership and responsible use

This software is the exclusive property of **José Miguel Tirado Quiroz**.
It is designed for the automation of advanced authorized assessment
operations under contract.

**Technical use restrictions:**
- Cryptographic binding to the authorized operator's hardware
- Does not run on hardware not registered with the licensing system
- All operations are recorded with a chain of custody
- The system limits itself if it detects operation outside scope

**Legal framework required for every engagement:**
- Services contract with the target client
- Documented and signed scope
- Authorization from the client's security lead
- Written rules of engagement or the jurisdictional equivalent

---

*Last updated: v8.0 — Authorization Interlock*

## License

MIT — see [`LICENSE`](LICENSE).

The software is distributed for use in authorized security assessments.
The author accepts no responsibility for unauthorized use.

---

<div align="center">

**José Miguel Tirado Quiroz** · Security & Infrastructure Engineer · Lima, Peru

[![LinkedIn](https://img.shields.io/badge/LinkedIn-0A66C2?style=flat-square&logo=linkedin&logoColor=white)](https://linkedin.com/in/jose-miguel-tirado-quiroz)
[![GitHub](https://img.shields.io/badge/GitHub-ElechugasInterlock-181717?style=flat-square&logo=github&logoColor=white)](https://github.com/ElechugasInterlock)

</div>
