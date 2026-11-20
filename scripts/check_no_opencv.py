"""Reject OpenCV in a default build's compile, installed-port and link metadata."""
import argparse
import json
from pathlib import Path
import re


OPENCV = re.compile(r"(?:^|[/\\\s\"'=;]|-l)(?:lib)?opencv(?:[0-9_./\\\s\"';=-]|$)", re.I)


def inspect(root):
    root = Path(root)
    commands = json.loads((root / "compile_commands.json").read_text(encoding="utf-8"))
    if not commands:
        raise ValueError("No compilation commands")
    for command in commands:
        if (OPENCV.search(json.dumps(command)) or
                Path(command["file"]).name in {"ImageOps_opencv.cpp", "ImageOpsReferenceTest.cpp", "GenerateImageOpsGolden.cpp", "ImageOpsBenchmark.cpp"}):
            raise ValueError(f"Unexpected reference dependency: {command['file']}")
    status_path = root / "vcpkg_installed/vcpkg/status"
    ports = set()
    if status_path.exists():
        ports = set(re.findall(r"^Package: (.+)$", status_path.read_text(encoding="utf-8"), re.M))
        if any("opencv" in port.lower() for port in ports):
            raise ValueError("OpenCV remains installed in this build's dependency closure")
    # Ninja records both static and final executable links. Make generators use link.txt.
    links = [root / "build.ninja"] if (root / "build.ninja").exists() else list(root.rglob("link.txt"))
    if not links:
        raise ValueError("No generated link metadata")
    for link in links:
        if OPENCV.search(link.read_text(encoding="utf-8")):
            raise ValueError(f"OpenCV remains in generated link metadata: {link}")
    return {"project_sources": len(commands), "ports": sorted(ports), "installed_status_checked": status_path.exists(), "opencv": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    print(json.dumps(inspect(args.build), indent=2))
