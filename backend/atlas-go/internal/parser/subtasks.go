package parser

import (
	"interlock/atlas/internal/model"
	"log"
	"regexp"
	"strings"
	"time"
	"github.com/google/uuid"
)

// ParseSubtasks adapta la lógica de Plandex para extraer misiones tácticas
// del razonamiento de Cipher (DeepSeek-R1).
func ParseSubtasks(replyContent string, agentID string) []*model.Subtask {
	// Buscamos la sección de tareas. Soportamos "### Tasks" (estándar) o "### PDG" (Interlock spec)
	split := strings.Split(replyContent, "### Tasks")
	if len(split) < 2 {
		split = strings.Split(replyContent, "### PDG") // Primitive Dependency Graph
		if len(split) < 2 {
			log.Println("[Parser] No tasks/PDG section found in reply")
			return nil
		}
	}

	lines := strings.Split(split[1], "\n")
	var subtasks []*model.Subtask
	var currentTask *model.Subtask
	var descLines []string

	// Regex para detectar el inicio de una tarea (ej: "1. Tarea")
	taskRegex := regexp.MustCompile(`^\d+\.\s`)

	for _, line := range lines {
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}

		if taskRegex.MatchString(line) {
			// Guardar tarea previa
			if currentTask != nil {
				currentTask.Description = strings.Join(descLines, "\n")
				subtasks = append(subtasks, currentTask)
			}

			// Iniciar nueva tarea táctica
			parts := strings.SplitN(line, ". ", 2)
			if len(parts) == 2 {
				currentTask = &model.Subtask{
					ID:        uuid.New().String(),
					Title:     parts[1],
					Status:    "pending",
					CreatedAt: time.Now(),
				}
				descLines = nil
			}
			continue
		}

		// Soporte para "Uses:" (archivos/primitivas)
		if strings.HasPrefix(line, "Uses:") {
			if currentTask != nil {
				usesStr := strings.TrimPrefix(line, "Uses:")
				for _, use := range strings.Split(usesStr, ",") {
					use = strings.TrimSpace(use)
					use = strings.Trim(use, "`")
					if use != "" {
						currentTask.UsesFiles = append(currentTask.UsesFiles, use)
					}
				}
			}
			continue
		}

		// Acumular descripción/instrucciones
		if currentTask != nil {
			line = strings.TrimPrefix(line, "-")
			line = strings.TrimSpace(line)
			if line != "" {
				descLines = append(descLines, line)
			}
		}
	}

	// Guardar la última tarea
	if currentTask != nil {
		currentTask.Description = strings.Join(descLines, "\n")
		subtasks = append(subtasks, currentTask)
	}

	log.Printf("[Parser] Extracted %d operations missions for agent %s", len(subtasks), agentID)
	return subtasks
}
