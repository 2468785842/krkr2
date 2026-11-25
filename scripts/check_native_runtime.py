"""Verify the Windows/Linux SDL3 application has an independent, Cocos-free closure."""
import argparse
import json
from pathlib import Path
import re
import subprocess

from audit_dependencies import installed_ports


def inspect(build):
    cache = {}
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        if not line.startswith(("#", "//")) and "=" in line and ":" in line:
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    for name in ("KRKR2_BUILD_APP", "KRKR2_SDL3_ONLY", "KRKR2_ENABLE_SDL3_HOST"):
        if cache.get(name, "").upper() not in {"ON", "TRUE", "1", "YES"}:
            raise ValueError(f"{name} must be ON")
    if cache.get("KRKR2_ENABLE_UI_TEST_DRIVER", "").upper() not in {"OFF", "FALSE", "0", "NO"}:
        raise ValueError("Verify an ordinary application with UI test driver OFF")
    sdk = Path(cache["VCPKG_INSTALLED_DIR"])
    status = (sdk / "vcpkg/status").read_text()
    packages = {port["name"] for port in installed_ports(status).values()}
    windows = cache.get("WINDOWS") in {"ON", "TRUE", "1"}
    required = {"sdl3", "ffmpeg"} | (set() if windows else {"gtk3"})
    if "cocos2dx" in packages or not required <= packages:
        raise ValueError("SDK lacks native host dependencies or contains Cocos")
    commands = json.loads((build / "compile_commands.json").read_text())
    if not commands:
        raise ValueError("Empty compilation database")
    forbidden = re.compile(r"/environ/(?:cocos2d|ui)/|/include/cocos(?:/|\s)|(?:lib)?(?:cocos2d|fmod|glfw)[^/ ]*\.(?:a|so|lib|dll)", re.I)
    def legacy(text):
        return forbidden.search(re.sub(r"/+", "/", text.replace("\\", "/")))
    for entry in commands:
        if legacy(json.dumps(entry)):
            raise ValueError(f"Legacy dependency in compilation: {entry['file']}")
    linked = subprocess.check_output(["ninja", "-C", str(build), "-t", "commands", "krkr2"], text=True)
    if legacy(linked):
        raise ValueError("Legacy dependency in application build/link commands")
    engine = build / "bin/krkr2" / ("krkr2.exe" if windows else "krkr2")
    libraries = subprocess.check_output(
        ["dumpbin", "/DEPENDENTS", str(engine)] if windows else ["ldd", str(engine)], text=True, errors="replace")
    if legacy(libraries) or "not found" in libraries:
        raise ValueError("Missing or legacy runtime library: " + libraries)
    resources = engine.parent / "Resources"
    if any((resources / name).exists() for name in ("img", "out_ui")):
        raise ValueError("Cocos management UI assets remain in native packaging")
    if not (resources / "locale/en_us.json").is_file():
        raise ValueError("Missing native locale resources")
    if not (resources / "licenses/PVRTCDecoder.txt").is_file():
        raise ValueError("Missing PowerVR license")
    rejected = subprocess.run([str(engine), "--host=cocos", "--help"], capture_output=True, text=True)
    if rejected.returncode == 0 or "Cocos host is unavailable" not in rejected.stderr:
        raise ValueError("Native application must reject the unavailable Cocos host")
    manifest = json.loads((Path(__file__).resolve().parents[1] / "vcpkg.json").read_text())
    features = manifest["features"]
    def name(dependency):
        return dependency if isinstance(dependency, str) else dependency["name"]
    if any(name(d) == "cocos2dx" for d in features["engine"]["dependencies"]):
        raise ValueError("Common engine dependencies must not contain Cocos")
    if not any(name(d) == "cocos2dx" for d in features["cocos"]["dependencies"]):
        raise ValueError("Legacy host must explicitly declare its Cocos feature")
    print(f"Cocos-free {'Windows' if windows else 'Linux'} runtime verified: {len(commands)} compilation units, {len(packages)} SDK ports")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    inspect(parser.parse_args().build.resolve())
