import sys
import os

# Añadir el directorio build para encontrar el binario compilado
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'build'))

try:
    import interlock_core_bind as core
    print("Successfully imported interlock_core_bind")
except ImportError as e:
    print(f"Failed to import interlock_core_bind: {e}")
    sys.exit(1)

def test_blackboard():
    bb = core.Blackboard.get_instance()

    # Test Agent State
    state = core.AgentState()
    state.alias = "cipher"
    state.status = "OPERATIONAL"
    state.vram_used_gb = 1.2
    bb.update_agent_state(state)

    retrieved = bb.get_agent_state("cipher")
    print(f"Agent State: {retrieved.alias}, Status: {retrieved.status}, VRAM: {retrieved.vram_used_gb}GB")
    assert retrieved.alias == "cipher"

    # Test Message
    msg = core.Message()
    msg.msg_id = "M-101"
    msg.from_agent = "cipher"
    msg.to = "apex"
    msg.content = "Test content"
    msg.priority = core.MessagePriority.HIGH

    bb.push_message(msg)
    tail = bb.get_message_tail(1)
    print(f"Message Tail Size: {len(tail)}")
    if len(tail) > 0:
        print(f"Last Message: {tail[0].content} (Priority: {tail[0].priority})")

def test_router():
    router = core.MessageRouter.get_instance()
    router.start()

    msg = core.Message()
    msg.msg_id = "M-202"
    msg.from_agent = "apex"
    msg.to = "cipher"
    msg.content = "Routed message"

    router.route(msg)
    print("Message routed")

    import time
    time.sleep(0.1) # Breve espera para el despacho

    bb = core.Blackboard.get_instance()
    tail = bb.get_message_tail(1)
    print(f"Blackboard tail after routing: {tail[0].content if tail else 'Empty'}")

    router.stop()

if __name__ == "__main__":
    print("--- Running Binding Tests ---")
    test_blackboard()
    test_router()
    print("--- Tests Completed Successfully ---")
