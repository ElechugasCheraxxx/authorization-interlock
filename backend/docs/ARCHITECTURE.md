# Assessment IDE — Architecture

## Overview

Three-layer architecture: Frontend → Orchestration Engine → Model Layer.

```
Frontend (SwiftUI / Electron+React)
    │ WebSocket + REST
Orchestration Engine (Python FastAPI)
    │ ModelProxy abstraction
Model Layer (llama.cpp / MLX)
```

## Core Components

### Blackboard
Asynchronous shared state with PostgreSQL persistence. All agents read/write via async API.
No agent ever reads another agent's context directly — all sharing goes through the Blackboard.
Redis PubSub handles real-time cross-instance synchronization for horizontal scaling.

### MessageRouter
Validates, persists, and delivers all inter-agent messages.
Critical messages bypass the normal queue (FIFO) for priority delivery.
Every message is persisted to PostgreSQL before delivery — zero message loss.

### StateMachine
Enforces valid operation phase transitions: INIT → BRIEFING → RECON → ANALYSIS → EXPLOITATION → POST_OP → CLOSED.
Cipher controls phase transitions. Operator can force-transition via API.

### ModelProxy
Single interface between agents and AI models. Agents never call llama.cpp or MLX directly.
Swap adapter (MLX ↔ llama_cpp) without touching agent code.

### HeadlessController
Plan B for Cipher failure. Activates headless mode using the pre-generated Headless Constitution.
Coordinator/Atlas/Vector operate peer-to-peer until Cipher recovers or operator intervenes.

## Security Boundaries

- All traffic on localhost or authorized LAN only.
- Evidence Vault encrypted at rest (AES-256) — [REQUIRES_RUNTIME_VALIDATION: filesystem encryption].
- Message log signed with SHA-256 hashes for tamper detection.
- data/ directory never committed to git.
- Session tokens generated per operation, not reused.

## Scaling Path

Phase 1: M4 Pro — 2-3 models, 3-5 agents.
Phase 2: +Blackwell GPUs — full 14-agent team, 671B models.
No code changes required — add nodes to hardware.toml, update agents.toml paths.
