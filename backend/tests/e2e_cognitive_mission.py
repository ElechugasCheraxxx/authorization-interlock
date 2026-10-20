import sys
import os
import time
import json

# Add build directory to path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'build'))

try:
    import interlock_core_bind as core
    print("[E2E] Successfully imported interlock_core_bind")
except ImportError as e:
    print(f"[E2E] ERROR: Failed to import interlock_core_bind: {e}")
    sys.exit(1)

def run_mission():
    print("--- Starting End-to-End Zero-Latency Cognitive Mission ---")

    # 1. Hardware & Engine Initialization
    gpu = core.GPUAllocator.get_instance()
    gpu.update_hardware_stats()
    metrics = gpu.get_metrics()
    print(f"[Core] Hardware: M4 Pro | VRAM Free: {metrics.vram_free_gb:.2f}GB | Temp: {metrics.temperature_c}°C")

    inference = core.InferenceEngine.get_instance()
    inference.initialize()

    # 2. Setup Agent & Operation Context
    bb = core.Blackboard.get_instance()
    bb.clear()

    agent = core.AgentState()
    agent.alias = "SPECTRE"
    agent.status = "MISSION_ACTIVE"
    agent.current_model = "LLaMA-3-70B-RDT"
    bb.update_agent_state(agent)

    # Simulate Operation metadata
    bb.kv_set("operation_id", json.dumps("OP-CHRONOS-72"))
    bb.kv_set("mission_objective", json.dumps("Exfiltrate R&D data from internal segment without EDR triggering."))

    # 3. Trigger Cognitive Loop (Hypothesis Generation)
    print("[Cognitive] Triggering Hypothesis Engine for mission context...")
    hyp_engine = core.HypothesisEngine.get_instance()

    context = "Network scan revealed an isolated workstation 10.0.5.42 with open port 445 but high EDR monitoring. No lateral movement detected yet."
    hyp_engine.generate_hypotheses(context)

    # Wait for async simulated "reasoning"
    time.sleep(1)

    # 4. Validate Results in Blackboard
    trees = bb.get_all_hypothesis_trees()
    print(f"[Blackboard] Found {len(trees)} hypothesis trees.")

    for vector_id, tree in trees.items():
        print(f"  Tree ID: {vector_id}")
        for node_id, node in tree.nodes.items():
            print(f"    - [{node_id}] Confidence: {node.confidence:.2f} | Content: {node.content}")

    # 5. Test Maintenance Loop & Journaling (Simulated)
    print("[Maintenance] Testing event journaling...")
    ev = core.Event()
    ev.id = "EV-999"
    ev.type = "LATERAL_MOVEMENT_SUCCESS"
    ev.source = "SPECTRE"

    # This will likely fail to connect to DB if not running, falling back to journal
    # Note: we don't have a direct persist_event binding in Python yet,
    # but we can test if components trigger it.

    # 6. Check GPU/RDT Sizing
    rdt_factor = gpu.calculate_optimal_rdt_factor()
    print(f"[RDT] Optimal Reasoning Depth Factor: {rdt_factor}x (Base 12x depth)")

    print("--- E2E Mission Completed ---")

if __name__ == "__main__":
    run_mission()
