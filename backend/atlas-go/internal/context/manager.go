package context

import (
	"fmt"
	"sort"
	"strings"
)

// FileMap representa un mapa de archivos con su resumen o símbolos.
// Basado en la lógica de Plandex para optimización de tokens (MLA en Interlock).
type FileMap struct {
	Paths   map[string]string // Path -> Resumen/Símbolos
	Tokens  map[string]int    // Path -> Token count
}

func NewFileMap() *FileMap {
	return &FileMap{
		Paths:  make(map[string]string),
		Tokens: make(map[string]int),
	}
}

// CombinedMap genera una representación compacta para el LLM (Cipher/Vantage).
func (m *FileMap) CombinedMap() string {
	var sb strings.Builder
	paths := make([]string, 0, len(m.Paths))
	for path := range m.Paths {
		paths = append(paths, path)
	}
	sort.Strings(paths)

	for _, path := range paths {
		body := strings.TrimSpace(m.Paths[path])
		tokens := m.Tokens[path]

		sb.WriteString(fmt.Sprintf("\n### %s (%d tokens)\n", path, tokens))
		if body == "" {
			sb.WriteString("[NO MAP - FILE EMPTY OR BINARY]\n")
		} else {
			sb.WriteString(body)
		}
		sb.WriteString("\n")
	}
	return sb.String()
}

// AddFile añade un archivo al mapa de contexto.
func (m *FileMap) AddFile(path string, body string, tokens int) {
	m.Paths[path] = body
	m.Tokens[path] = tokens
}
