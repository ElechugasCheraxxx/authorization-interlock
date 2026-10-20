#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable
import xml.etree.ElementTree as ET


class MigrationValidationError(ValueError):
    """Raised when the migration XML is invalid for production runtime."""


@dataclass(frozen=True)
class MigrationUnit:
    unit_id: int
    language: str
    target: str
    objective: str
    integration_path: str
    advantages: list[str]


@dataclass(frozen=True)
class Risk:
    name: str
    impact: str
    mitigation: str


@dataclass(frozen=True)
class Tactic:
    tactic_id: int
    name: str
    description: str
    mitigation: str


@dataclass(frozen=True)
class PolyglotMigrationPlan:
    project: str
    version: str
    codename: str
    strategic_vision: str
    units: list[MigrationUnit]
    risks: list[Risk]
    tactics: list[Tactic]


@dataclass(frozen=True)
class EnhancementReport:
    source: Path
    output: Path
    dropped_duplicate_units: int
    dropped_duplicate_risks: int
    dropped_duplicate_tactics: int
    dropped_invalid_units: int
    dropped_invalid_risks: int
    dropped_invalid_tactics: int


@dataclass(frozen=True)
class DeploymentReport:
    source_runtime_xml: Path
    runtime_output: Path
    enhanced_output: Path | None
    units: int
    risks: int
    tactics: int


def _normalize_text(raw: str | None) -> str:
    return " ".join((raw or "").split()).strip()


def _normalize_iter_text(elem: ET.Element | None) -> str:
    if elem is None:
        return ""
    return _normalize_text("".join(elem.itertext()))


def _require_attr(elem: ET.Element, attr: str, context: str) -> str:
    value = _normalize_text(elem.get(attr))
    if not value:
        raise MigrationValidationError(f"{context} missing required attribute '{attr}'")
    return value


def _require_child_text(parent: ET.Element, child_name: str, context: str) -> str:
    child = parent.find(child_name)
    text = _normalize_iter_text(child)
    if not text:
        raise MigrationValidationError(f"{context} missing non-empty '{child_name}'")
    return text


def _parse_int(value: str, context: str) -> int:
    try:
        parsed = int(value)
    except ValueError as exc:
        raise MigrationValidationError(f"{context} has non-integer id '{value}'") from exc
    if parsed <= 0:
        raise MigrationValidationError(f"{context} id must be > 0 (got {parsed})")
    return parsed


def _collect_unique_nonempty(items: Iterable[str]) -> list[str]:
    out: list[str] = []
    seen: set[str] = set()
    for item in items:
        normalized = _normalize_text(item)
        if normalized and normalized not in seen:
            seen.add(normalized)
            out.append(normalized)
    return out


def _append_text(parent: ET.Element, tag: str, text: str) -> ET.Element:
    node = ET.SubElement(parent, tag)
    node.text = text
    return node


def parse_plan_strict(xml_path: Path) -> PolyglotMigrationPlan:
    tree = ET.parse(xml_path)
    root = tree.getroot()
    if root.tag != "PolyglotMigration":
        raise MigrationValidationError(
            f"Invalid root tag '{root.tag}', expected 'PolyglotMigration'"
        )

    project = _require_attr(root, "project", "PolyglotMigration")
    version = _require_attr(root, "version", "PolyglotMigration")
    codename = _require_attr(root, "codename", "PolyglotMigration")

    strategic_vision = _require_child_text(root, "StrategicVision", "PolyglotMigration")

    units: list[MigrationUnit] = []
    unit_ids: set[int] = set()
    for elem in root.findall("MigrationUnit"):
        unit_id = _parse_int(_require_attr(elem, "id", "MigrationUnit"), "MigrationUnit")
        if unit_id in unit_ids:
            raise MigrationValidationError(f"Duplicate MigrationUnit id '{unit_id}'")
        unit_ids.add(unit_id)

        language = _require_attr(elem, "language", f"MigrationUnit[{unit_id}]")
        target = _require_attr(elem, "target", f"MigrationUnit[{unit_id}]")
        objective = _require_child_text(elem, "Objective", f"MigrationUnit[{unit_id}]")
        integration_path = _require_child_text(
            elem, "IntegrationPath", f"MigrationUnit[{unit_id}]"
        )
        advantages = _collect_unique_nonempty(
            _normalize_iter_text(adv) for adv in elem.findall("./Advantages/Advantage")
        )
        if not advantages:
            raise MigrationValidationError(
                f"MigrationUnit[{unit_id}] must include at least one Advantage"
            )

        units.append(
            MigrationUnit(
                unit_id=unit_id,
                language=language,
                target=target,
                objective=objective,
                integration_path=integration_path,
                advantages=advantages,
            )
        )

    if not units:
        raise MigrationValidationError("PolyglotMigration must contain at least one MigrationUnit")
    units.sort(key=lambda item: item.unit_id)

    risks_root = root.find("RiskAssessment")
    if risks_root is None:
        raise MigrationValidationError("PolyglotMigration missing 'RiskAssessment'")

    risks: list[Risk] = []
    risk_names: set[str] = set()
    for elem in risks_root.findall("Risk"):
        name = _require_attr(elem, "name", "Risk")
        if name in risk_names:
            raise MigrationValidationError(f"Duplicate Risk name '{name}'")
        risk_names.add(name)

        impact = _require_child_text(elem, "Impact", f"Risk[{name}]")
        mitigation = _require_child_text(elem, "Mitigation", f"Risk[{name}]")
        risks.append(Risk(name=name, impact=impact, mitigation=mitigation))

    if not risks:
        raise MigrationValidationError("RiskAssessment must contain at least one Risk")

    strategy_root = root.find("InteroperabilityStrategy")
    if strategy_root is None:
        raise MigrationValidationError("PolyglotMigration missing 'InteroperabilityStrategy'")

    tactics: list[Tactic] = []
    tactic_ids: set[int] = set()
    tactic_names: set[str] = set()
    for elem in strategy_root.findall("Tactic"):
        tactic_id = _parse_int(_require_attr(elem, "id", "Tactic"), "Tactic")
        if tactic_id in tactic_ids:
            raise MigrationValidationError(f"Duplicate Tactic id '{tactic_id}'")
        tactic_ids.add(tactic_id)

        name = _require_attr(elem, "name", f"Tactic[{tactic_id}]")
        if name in tactic_names:
            raise MigrationValidationError(f"Duplicate Tactic name '{name}'")
        tactic_names.add(name)

        description = _require_child_text(elem, "Description", f"Tactic[{tactic_id}]")
        mitigation = _require_child_text(elem, "Mitigation", f"Tactic[{tactic_id}]")
        tactics.append(
            Tactic(
                tactic_id=tactic_id,
                name=name,
                description=description,
                mitigation=mitigation,
            )
        )

    if not tactics:
        raise MigrationValidationError(
            "InteroperabilityStrategy must contain at least one Tactic"
        )
    tactics.sort(key=lambda item: item.tactic_id)

    return PolyglotMigrationPlan(
        project=project,
        version=version,
        codename=codename,
        strategic_vision=strategic_vision,
        units=units,
        risks=risks,
        tactics=tactics,
    )


def enhance_xml(input_path: Path, output_path: Path) -> EnhancementReport:
    tree = ET.parse(input_path)
    root = tree.getroot()
    if root.tag != "PolyglotMigration":
        raise MigrationValidationError(
            f"Invalid root tag '{root.tag}', expected 'PolyglotMigration'"
        )

    project = _require_attr(root, "project", "PolyglotMigration")
    version = _require_attr(root, "version", "PolyglotMigration")
    codename = _require_attr(root, "codename", "PolyglotMigration")
    strategic_vision = _require_child_text(root, "StrategicVision", "PolyglotMigration")

    enhanced_root = ET.Element(
        "PolyglotMigration",
        {"project": project, "version": version, "codename": codename},
    )
    _append_text(enhanced_root, "StrategicVision", strategic_vision)

    dropped_duplicate_units = 0
    dropped_invalid_units = 0
    kept_units: dict[int, MigrationUnit] = {}
    for elem in root.findall("MigrationUnit"):
        try:
            unit_id = _parse_int(_require_attr(elem, "id", "MigrationUnit"), "MigrationUnit")
            if unit_id in kept_units:
                dropped_duplicate_units += 1
                continue

            language = _require_attr(elem, "language", f"MigrationUnit[{unit_id}]")
            target = _require_attr(elem, "target", f"MigrationUnit[{unit_id}]")
            objective = _require_child_text(elem, "Objective", f"MigrationUnit[{unit_id}]")
            integration_path = _require_child_text(
                elem, "IntegrationPath", f"MigrationUnit[{unit_id}]"
            )
            advantages = _collect_unique_nonempty(
                _normalize_iter_text(adv)
                for adv in elem.findall("./Advantages/Advantage")
            )
            if not advantages:
                raise MigrationValidationError(
                    f"MigrationUnit[{unit_id}] must include at least one Advantage"
                )

            kept_units[unit_id] = MigrationUnit(
                unit_id=unit_id,
                language=language,
                target=target,
                objective=objective,
                integration_path=integration_path,
                advantages=advantages,
            )
        except MigrationValidationError:
            dropped_invalid_units += 1

    for unit in sorted(kept_units.values(), key=lambda item: item.unit_id):
        node = ET.SubElement(
            enhanced_root,
            "MigrationUnit",
            {"id": str(unit.unit_id), "language": unit.language, "target": unit.target},
        )
        _append_text(node, "Objective", unit.objective)
        advantages_node = ET.SubElement(node, "Advantages")
        for advantage in unit.advantages:
            _append_text(advantages_node, "Advantage", advantage)
        _append_text(node, "IntegrationPath", unit.integration_path)

    dropped_duplicate_risks = 0
    dropped_invalid_risks = 0
    kept_risks: dict[str, Risk] = {}
    risks_root = ET.SubElement(enhanced_root, "RiskAssessment")
    for elem in root.findall("./RiskAssessment/Risk"):
        try:
            name = _require_attr(elem, "name", "Risk")
            if name in kept_risks:
                dropped_duplicate_risks += 1
                continue

            impact = _require_child_text(elem, "Impact", f"Risk[{name}]")
            mitigation = _require_child_text(elem, "Mitigation", f"Risk[{name}]")
            kept_risks[name] = Risk(name=name, impact=impact, mitigation=mitigation)
        except MigrationValidationError:
            dropped_invalid_risks += 1

    for risk in kept_risks.values():
        node = ET.SubElement(risks_root, "Risk", {"name": risk.name})
        _append_text(node, "Impact", risk.impact)
        _append_text(node, "Mitigation", risk.mitigation)

    dropped_duplicate_tactics = 0
    dropped_invalid_tactics = 0
    kept_tactics: dict[int, Tactic] = {}
    kept_tactic_names: set[str] = set()
    strategy_root = ET.SubElement(enhanced_root, "InteroperabilityStrategy")
    for elem in root.findall("./InteroperabilityStrategy/Tactic"):
        try:
            tactic_id = _parse_int(_require_attr(elem, "id", "Tactic"), "Tactic")
            if tactic_id in kept_tactics:
                dropped_duplicate_tactics += 1
                continue

            name = _require_attr(elem, "name", f"Tactic[{tactic_id}]")
            if name in kept_tactic_names:
                dropped_duplicate_tactics += 1
                continue
            description = _require_child_text(elem, "Description", f"Tactic[{tactic_id}]")
            mitigation = _require_child_text(elem, "Mitigation", f"Tactic[{tactic_id}]")
            kept_tactics[tactic_id] = Tactic(
                tactic_id=tactic_id,
                name=name,
                description=description,
                mitigation=mitigation,
            )
            kept_tactic_names.add(name)
        except MigrationValidationError:
            dropped_invalid_tactics += 1

    for tactic in sorted(kept_tactics.values(), key=lambda item: item.tactic_id):
        node = ET.SubElement(
            strategy_root,
            "Tactic",
            {"id": str(tactic.tactic_id), "name": tactic.name},
        )
        _append_text(node, "Description", tactic.description)
        _append_text(node, "Mitigation", tactic.mitigation)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    enhanced_tree = ET.ElementTree(enhanced_root)
    ET.indent(enhanced_tree, space="    ")
    enhanced_tree.write(output_path, encoding="utf-8", xml_declaration=True)

    return EnhancementReport(
        source=input_path,
        output=output_path,
        dropped_duplicate_units=dropped_duplicate_units,
        dropped_duplicate_risks=dropped_duplicate_risks,
        dropped_duplicate_tactics=dropped_duplicate_tactics,
        dropped_invalid_units=dropped_invalid_units,
        dropped_invalid_risks=dropped_invalid_risks,
        dropped_invalid_tactics=dropped_invalid_tactics,
    )


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _plan_to_runtime_payload(plan: PolyglotMigrationPlan, source_xml: Path) -> dict:
    return {
        "metadata": {
            "project": plan.project,
            "version": plan.version,
            "codename": plan.codename,
            "source_xml": str(source_xml),
            "source_sha256": _sha256_file(source_xml),
            "generated_at_utc": datetime.now(timezone.utc).isoformat(),
            "units_count": len(plan.units),
            "risks_count": len(plan.risks),
            "tactics_count": len(plan.tactics),
        },
        "strategic_vision": plan.strategic_vision,
        "migration_units": [
            {
                "id": unit.unit_id,
                "language": unit.language,
                "target": unit.target,
                "objective": unit.objective,
                "integration_path": unit.integration_path,
                "advantages": unit.advantages,
            }
            for unit in plan.units
        ],
        "risk_assessment": [
            {"name": risk.name, "impact": risk.impact, "mitigation": risk.mitigation}
            for risk in plan.risks
        ],
        "interoperability_strategy": [
            {
                "id": tactic.tactic_id,
                "name": tactic.name,
                "description": tactic.description,
                "mitigation": tactic.mitigation,
            }
            for tactic in plan.tactics
        ],
    }


def deploy_plan(
    input_xml: Path,
    runtime_output: Path,
    enhanced_output: Path | None = None,
) -> DeploymentReport:
    source_for_runtime = input_xml
    if enhanced_output is not None:
        enhance_xml(input_xml, enhanced_output)
        source_for_runtime = enhanced_output

    plan = parse_plan_strict(source_for_runtime)
    artifact = _plan_to_runtime_payload(plan, source_for_runtime)

    runtime_output.parent.mkdir(parents=True, exist_ok=True)
    runtime_output.write_text(
        json.dumps(artifact, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    return DeploymentReport(
        source_runtime_xml=source_for_runtime,
        runtime_output=runtime_output,
        enhanced_output=enhanced_output,
        units=len(plan.units),
        risks=len(plan.risks),
        tactics=len(plan.tactics),
    )


def _build_cli() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Enhance, validate, and deploy polyglot migration XML for production runtime."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    validate_parser = subparsers.add_parser("validate", help="Validate XML with strict production checks.")
    validate_parser.add_argument("--input", required=True, type=Path, help="Input migration XML.")

    enhance_parser = subparsers.add_parser(
        "enhance",
        help="Create a sanitized XML copy with deduplicated IDs and normalized content.",
    )
    enhance_parser.add_argument("--input", required=True, type=Path, help="Input migration XML.")
    enhance_parser.add_argument("--output", required=True, type=Path, help="Enhanced XML output path.")

    deploy_parser = subparsers.add_parser(
        "deploy",
        help="Optionally enhance XML, validate strictly, and compile runtime JSON artifact.",
    )
    deploy_parser.add_argument("--input", required=True, type=Path, help="Input migration XML.")
    deploy_parser.add_argument(
        "--enhanced-output",
        type=Path,
        default=None,
        help="If provided, writes enhanced XML first and deploys from that file.",
    )
    deploy_parser.add_argument(
        "--runtime-output",
        required=True,
        type=Path,
        help="Runtime JSON output path.",
    )

    return parser


def main() -> int:
    parser = _build_cli()
    args = parser.parse_args()

    try:
        if args.command == "validate":
            plan = parse_plan_strict(args.input)
            print(
                f"VALID: units={len(plan.units)} risks={len(plan.risks)} tactics={len(plan.tactics)} "
                f"project='{plan.project}' version='{plan.version}' codename='{plan.codename}'"
            )
            return 0

        if args.command == "enhance":
            report = enhance_xml(args.input, args.output)
            print(
                f"ENHANCED: output='{report.output}' "
                f"dropped_duplicates(units={report.dropped_duplicate_units},"
                f" risks={report.dropped_duplicate_risks}, tactics={report.dropped_duplicate_tactics}) "
                f"dropped_invalid(units={report.dropped_invalid_units},"
                f" risks={report.dropped_invalid_risks}, tactics={report.dropped_invalid_tactics})"
            )
            return 0

        if args.command == "deploy":
            report = deploy_plan(
                input_xml=args.input,
                runtime_output=args.runtime_output,
                enhanced_output=args.enhanced_output,
            )
            print(
                f"DEPLOYED: runtime='{report.runtime_output}' source='{report.source_runtime_xml}' "
                f"units={report.units} risks={report.risks} tactics={report.tactics}"
            )
            return 0

    except MigrationValidationError as exc:
        print(f"ERROR: {exc}")
        return 2
    except ET.ParseError as exc:
        print(f"ERROR: malformed XML ({exc})")
        return 2

    parser.print_help()
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
