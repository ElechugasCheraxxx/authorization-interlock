# Development Roadmap

## Phase 0 — Backend Base (Weeks 1-2)
- [x] SQLite + in-memory Blackboard
- [x] FastAPI + WebSocket
- [x] MessageRouter + StateMachine
- [ ] mlx-lm server running stable on M4 Pro
- [ ] TEST: Cipher → Coordinator → Minimizer message round-trip

## Phase 1 — Orchestration Complete (Weeks 3-5)
- [x] ContextManager + HealthMonitor + HeadlessController
- [x] All agents defined
- [x] Headless Constitution generated at session start
- [x] Operation Brief API (backend)
- [ ] Full pipeline: Cipher → Coordinator → Minimizer (without UI, CLI test)

## Phase 2 — Full Communication (Weeks 6-8)
- [ ] All agents active and messaging
- [ ] P2P between agents working
- [ ] GlobalMapView logic in backend
- [ ] EvidenceVault auto-indexing

## Phase 3 — Mac Lite UI (Weeks 9-14)
- [ ] SwiftUI War Room layout
- [ ] Cipher Command Center
- [ ] Department Panels
- [ ] Communication Feed
- [ ] Agent Workspaces
- [ ] Blackboard LiveView
- [ ] GlobalMapView (WKWebView + Cytoscape.js)

## Phase 4 — Polish & Launch (Weeks 15-18)
- [ ] Report Forge + Replay Mode
- [ ] Persona Manager
- [ ] Design System coherent throughout
- [ ] LAUNCH: First HackerOne Bug Bounty operation

## Phase 5 — Scale (Weeks 19+)
- [ ] Fine-tune prompts with real operation data
- [ ] Scope Guard with Claude Opus
- [ ] Windows Max frontend
- [ ] Multi-Blackwell nodes
