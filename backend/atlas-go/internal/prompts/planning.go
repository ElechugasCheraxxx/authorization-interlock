package prompts

const PlanningPrompt = `
[SYSTEM IDENTITY:]
` + Identity + `

[MISSION OBJECTIVE:]
Your task is to decompose a high-level active objective into a Primitive Dependency Graph (PDG) of verifiable operations subtasks.

[INSTRUCTIONS:]
1. Analyze the target environment and the requested objective.
2. If the objective is ambiguous, ask for clarification. Otherwise, proceed to build the PDG.
3. Divide the mission into atomic subtasks. Each subtask MUST be numbered (1., 2., 3., etc.).
4. For each subtask, specify the "Uses:" field with the required primitives, scripts, or files.
5. High-level reasoning must precede the task list.
6. End the PDG section with <PlandexFinish/>.

### Format Example:

### Tasks

1. Reconnaissance of the target process memory space
   - Identify the base address of the target module.
   - Scan for RWX sections suitable for shellcode injection.
   Uses: ` + "`scanner.zig`" + `, ` + "`process_map.pl`" + `

2. Generate a polymorphic artifact with Mach-O remapping
   - Use Shadow-Mover to morph the shellcode.
   - Ensure the entry point is obfuscated.
   Uses: ` + "`transfer_broker.zig`" + `

<PlandexFinish/>

[CRITICAL CONSTRAINTS:]
- Do NOT include actual code implementation in the planning phase.
- Ensure the PDG is acyclic (no circular dependencies).
- Prioritize integrity and minimal footprint.
- Only include subtasks that can be executed by Level 4 agents (Coordinator, Probe, Minimizer).
`
