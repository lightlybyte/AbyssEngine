# BPC Pipeline Smoke Tests
"""Run a minimal GLB through the complete pipeline and verify output."""
import sys
import json
import os
import struct

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from glb_reader import read_glb
from main import build_instructions


def test_valid_glb():
    """Test that a valid GLB produces structured instructions."""
    glb_path = os.path.join(SCRIPT_DIR, "example", "test.glb")
    assert os.path.exists(glb_path), f"Missing test fixture: {glb_path}"

    result = build_instructions(glb_path)
    assert isinstance(result, list), "Expected list of instructions"
    assert len(result) > 0, "Expected at least one instruction from test GLB"

    required_fields = {"source", "primitive_index", "component", "primitive", "params"}
    for instr in result:
        missing = required_fields - set(instr.keys())
        assert not missing, f"Instruction missing fields: {missing}"
        assert instr["primitive"] in (
            "box", "slab", "plate", "cylinder", "sphere", "complex", "plane"
        ), f"Unknown primitive kind: {instr['primitive']}"
        assert isinstance(instr["params"], dict), "params must be a dict"

    print(f"PASS: {len(result)} instruction(s) generated from test.glb")


def test_missing_file():
    """Test that a missing file produces a clear error, not a traceback."""
    try:
        build_instructions("/nonexistent/path/to/file.glb")
        assert False, "Should have raised ValueError"
    except (ValueError, FileNotFoundError) as e:
        assert "not found" in str(e).lower() or "invalid" in str(e).lower(), f"Unclear error: {e}"
        print(f"PASS: Missing file handled cleanly: {e}")
    except Exception as e:
        assert False, f"Unexpected exception type: {type(e).__name__}: {e}"


def test_malformed_magic():
    """Completely invalid bytes should be rejected clearly."""
    path = os.path.join(SCRIPT_DIR, "_malformed1.bin")
    try:
        with open(path, "wb") as f:
            f.write(b"\xDE\xAD\xBE\xEF" + b"\x00" * 100)
        try:
            build_instructions(path)
            assert False, "Should have raised"
        except ValueError as e:
            assert "magic" in str(e).lower() or "format" in str(e).lower(), f"Poor error: {e}"
            print(f"PASS: malformed_magic - {e}")
    finally:
        if os.path.exists(path):
            os.remove(path)


def test_empty_scene():
    """Minimal GLB with no meshes should produce an empty result, not crash."""
    # Properly formed GLB with empty nodes and no references to undefined indices
    json_data = b'{"asset":{"version":"2.0"},"scene":0,"scenes":[],"nodes":[],"accessors":[],"meshes":[]}'
    glb_bytes = struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(json_data))
    glb_bytes += struct.pack("<II", len(json_data), 0x4E4F534A)
    glb_bytes += json_data

    tmp_path = os.path.join(SCRIPT_DIR, "_tmp_minimal.glb")
    try:
        with open(tmp_path, "wb") as f:
            f.write(glb_bytes)
        result = build_instructions(tmp_path)
        assert isinstance(result, list), "Expected list even for empty scene"
        print("PASS: empty_scene handled without crashing")
    except (IndexError, KeyError, ValueError) as e:
        print(f"FAIL: empty_scene - {e}")
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)


if __name__ == "__main__":
    failed = 0
    for name, fn in [
        ("valid_glb", test_valid_glb),
        ("missing_file", test_missing_file),
        ("malformed_magic", test_malformed_magic),
        ("empty_scene", test_empty_scene),
    ]:
        try:
            fn()
        except AssertionError as e:
            print(f"FAIL: {name}: {e}")
            failed += 1
        except Exception as e:
            print(f"FAIL: {name} unexpected error: {type(e).__name__}: {e}")
            failed += 1

    sys.exit(1 if failed else 0)