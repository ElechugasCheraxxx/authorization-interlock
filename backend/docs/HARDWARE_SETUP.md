# Hardware Setup Guide

## Phase 1: M4 Pro (Development & Initial Operations)

### Prerequisites
- macOS 14.0+ (Sonoma)
- Xcode 15+
- Python 3.12+
- Node.js 20+
- Homebrew

### Model Server Setup (mlx-lm)
```bash
pip install mlx-lm
mlx_lm.server --model mlx-community/DeepSeek-R1-Distill-Qwen-7B-4bit --port 8080
```

### Multiple Model Servers
Run one server per port. Configure ports in config/agents.toml.
M4 Pro (36GB) can run 2-3 7B models simultaneously (~6GB each + KV cache).

## Phase 2: Blackwell GPU Nodes

[REQUIRES_RUNTIME_VALIDATION: actual-blackwell-hardware]

### llama.cpp Server (CUDA)
```bash
./llama-server --model models/deepseek-r1-70b.Q4_K_M.gguf \
  --ctx-size 8192 --n-gpu-layers 999 --port 8080 --host 0.0.0.0
```

### Network Configuration
All GPU nodes on the same LAN. Configure IPs in config/hardware.toml.
The Orchestration Engine on the M4 Pro manages all remote nodes via REST.

## VRAM Planning

| Model         | Quant  | VRAM    | Agent       |
|---------------|--------|---------|-------------|
| R1 70B        | Q4_K_M | ~40GB   | Coordinator/Cipher  |
| Llama4 Scout  | Q4     | ~22GB   | Atlas/Probe |
| Qwen 32B      | Q4     | ~18GB   | Vector/Mirage|
| R1 14B        | Q5     | ~10GB   | Spectre/Archive|
| 7B models     | Q4_K_M | ~5-6GB  | Echo/proxies|
