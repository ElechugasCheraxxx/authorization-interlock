#pragma once
#include <string_view>
#include <map>

namespace interlock::embedded {

inline constexpr std::string_view AGENT_BASE = R"python(
from __future__ import annotations
import asyncio
import logging
import interlock_core_bind as core
from blackboard.blackboard import Blackboard
from blackboard.schema import AgentState, AgentStatus, Message, MessageType

_log = logging.getLogger(__name__)

class BaseAgent:
    """
    M4 Pro Optimized Base Agent.
    Bridges Python autonomy with C++ resource management.
    """
    def __init__(self, alias: str, department: str, blackboard: Blackboard):
        self.alias = alias
        self.department = department
        self._bb = blackboard
        self._gpu = core.GPUAllocator.get_instance()

    async def _set_status(self, status: AgentStatus, task: str = ""):
        # Physical sync with C++ memory
        self._bb._cpp.update_agent_state(AgentState(
            alias=self.alias,
            status=status,
            current_model="embedded-core-v1"
        ))

    async def auto_scale_logic(self):
        """Adjusts RDT Logic Depth based on M4 Pro thermals."""
        rdt_factor = self._gpu.calculate_optimal_rdt_factor()
        # Notify local MLX adapter if applicable or sync via C++
        return rdt_factor
)python";

inline constexpr std::string_view AGENT_REX = R"python(
from blackboard.schema import Message, MessageType
from agents.base import BaseAgent

class Coordinator(BaseAgent):
    """Exploitation & Payloads Orchestrator."""
    async def _on_message(self, msg: Message):
        if msg.type == MessageType.TASK:
            # Lógica de explotación embebida
            pass
)python";

inline const std::map<std::string_view, std::string_view> AGENT_MODULES = {
    {"agents.base", AGENT_BASE},
    {"agents.orchestrators.rex", AGENT_REX}
};

} // namespace interlock::embedded
