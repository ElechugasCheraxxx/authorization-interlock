package oracle

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"strings"
	"sync"
)

// GlasswingOracle implementa la lógica de validación formal en Go.
// Actúa como un bridge hacia Isabelle/Coq o ejecutables extraídos (OCaml).
type GlasswingOracle struct {
	mu          sync.Mutex
	isExecuting bool
	trustedPath string // Ruta al binario extraído de Coq
}

func NewGlasswingOracle(path string) *GlasswingOracle {
	if path == "" {
		// Use project-local compiled binary for M4 Pro dev environment
		path = "/Users/Cheraxxx/Interlock IDE/backend/proofs/coq/interlock_coq_validator"
	}
	return &GlasswingOracle{
		trustedPath: path,
	}
}

// ValidatePDG verifica que el grafo de tareas no tenga ciclos y sea formalmente seguro.
func (o *GlasswingOracle) ValidatePDG(tasks []string) (bool, error) {
	o.mu.Lock()
	defer o.mu.Unlock()

	log.Printf("[Oracle] Validating PDG with %d tasks...", len(tasks))

	// Preparamos el input para el validador formal (Path B)
	input := strings.Join(tasks, ",")

	res, err := o.callTrustedValidator("is_acyclic", input)
	if err != nil {
		return false, fmt.Errorf("formal validation error: %v", err)
	}

	if strings.TrimSpace(res) == "true" {
		return true, nil
	}

	return false, fmt.Errorf("formal mismatch: PDG failed acyclicity check")
}

// ValidateTransition verifica formalmente si una transición de fase es legal.
func (o *GlasswingOracle) ValidateTransition(from, to string) (bool, error) {
	o.mu.Lock()
	defer o.mu.Unlock()

	log.Printf("[Oracle] Validating phase transition: %s -> %s", from, to)

	input := fmt.Sprintf("%s,%s", from, to)
	res, err := o.callTrustedValidator("is_legal_transition", input)
	if err != nil {
		return false, fmt.Errorf("formal validation error: %v", err)
	}

	return strings.TrimSpace(res) == "true", nil
}

// Artifact representa un elemento de evidencia para validación.
type Artifact struct {
	ID            string
	SourceNode    string
	IntegrityHash string
	IsCorrupted   bool
}

// ValidateArtifactIntegrity asegura que la evidencia no esté corrupta según el Core Seguro.
func (o *GlasswingOracle) ValidateArtifactIntegrity(raw string, art Artifact) (bool, error) {
	o.mu.Lock()
	defer o.mu.Unlock()

	log.Printf("[Oracle] Validating integrity for artifact %s", art.ID)

	input := fmt.Sprintf("%s,%s,%s,%s,%t", raw, art.ID, art.SourceNode, art.IntegrityHash, art.IsCorrupted)
	res, err := o.callTrustedValidator("check_integrity", input)
	if err != nil {
		return false, fmt.Errorf("formal validation error: %v", err)
	}

	return strings.TrimSpace(res) == "true", nil
}

func (o *GlasswingOracle) hasCycles(tasks []string) bool {
	// Implementación básica de detección de ciclos
	// En Interlock, esto se delega al Trusted Core (Path B)
	return false
}

// VerifyAssessmentConstraint asegura que el plan sigue la constitución de Interlock.
func (o *GlasswingOracle) VerifyAssessmentConstraint(plan string) bool {
	// Aquí se podrían integrar reglas LTL (Linear Temporal Logic) portadas de Coq
	if strings.Contains(plan, "destructive") && !strings.Contains(plan, "authorized") {
		log.Println("🚨 [ORACLE] Violation detected: Destructive action without explicit authorization!")
		return false
	}
	return true
}

func (o *GlasswingOracle) callTrustedValidator(cmd string, args string) (string, error) {
	if _, err := os.Stat(o.trustedPath); os.IsNotExist(err) {
		return "", fmt.Errorf("trusted core validator not found at %s", o.trustedPath)
	}

	out, err := exec.Command(o.trustedPath, cmd, args).CombinedOutput()
	if err != nil {
		return "", fmt.Errorf("validator execution failed: %v", err)
	}

	return string(out), nil
}
