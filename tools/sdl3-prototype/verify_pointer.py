"""Pointer/viewport assertions for the optional SDL3 experiment."""
import json

from verify import pixel


def records(path):
    """Only consume completed JSONL records while the engine is still writing."""
    if not path.exists():
        return []
    lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
    return [json.loads(line) for line in lines if line.endswith("\n")]


def event(kind, x, y, button=0, shift=0, synthetic=False, wheel_x=0, wheel_y=0):
    return dict(kind=kind, x=x, y=y, button=button, shift=shift,
                current=kind == "down", pending=False, synthetic=synthetic,
                wheel_x=wheel_x, wheel_y=wheel_y)


def check_fields(observed, expected):
    for name, value in expected.items():
        if observed.get(name) != value:
            raise ValueError(f"{name}: {observed.get(name)} != {value}")


def verify(directory):
    report = json.loads((directory / "report.json").read_text(encoding="utf-8"))
    check_fields(report, dict(completed=True, renderer="software", self_test=False,
        pointer_test=True, frames=8, key_downs=0, key_ups=0, close_vetoes=1,
        focus_losses=1, focus_gains=1, synthetic_releases=1,
        pointer_downs=2, pointer_ups=2, pointer_wheels=1, pointer_synthetic_ups=1,
        width=800, height=480, actual_width=800, actual_height=480,
        pixel_width=800, pixel_height=480, logical_width=640, logical_height=480,
        presentation=[80, 0, 640, 480]))
    for name, minimum in dict(pointer_moves=3, pointer_foreign=1,
                               pointer_unfocused=2, pointer_redundant_ups=1).items():
        if report.get(name, 0) < minimum:
            raise ValueError(f"{name}: {report.get(name)} < {minimum}")
    observed = records(directory / "pointer-events.jsonl")
    expected = [event("down", 100, 120, 1, 8), event("up", 770, 120, 1),
                event("wheel", 200, 220, wheel_x=-0.5, wheel_y=1.5),
                event("down", 200, 220, 2, 16), event("up", 200, 220, 2, synthetic=True)]
    if [item for item in observed if item["kind"] != "move"] != expected:
        raise ValueError(f"Unexpected pointer transitions: {observed}")
    # OS startup can add an initial motion. The scripted drag must stay ordered.
    moves = iter(item for item in observed if item["kind"] == "move")
    for expected_move in [event("move", 100, 120), event("move", -40, 120, shift=8),
                          event("move", 770, 120, shift=8)]:
        if not any(item == expected_move for item in moves):
            raise ValueError(f"Missing ordered motion: {expected_move}; {observed}")
    if (directory / "events.txt").read_text(encoding="utf-8").splitlines() != [
            "focus:0", "focus:1", "close-accepted:0", "close-accepted:1"]:
        raise ValueError("Unexpected focus/close sequence")
    samples = [("initial.bmp", 10, 10, 0x203040), ("initial.bmp", 100, 120, 0xd08030),
               ("pointer-down.bmp", 105, 125, 0x30c070), ("pointer-down.bmp", 90, 125, 0x203040),
               ("pointer-drag.bmp", 10, 125, 0x30c070), ("pointer-drag.bmp", 45, 125, 0x203040),
               ("pointer-up.bmp", 10, 125, 0x203040), ("pointer-wheel.bmp", 205, 225, 0x3050d0),
               ("pointer-unfocused.bmp", 10, 10, 0x102030),
               ("pointer-unfocused.bmp", 205, 225, 0xd08030),
               ("pointer-restored.bmp", 10, 10, 0x203040),
               ("pointer-restored.bmp", 205, 225, 0xd08030), ("pointer-veto.bmp", 10, 10, 0x403020)]
    for name, x, y, color in samples:
        actual = pixel(directory / name, x, y)
        if actual != color:
            raise ValueError(f"{name}({x},{y}): {actual:06x} != {color:06x}")
    return report
