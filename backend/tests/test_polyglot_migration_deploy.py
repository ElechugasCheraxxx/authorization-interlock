import json
import sys
import tempfile
import unittest
from pathlib import Path
REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.polyglot_migration_deploy import (
    MigrationValidationError,
    deploy_plan,
    enhance_xml,
    parse_plan_strict,
)


VALID_XML = """<?xml version="1.0" encoding="UTF-8"?>
<PolyglotMigration project="Test Project" version="1.0" codename="TEST">
    <StrategicVision>Build a safe multi-runtime architecture.</StrategicVision>
    <MigrationUnit id="1" language="Rust" target="Cipher Module">
        <Objective>Move critical crypto to memory-safe runtime.</Objective>
        <Advantages>
            <Advantage>Memory safety.</Advantage>
            <Advantage>Deterministic behavior.</Advantage>
        </Advantages>
        <IntegrationPath>FFI with C++ core.</IntegrationPath>
    </MigrationUnit>
    <RiskAssessment>
        <Risk name="Boundary mismatch">
            <Impact>Unstable runtime behavior.</Impact>
            <Mitigation>Use strict ABI wrappers and integration tests.</Mitigation>
        </Risk>
    </RiskAssessment>
    <InteroperabilityStrategy>
        <Tactic id="1" name="C-ABI Bridge">
            <Description>Expose stable interfaces.</Description>
            <Mitigation>Use fixed-width primitive types.</Mitigation>
        </Tactic>
    </InteroperabilityStrategy>
</PolyglotMigration>
"""


DUPLICATE_TACTIC_XML = """<?xml version="1.0" encoding="UTF-8"?>
<PolyglotMigration project="Test Project" version="1.0" codename="TEST">
    <StrategicVision>Build a safe multi-runtime architecture.</StrategicVision>
    <MigrationUnit id="1" language="Rust" target="Cipher Module">
        <Objective>Move critical crypto to memory-safe runtime.</Objective>
        <Advantages>
            <Advantage>Memory safety.</Advantage>
        </Advantages>
        <IntegrationPath>FFI with C++ core.</IntegrationPath>
    </MigrationUnit>
    <RiskAssessment>
        <Risk name="Boundary mismatch">
            <Impact>Unstable runtime behavior.</Impact>
            <Mitigation>Use strict ABI wrappers and integration tests.</Mitigation>
        </Risk>
    </RiskAssessment>
    <InteroperabilityStrategy>
        <Tactic id="1" name="C-ABI Bridge">
            <Description>Expose stable interfaces.</Description>
            <Mitigation>Use fixed-width primitive types.</Mitigation>
        </Tactic>
        <Tactic id="1" name="C-ABI Bridge Duplicate">
            <Description>Duplicate row.</Description>
            <Mitigation>Should be dropped by enhancer.</Mitigation>
        </Tactic>
    </InteroperabilityStrategy>
</PolyglotMigration>
"""


class TestPolyglotMigrationDeploy(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.base = Path(self.temp_dir.name)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def _write(self, name: str, content: str) -> Path:
        path = self.base / name
        path.write_text(content, encoding="utf-8")
        return path

    def test_strict_validation_rejects_duplicate_tactic_ids(self) -> None:
        xml_path = self._write("duplicate.xml", DUPLICATE_TACTIC_XML)
        with self.assertRaises(MigrationValidationError):
            parse_plan_strict(xml_path)

    def test_enhance_deduplicates_and_becomes_strictly_valid(self) -> None:
        source = self._write("source.xml", DUPLICATE_TACTIC_XML)
        enhanced = self.base / "enhanced.xml"

        report = enhance_xml(source, enhanced)
        plan = parse_plan_strict(enhanced)

        self.assertEqual(report.dropped_duplicate_tactics, 1)
        self.assertEqual(len(plan.tactics), 1)
        self.assertEqual(plan.tactics[0].name, "C-ABI Bridge")

    def test_deploy_generates_runtime_json(self) -> None:
        source = self._write("source.xml", VALID_XML)
        runtime_json = self.base / "runtime.json"

        report = deploy_plan(source, runtime_json)
        self.assertTrue(runtime_json.exists())
        self.assertEqual(report.units, 1)
        self.assertEqual(report.risks, 1)
        self.assertEqual(report.tactics, 1)

        artifact = json.loads(runtime_json.read_text(encoding="utf-8"))
        self.assertEqual(artifact["metadata"]["project"], "Test Project")
        self.assertEqual(artifact["metadata"]["units_count"], 1)
        self.assertEqual(artifact["metadata"]["tactics_count"], 1)
        self.assertEqual(len(artifact["metadata"]["source_sha256"]), 64)


if __name__ == "__main__":
    unittest.main()
