"""Drive and check XTest mouse input through a real SDL X11 window."""
from verify_pointer import check_fields, event, records


def drive(x11, window, helper, directory, events, wait_for):
    def pointers():
        return records(directory / "pointer-events.jsonl")

    def viewport(width, height, rectangle):
        observations = records(directory / "presentation-events.jsonl")
        return observations and all(observations[-1].get(key) == value for key, value in dict(
            actual_width=width, actual_height=height, pixel_width=width, pixel_height=height,
            logical_width=640, logical_height=480, presentation=rectangle).items())

    def send(action, expected):
        start = len(pointers())
        action()
        wait_for(lambda: expected in pointers()[start:])

    wait_for(lambda: viewport(800, 480, [80, 0, 640, 480]))
    send(lambda: x11.motion(window, 180, 120), event("move", 100, 120))
    send(lambda: x11.button(True), event("down", 100, 120, 1, 8))
    send(lambda: x11.motion(window, 40, 120), event("move", -40, 120, shift=8))
    # SDL's automatic capture must retain this real drag outside the window.
    send(lambda: x11.motion(window, 850, 120), event("move", 770, 120, shift=8))
    send(lambda: x11.button(False), event("up", 770, 120, 1))
    send(lambda: x11.motion(window, 280, 220), event("move", 200, 220))
    send(lambda: x11.button(True, 4), event("wheel", 200, 220, wheel_y=1))
    x11.button(False, 4)
    x11.resize(window, 1200, 720)
    wait_for(lambda: viewport(1200, 720, [120, 0, 960, 720]))
    send(lambda: x11.motion(window, 270, 180), event("move", 100, 120))
    send(lambda: x11.button(True, 3), event("down", 100, 120, 2, 16))
    start = len(pointers())
    x11.focus(helper)
    wait_for(lambda: "focus:0" in events())
    wait_for(lambda: any(item["kind"] == "up" and item["button"] == 2
                         for item in pointers()[start:]))
    x11.button(False, 3)
    previous_gains = events().count("focus:1")
    x11.focus(window)
    wait_for(lambda: events().count("focus:1") == previous_gains + 1)
    # Focus transitions can move the virtual pointer. Acknowledge its position.
    send(lambda: x11.motion(window, 300, 210), event("move", 120, 140))
    send(lambda: x11.motion(window, 270, 180), event("move", 100, 120))
    send(lambda: x11.button(True, 3), event("down", 100, 120, 2, 16))
    send(lambda: x11.button(False, 3), event("up", 100, 120, 2))


def verify(directory, report):
    check_fields(report, dict(key_downs=0, key_ups=0, pointer_downs=3, pointer_ups=3,
        pointer_wheels=1, pointer_test=False, actual_width=1200, actual_height=720,
        pixel_width=1200, pixel_height=720, logical_width=640, logical_height=480,
        presentation=[120, 0, 960, 720]))
    observed = records(directory / "pointer-events.jsonl")
    transitions = [item for item in observed if item["kind"] != "move"]
    expected = [event("down", 100, 120, 1, 8), event("up", 770, 120, 1),
                event("wheel", 200, 220, wheel_y=1), event("down", 100, 120, 2, 16),
                event("up", 100, 120, 2), event("down", 100, 120, 2, 16),
                event("up", 100, 120, 2)]
    # SDL may itself emit an up before focus loss, or our shared state releases
    # it on loss. Require exactly one up, with no held flags, in either case.
    if len(transitions) == len(expected):
        expected[4]["synthetic"] = transitions[4]["synthetic"]
    if transitions != expected:
        raise ValueError(f"Unexpected native pointer transitions: {transitions}")
    return observed
