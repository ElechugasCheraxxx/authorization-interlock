package model

import "time"

// Subtask representa una unidad mínima de operación táctica.
// Adapted from Plandex for the assessment domain (Interlock).
type Subtask struct {
	ID          string    `json:"id"`
	Title       string    `json:"title"`       // Título de la tarea (ej: "Escalada de privilegios vía SeBackupPrivilege")
	Description string    `json:"description"` // Detalles de ejecución
	UsesFiles   []string  `json:"uses_files"`  // Archivos/Scripts necesarios (ej: ["assess.zig", "artifact.bin"])
	Status      string    `json:"status"`      // pending, running, completed, failed, verified
	CreatedAt   time.Time `json:"created_at"`
}

// Plan es un conjunto de subtasks que forman un vector de ataque.
type Plan struct {
	ID        string     `json:"id"`
	Target    string     `json:"target"` // IP, Dominio o Proceso objetivo
	Subtasks  []*Subtask `json:"subtasks"`
	AgentID   string     `json:"agent_id"` // Agente que generó el plan (ej: Cipher)
	CreatedAt time.Time  `json:"created_at"`
}
