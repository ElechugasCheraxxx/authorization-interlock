#!/usr/bin/env python3

"""
Interlock ABB Inspector - Análisis de Autonomous Breadcrumb Buffer
Permite inspeccionar, analizar y depurar transiciones inter-lenguajes
"""

import struct
import mmap
import sys
import os
from datetime import datetime
from typing import List, NamedTuple

# Estructura de entrada ABB (64 bytes)
class BreadcrumbEntry(NamedTuple):
    timestamp: int
    source_id: int
    target_id: int
    func_pointer: int
    stack_depth: int
    thread_id: int
    execution_context: int

ENTRY_SIZE = 64
MAX_ENTRIES = (256 * 1024 * 1024) // ENTRY_SIZE
ABB_PATH = "/dev/shm/interlock_abb_prod"

# Mapeo de IDs a nombres
COMPONENT_NAMES = {
    0: "UNKNOWN",
    1: "CPP_CORE",
    2: "SWIFT",
    3: "RUST_CIPHER",
    4: "GO_ATLAS",
    5: "ZIG_PAYLOAD",
    6: "C_SHIELD",
    7: "ASM_STUB",
    8: "TS_INTEL",
    9: "PERL_SURGERY",
}

def get_component_name(comp_id: int) -> str:
    return COMPONENT_NAMES.get(comp_id, f"COMPONENT_{comp_id}")

def read_entry(data: bytes, offset: int) -> BreadcrumbEntry:
    """Lee una entrada del buffer ABB"""
    fmt = "<QQIIIBBBBI"
    values = struct.unpack(fmt, data[offset:offset + ENTRY_SIZE])
    return BreadcrumbEntry(*values)

def format_timestamp(ts: int) -> str:
    """Convierte timestamp a formato legible"""
    try:
        sec = ts // 1_000_000_000
        return datetime.fromtimestamp(sec).isoformat()
    except:
        return f"0x{ts:016x}"

def inspect_abb(num_entries: int = 100):
    """Inspecciona el ABB y muestra transiciones recientes"""

    if not os.path.exists(ABB_PATH):
        print(f"❌ Error: {ABB_PATH} no encontrado")
        print("   Asegúrate de que el Core está ejecutándose")
        return 1

    try:
        with open(ABB_PATH, "r+b") as f:
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as buf:
                print(f"{'═' * 100}")
                print(f"  Interlock Autonomous Breadcrumb Buffer (ABB) Inspector")
                print(f"  Archivo: {ABB_PATH}")
                print(f"  Tamaño: {len(buf) / 1024 / 1024:.0f} MB")
                print(f"{'═' * 100}")
                print()

                # Intentar leer el índice de escritura (primeros 8 bytes)
                # (En una implementación real, esta información estaría disponible)

                # Leer últimas N entradas
                print(f"{'Timestamp':<26} {'Source':<15} {'Target':<15} {'Function':<16} {'Depth':<6} {'Context':<8}")
                print(f"{'-' * 100}")

                entries_shown = 0
                for i in range(num_entries):
                    offset = i * ENTRY_SIZE
                    if offset >= len(buf):
                        break

                    try:
                        entry = read_entry(buf[:], offset)

                        # Saltar entradas vacías
                        if entry.timestamp == 0:
                            continue

                        src = get_component_name(entry.source_id)
                        tgt = get_component_name(entry.target_id)
                        func_ptr = f"0x{entry.func_pointer:016x}"
                        ts_str = format_timestamp(entry.timestamp)
                        context = "kernel" if entry.execution_context == 1 else "userspace"

                        print(f"{ts_str:<26} {src:<15} {tgt:<15} {func_ptr:<16} {entry.stack_depth:<6} {context:<8}")

                        entries_shown += 1
                        if entries_shown >= num_entries:
                            break
                    except Exception as e:
                        continue

                print()
                print(f"  Entradas mostradas: {entries_shown}")
                print(f"  Capacidad total: {MAX_ENTRIES:,}")
                print(f"{'-' * 100}")

                return 0

    except Exception as e:
        print(f"❌ Error al leer ABB: {e}")
        return 1

def analyze_abb_patterns():
    """Analiza patrones de comunicación en el ABB"""

    if not os.path.exists(ABB_PATH):
        print(f"❌ {ABB_PATH} no encontrado")
        return 1

    transition_counts = {}
    total_entries = 0

    try:
        with open(ABB_PATH, "r+b") as f:
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as buf:
                print(f"Analizando patrones de transiciones...")

                for i in range(MAX_ENTRIES):
                    offset = i * ENTRY_SIZE
                    if offset >= len(buf):
                        break

                    try:
                        entry = read_entry(buf[:], offset)
                        if entry.timestamp == 0:
                            continue

                        key = (entry.source_id, entry.target_id)
                        transition_counts[key] = transition_counts.get(key, 0) + 1
                        total_entries += 1
                    except:
                        continue

                if total_entries == 0:
                    print("No hay entradas en el ABB")
                    return 0

                print(f"\n{'═' * 80}")
                print(f"  Transiciones más frecuentes ({total_entries} total)")
                print(f"{'═' * 80}")

                # Ordenar por frecuencia
                sorted_transitions = sorted(transition_counts.items(), key=lambda x: x[1], reverse=True)

                print(f"{'Origen':<15} {'Destino':<15} {'Count':<10} {'%':<8}")
                print(f"{'-' * 80}")

                for (src_id, tgt_id), count in sorted_transitions[:20]:
                    src = get_component_name(src_id)
                    tgt = get_component_name(tgt_id)
                    pct = (count / total_entries) * 100
                    print(f"{src:<15} {tgt:<15} {count:<10} {pct:>6.1f}%")

                return 0

    except Exception as e:
        print(f"❌ Error: {e}")
        return 1

if __name__ == "__main__":
    if len(sys.argv) > 1:
        if sys.argv[1] == "analyze":
            sys.exit(analyze_abb_patterns())
        elif sys.argv[1] == "inspect":
            num = int(sys.argv[2]) if len(sys.argv) > 2 else 100
            sys.exit(inspect_abb(num))
    else:
        # Por defecto, mostrar últimas 50 entradas
        sys.exit(inspect_abb(50))
