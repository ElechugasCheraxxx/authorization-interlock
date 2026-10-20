use serde::{Deserialize, Serialize};
use bitflags::bitflags;

bitflags! {
    #[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
    pub struct AgentFlags: u32 {
        const STEALTH_MODE = 0b00000001;
        const AGGRESSIVE   = 0b00000010;
        const PQC_SIGNED   = 0b00000100;
        const VERIFIED     = 0b00001000;
        const IS_SWARM_LEADER = 0b00010000;
    }
}

impl serde::Serialize for AgentFlags {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_u32(self.bits())
    }
}

impl<'de> serde::Deserialize<'de> for AgentFlags {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let bits = u32::deserialize(deserializer)?;
        AgentFlags::from_bits(bits).ok_or_else(|| serde::de::Error::custom("Invalid AgentFlags"))
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ChrxxxMessage {
    pub version: u32,
    pub timestamp: u64,
    pub sender_id: [u8; 32],
    pub flags: AgentFlags,
    pub artifact: ChrxxxPayload,
    pub signature: Option<Vec<u8>>, // ML-DSA-65 signature
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum ChrxxxPayload {
    Directive(String),       // Raw .chrxxx directive
    Task(TaskDefinition),    // Specific compiled task
    Telemetry(TelemetryData), // Agent status/results
    Handshake { public_key: Vec<u8> },
    KillSwitch { auth_token: [u8; 32] },
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct TaskDefinition {
    pub task_id: u32,
    pub primitive: String,
    pub arguments: Vec<String>,
    pub memory_alignment: u32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct TelemetryData {
    pub success: bool,
    pub logs: Vec<String>,
    pub metrics: std::collections::HashMap<String, f64>,
}
