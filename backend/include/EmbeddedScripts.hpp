#pragma once
#include <string_view>

namespace interlock::embedded {

/**
 * M4 Pro Optimized: Python code as static resources.
 * This eliminates disk I/O when loading the backend logic.
 */

inline constexpr std::string_view BLACKBOARD_PROXY = R"python(
from __future__ import annotations
import interlock_core_bind as core
import json
from datetime import datetime, timezone
from typing import Any, Optional, Dict, List

class Blackboard:
    def __init__(self, op_id: str):
        self._cpp = core.Blackboard.get_instance()
        self.op_id = op_id

    def get_agent_states(self) -> dict:
        return self._cpp.get_all_agent_states()

    def kv_set(self, key: str, value: Any):
        self._cpp.kv_set(key, json.dumps(value))
)python";

inline constexpr std::string_view MESSAGE_ROUTER_LOGIC = R"python(
import interlock_core_bind as core
import asyncio

class MessageRouter:
    """
    Python side of the high-speed router.
    Handles async event loop integration while C++ handles physical routing.
    """
    def __init__(self):
        self._cpp = core.MessageRouter.get_instance()

    async def route_async(self, msg_dict: dict):
        # Convert dict to C++ Message object via bridge
        msg = core.Message()
        msg.from_agent = msg_dict.get("from_agent", "system")
        msg.to = msg_dict.get("to", "all")
        msg.content = msg_dict.get("content", "")
        msg.type = msg_dict.get("type", "task")

        # Physical route via C++
        self._cpp.route(msg)
)python";

} // namespace interlock::embedded
