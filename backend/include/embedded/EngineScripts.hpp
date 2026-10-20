#pragma once
#include <string_view>
#include <map>

namespace interlock::embedded {

inline constexpr std::string_view MESSAGE_ROUTER_LOGIC = R"python(
from __future__ import annotations
import asyncio
import logging
import interlock_core_bind as core
from blackboard.schema import Message, MessagePriority

_log = logging.getLogger(__name__)

class MessageRouter:
    """
    Python Proxy for C++ MessageRouter.
    Ensures zero-latency routing via the 512-bit M4 bus.
    """
    def __init__(self, op_id: str):
        self._op_id = op_id
        self._cpp = core.MessageRouter.get_instance()

    async def route(self, msg: Message):
        # Delegate physical routing to C++
        self._cpp.route(msg.model_dump_json())

    async def start(self):
        _log.info("MessageRouter (C++ Backed) Online")

    async def stop(self):
        pass
)python";

inline const std::map<std::string_view, std::string_view> ENGINE_MODULES = {
    {"engine.message_router", MESSAGE_ROUTER_LOGIC}
};

} // namespace interlock::embedded
