#pragma once
#include <string_view>
#include <map>

namespace interlock::embedded {

inline constexpr std::string_view BLACKBOARD_SCHEMA = R"python(
from __future__ import annotations
import datetime
import uuid
from enum import Enum
from typing import Any, Optional, Union, List, Dict
from pydantic import AliasChoices, BaseModel, ConfigDict, Field, model_validator

def _utcnow() -> datetime.datetime:
    return datetime.datetime.now(datetime.timezone.utc)

def _get_high_precision_timestamp() -> str:
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.%f") + "Z"

def _short_id(prefix: str) -> str:
    return f"{prefix}-{uuid.uuid4().hex[:12]}"

class CompatModel(BaseModel):
    model_config = ConfigDict(populate_by_name=True, extra="allow")

class MessageType(str, Enum):
    TASK = "task"
    OBSERVATION = "observation"
    HYPOTHESIS = "hypothesis"
    FINDING = "finding"
    INTEL = "intel"
    ERROR = "error"
    COMMAND = "command"
    BRIEFING = "briefing"
    CORRELATION = "correlation"
    INJECT = "inject"
    SYSTEM = "system"
    ESCALATE = "escalate"
    SITREP = "sitrep"
    RESPONSE = "response"
    PIVOT = "pivot"
    ACKNOWLEDGE = "acknowledge"
    REQUEST = "request"
    REPORT = "report"
    EMERGENCY = "emergency"
    CRITICAL_FINDING = "critical_finding"
    TECHNICAL_FAILURE = "technical_failure"

class MessagePriority(str, Enum):
    LOW = "low"
    NORMAL = "normal"
    HIGH = "high"
    CRITICAL = "critical"

class MessagePayload(CompatModel):
    content: str
    data: Dict[str, Any] = Field(default_factory=dict)
    refs: List[str] = Field(default_factory=list)
    metadata: Dict[str, Any] = Field(default_factory=dict)
    tool_calls: List[Dict[str, Any]] = Field(default_factory=list)

class Message(CompatModel):
    msg_id: str = Field(default_factory=lambda: _short_id("MSG"))
    from_agent: str
    to: str
    channel: str = "#general"
    type: MessageType = MessageType.TASK
    priority: MessagePriority = MessagePriority.NORMAL
    artifact: MessagePayload
    timestamp: datetime.datetime = Field(default_factory=_utcnow)
    ts_precise: str = Field(default_factory=_get_high_precision_timestamp)
    requires_ack: bool = False

class Severity(str, Enum):
    INFO = "info"
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"
    CRITICAL = "critical"

class Finding(CompatModel):
    id: str = Field(default_factory=lambda: _short_id("FND"))
    agent: str = "system"
    department: str = "general"
    type: str = "finding"
    title: str
    description: str
    severity: Severity
    vector: str = ""
    host: Optional[str] = None
    recommended_action: str = ""
    raw_stream: Optional[str] = None
    evidence_ids: List[str] = Field(default_factory=list)
    tags: List[str] = Field(default_factory=list)
    metadata: Dict[str, Any] = Field(default_factory=dict)
    status: str = "new"
    timestamp: datetime.datetime = Field(default_factory=_utcnow)
    ts_precise: str = Field(default_factory=_get_high_precision_timestamp)

    @model_validator(mode="after")
    def _default_vector(self) -> Finding:
        if not self.vector:
            self.vector = self.type
        return self

class AgentStatus(str, Enum):
    IDLE = "idle"
    THINKING = "thinking"
    EXECUTING = "executing"
    REPORTING = "reporting"
    DEGRADED = "degraded"
    ERROR = "error"
    OFFLINE = "offline"

class AgentState(CompatModel):
    alias: str
    status: AgentStatus
    current_task: Optional[str] = None
    last_message: Optional[str] = None
    thinking_buffer: str = ""
    model: str = ""
    department: str = ""
    gpu_node: str = "local"
    vram_used_gb: float = 0.0
    vram_usage_gb: float = 0.0
    total_tokens: int = 0
    token_quota: int = 0
    quota_usage_pct: float = 0.0
    context_usage_pct: float = 0.0
    is_suspended: bool = False
    last_seen: datetime.datetime = Field(default_factory=_utcnow)

class DepartmentHealth(str, Enum):
    GREEN = "green"
    YELLOW = "yellow"
    RED = "red"
    STANDBY = "standby"
    DEGRADED = "degraded"

class DepartmentState(CompatModel):
    name: str
    health: Union[DepartmentHealth, float] = DepartmentHealth.STANDBY
    findings_count: int = 0
    active_agents: Union[List[str], int] = Field(default_factory=list)

class OperationPhase(str, Enum):
    INIT = "init"
    BRIEFING = "briefing"
    RECON = "recon"
    ANALYSIS = "analysis"
    VULN_RESEARCH = "vuln_research"
    EXPLOITATION = "exploitation"
    POST_EXPLOITATION = "post_exploitation"
    POST_OP = "post_op"
    REPORTING = "reporting"
    CLEANUP = "cleanup"
    PAUSED = "paused"
    CLOSED = "closed"

class OperationMode(str, Enum):
    AUTONOMOUS = "autonomous"
    FULL_AUTO = "full_auto"
    SUPERVISED = "supervised"
    MANUAL = "manual"
    MANUAL_ASSIST = "manual_assist"
    UNCONSTRAINED = "unconstrained"

class NodeStatus(str, Enum):
    DISCOVERED = "discovered"
    ACTIVE = "active"
    COMPROMISED = "compromised"
    UNKNOWN = "unknown"

class MapNode(CompatModel):
    id: str = Field(default_factory=lambda: _short_id("NODE"))
    label: str
    type: str
    status: NodeStatus = NodeStatus.UNKNOWN
    discovered_by: Optional[str] = None
    metadata: Dict[str, Any] = Field(default_factory=dict)

class MapEdge(CompatModel):
    source: str
    target: str
    label: Optional[str] = None
    id: Optional[str] = None
    metadata: Dict[str, Any] = Field(default_factory=dict)

    @model_validator(mode="after")
    def _default_id(self) -> MapEdge:
        if self.id is None:
            self.id = f"{self.source}->{self.target}:{self.label or ''}"
        return self

class OperationState(CompatModel):
    op_id: str
    name: str = ""
    phase: OperationPhase = OperationPhase.INIT
    mode: OperationMode = OperationMode.SUPERVISED
    objective: str = ""
    scope: str = ""
    restrictions: str = ""
    timeline_hours: int = Field(4, gt=0)
    target_profile: str = ""
    speed_depth_ratio: float = Field(0.5, ge=0, le=1)
    risk_score: float = Field(0.0, ge=0, le=1)
    overall_status: str = "on_track"
    start_time: datetime.datetime = Field(default_factory=_utcnow)
    phase_started: datetime.datetime = Field(default_factory=_utcnow)
    ts_precise: str = Field(default_factory=_get_high_precision_timestamp)
    metadata: Dict[str, Any] = Field(default_factory=dict)

class BlackboardSnapshot(CompatModel):
    operation: OperationState
    timestamp: datetime.datetime = Field(default_factory=_utcnow)
    last_event_id: Optional[str] = None
    objectives: List[Any] = Field(default_factory=list)
    findings: List[Finding] = Field(default_factory=list)
    agent_states: Dict[str, AgentState] = Field(default_factory=dict)
    department_states: Dict[str, DepartmentState] = Field(default_factory=dict)
    kv_store: Dict[str, Any] = Field(default_factory=dict)
    message_bus_tail: List[Message] = Field(default_factory=list)
    map_nodes: List[MapNode] = Field(default_factory=list)
    map_edges: List[MapEdge] = Field(default_factory=list)
)python";

inline constexpr std::string_view BLACKBOARD_LOGIC = R"python(
from __future__ import annotations
import asyncio
import logging
import interlock_core_bind as core
from blackboard.schema import (
    OperationState, AgentState, DepartmentState, Finding,
    MapEdge, MapNode, Message, BlackboardSnapshot
)

_log = logging.getLogger(__name__)

class Blackboard:
    """
    C++23 Backed Blackboard Proxy.
    Delegates all state to the high-performance Master Core.
    """
    def __init__(self, operation: OperationState):
        self._op_id = operation.op_id
        self._cpp = core.Blackboard.get_instance()
        self._operation = operation

    async def update_agent_state(self, state: AgentState):
        # Enviar a C++ para almacenamiento en memoria unificada
        self._cpp.update_agent_state(state.alias, state.model_dump_json())

    async def get_agent_states(self) -> dict[str, AgentState]:
        raw = self._cpp.get_all_agent_states()
        return {k: AgentState.model_validate_json(v) for k, v in raw.items()}

    async def push_message(self, msg: Message):
        self._cpp.push_message(msg.model_dump_json())

    async def kv_set(self, key: str, value: Any):
        import json
        self._cpp.kv_set(key, json.dumps(value))

    async def snapshot(self) -> BlackboardSnapshot:
        # Reconstruir snapshot desde la memoria de C++
        return BlackboardSnapshot(
            operation=self._operation,
            agent_states=await self.get_agent_states(),
            # ... otros campos
        )
)python";

inline const std::map<std::string_view, std::string_view> BLACKBOARD_MODULES = {
    {"blackboard.schema", BLACKBOARD_SCHEMA},
    {"blackboard.blackboard", BLACKBOARD_LOGIC}
};

} // namespace interlock::embedded
