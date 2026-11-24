"""Package an audited Release runtime, resources, licenses and dependency manifest."""
import argparse
import hashlib
import json
import platform
import re
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

from check_native_runtime import inspect


def package(build, output):
    build = build.resolve()
    inspect(build)
    cache = (build / "CMakeCache.txt").read_text()
    if "CMAKE_BUILD_TYPE:STRING=Release" not in cache:
        raise ValueError("Distribution requires a Release build")
    windows = platform.system() == "Windows"
    package_name = "krkr2-sdl3-" + ("windows" if windows else "linux") + "-" + platform.machine().lower()
    output.mkdir(parents=True, exist_ok=True)
    archive = output / (package_name + (".zip" if windows else ".tar.gz"))
    if archive.exists():
        raise FileExistsError(archive)
    source = Path(__file__).resolve().parents[1]
    executable = "krkr2.exe" if windows else "krkr2"
    binary = build / "bin/krkr2" / executable
    dependencies = subprocess.check_output(
        ["dumpbin", "/DEPENDENTS", str(binary)] if windows else ["ldd", str(binary)],
        text=True, errors="replace")
    with tempfile.TemporaryDirectory(prefix="krkr2-package-") as temporary:
        root = Path(temporary) / package_name
        root.mkdir()
        shutil.copy2(binary, root / executable)
        if windows:
            pending = re.findall(r"\S+\.dll", dependencies, re.I)
            seen = set()
            while pending:
                name = pending.pop()
                if name.lower() in seen:
                    continue
                seen.add(name.lower())
                dll = binary.parent / name
                if dll.is_file():
                    shutil.copy2(dll, root / dll.name)
                    nested = subprocess.check_output(["dumpbin", "/DEPENDENTS", str(dll)], text=True, errors="replace")
                    pending.extend(re.findall(r"\S+\.dll", nested, re.I))
        # Package only the current manifest, excluding stale files from a build
        # directory previously configured with another host or resource set.
        selected = (build / "runtime-assets-selected.txt").read_text().splitlines()
        if not selected:
            raise ValueError("Empty runtime resource selection")
        for resource_name in selected:
            path = Path(resource_name)
            if path.is_absolute() or ".." in path.parts:
                raise ValueError(f"Invalid resource path: {resource_name}")
            target = root / "Resources" / path
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(binary.parent / "Resources" / path, target)
        shutil.copy2(source / "LICENSE", root / "LICENSE")
        (root / "runtime-dependencies.txt").write_text(dependencies, encoding="utf-8")
        (root / "README.txt").write_text(
            "KrKr2 SDL3 native runtime (software renderer)\n"
            f"Run: ./{executable} --game /path/to/game\n"
            "Keep Resources beside the executable. Cocos is not required.\n"
            "Install the OS libraries listed in runtime-dependencies.txt; they are not bundled.\n"
            "Linux binaries require the build host's glibc version or newer. Windows requires the MSVC runtime.\n"
            "Game assets and saves are not included.\n", encoding="utf-8")
        files = {str(path.relative_to(root)).replace("\\", "/"):
                 hashlib.sha256(path.read_bytes()).hexdigest()
                 for path in sorted(root.rglob("*")) if path.is_file()}
        metadata = {
            "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip(),
            "dirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=source, text=True)),
            "platform": platform.system(), "architecture": platform.machine(),
            "libc": platform.libc_ver(), "renderer": "software", "sha256": files,
        }
        (root / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        if windows:
            with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as target:
                for path in sorted(root.rglob("*")):
                    if path.is_file():
                        target.write(path, str(path.relative_to(root.parent)))
        else:
            def permissions(info):
                info.uid = info.gid = 0
                info.uname = info.gname = "root"
                info.mode = 0o755 if info.isdir() or info.name.endswith("/" + executable) else 0o644
                return info
            with tarfile.open(archive, "w:gz") as target:
                target.add(root, arcname=package_name, filter=permissions)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_name(archive.name + ".sha256").write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"Created {archive} ({archive.stat().st_size} bytes), SHA256 {digest}")
    return archive


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    package(args.build, args.output)
