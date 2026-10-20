use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Mission {
    pub name: String,
    pub directives: Vec<Directive>,
    pub verify_block: Vec<Verification>,
    pub resources: Vec<Resource>,
    pub swarms: Vec<Swarm>,
    pub flows: Vec<Flow>,
    pub strict_limits: Vec<StrictLimit>,
    pub choreographies: Vec<Choreography>,
    pub swarm_syncs: Vec<SwarmSync>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct StrictLimit {
    pub key: String,
    pub condition: String, // e.g., "<"
    pub value: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Choreography {
    pub name: String,
    pub phases: Vec<Phase>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Phase {
    pub name: String,
    pub commands: Vec<ChrxCommand>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum ChrxCommand {
    Bind(String),
    Await(String),
    Lock(String),
    Assert(String),
    Purge(String),
    Call(String), // For generic calls like Go::Atlas.SuspendTelemetry()
    Halt(String),
    Commit(String),
    Resume(String),
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct SwarmSync {
    pub events: Vec<SyncEvent>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct SyncEvent {
    pub event_name: String,
    pub commands: Vec<ChrxCommand>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum Directive {
    Compiler {
        target: String,
        optimization: String,
        integrity: Option<String>,
        armor: Option<String>
    },
    MemoryPool { name: String, size: String },
    Alignment { bytes: u32 },
    TemporalDrift { value: String },
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum Verification {
    Invariant(String),
    Precondition(String),
    Postcondition(String),
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Resource {
    pub name: String,
    pub r_type: String,
    pub size: String,
    pub alignment: u32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Swarm {
    pub name: String,
    pub agents: Vec<Agent>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Agent {
    pub name: String,
    pub role: String,
    pub artifact: Option<String>,
    pub integrity: Option<String>,
    pub target: Option<String>,
    pub trigger: Option<String>,
    pub on_success: Option<String>,
    pub on_fail: Option<String>,
    pub tasks: Vec<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Flow {
    pub name: String,
    pub steps: Vec<Step>,
    pub links: Vec<Link>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Step {
    pub id: u32,
    pub action: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Link {
    pub from: u32,
    pub to: u32,
    pub condition: String,
}
