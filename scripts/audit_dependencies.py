"""Inventory an actual vcpkg SDK and CMake build; no unused-code inference."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path, PurePosixPath
import re


def paragraphs(text):
    result = []
    for block in re.split(r"\r?\n\s*\r?\n", text.strip()):
        fields = {}
        key = None
        for line in block.splitlines():
            if line.startswith((" ", "\t")) and key:
                fields[key] += " " + line.strip()
            elif ": " in line:
                key, value = line.split(": ", 1)
                fields[key] = value
        if fields:
            result.append(fields)
    return result


def installed_ports(text):
    ports = {}
    for fields in paragraphs(text):
        if fields.get("Status") != "install ok installed":
            continue
        name, arch = fields["Package"], fields["Architecture"]
        key = name + ":" + arch
        port = ports.setdefault(key, {"name": name, "triplet": arch,
                                     "version": fields.get("Version", ""),
                                     "features": set(), "dependencies": set()})
        if "Version" in fields:
            port["version"] = fields["Version"]
        if "Feature" in fields:
            port["features"].add(fields["Feature"])
        for dependency in fields.get("Depends", "").split(","):
            dependency = dependency.strip()
            if dependency:
                # Preserve architecture qualifiers; strip version constraints.
                port["dependencies"].add(dependency.split(" ", 1)[0])
    return ports


def footprint(sdk, port):
    manifests = list((sdk / "vcpkg/info").glob(
        f"{port['name']}_*_{port['triplet']}.list"))
    if len(manifests) != 1:
        raise ValueError(f"Expected one installed file list for {port['name']}: {manifests}")
    sizes = defaultdict(int)
    files, missing = 0, []
    for name in sorted(set(manifests[0].read_text(encoding="utf-8").splitlines())):
        path = PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts or "\\" in name or ":" in name:
            raise ValueError(f"Invalid SDK inventory path: {name}")
        if name.endswith("/") or not path.parts:
            continue
        local = sdk.joinpath(*path.parts)
        if not local.exists() and not local.is_symlink():
            missing.append(name)
            continue
        if local.is_file() or local.is_symlink():
            sizes[path.parts[1] if len(path.parts) > 1 else "other"] += local.lstat().st_size
            files += 1
    return {"bytes_by_directory": dict(sorted(sizes.items())),
            "installed_bytes": sum(sizes.values()), "file_count": files,
            "missing_files": missing}


def read_cache(path):
    cache = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith(("#", "//")) and ":" in line and "=" in line:
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    return cache


def inspect(build):
    cache = read_cache(build / "CMakeCache.txt")
    sdk = Path(cache["VCPKG_INSTALLED_DIR"])
    if not sdk.is_absolute():
        sdk = build / sdk
    sdk = sdk.resolve()
    ports = installed_ports((sdk / "vcpkg/status").read_text(encoding="utf-8"))
    if not ports:
        raise ValueError("SDK contains no installed port records")
    reverse = defaultdict(set)
    for key, port in ports.items():
        for dependency in port["dependencies"]:
            candidates = [dependency] if ":" in dependency else [dependency + ":" + port["triplet"]]
            if candidates[0] not in ports:
                candidates = [k for k, p in ports.items() if p["name"] == dependency]
            for candidate in candidates:
                if candidate in ports:
                    reverse[candidate].add(key)
    inventory = []
    for key, port in sorted(ports.items()):
        inventory.append({**port, "features": sorted(port["features"]),
                          "dependencies": sorted(port["dependencies"]),
                          "required_by": sorted(reverse[key]), **footprint(sdk, port)})
    commands = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    if not commands:
        raise ValueError("Compilation database is empty")
    openmp = []
    for command in commands:
        flags = command.get("command", " ".join(command.get("arguments", [])))
        if re.search(r"(?:^|\s)(?:-fopenmp(?:=\w+)?|/openmp(?::\w+)?)(?:\s|$)", flags):
            openmp.append(command["file"])
    engine = build / "bin/krkr2" / ("krkr2.exe" if cache.get("WINDOWS") in {"ON", "TRUE", "1"} else "krkr2")
    manifest = Path(cache["CMAKE_HOME_DIRECTORY"]) / "vcpkg.json"
    return {"build": str(build.resolve()), "sdk": str(sdk),
            "scope": cache.get("KRKR2_TEST_SCOPE"),
            "sdl3_only": cache.get("KRKR2_SDL3_ONLY"),
            "manifest_features_declared": json.loads(manifest.read_text(encoding="utf-8"))["features"],
            "port_count": len(inventory),
            "sdk_file_bytes": sum(p["installed_bytes"] for p in inventory),
            "compilation_units": len(commands), "openmp_sources": sorted(openmp),
            "engine": {"bytes": engine.stat().st_size,
                       "sha256": hashlib.sha256(engine.read_bytes()).hexdigest()} if engine.is_file() else None,
            "ports": inventory,
            "limits": ["Installed SDK may contain packages from earlier configurations.",
                       "File sizes are logical inventory bytes, not disk blocks or runtime package size.",
                       "Dependencies combine installed port features; source/API consumers need separate review.",
                       "Declared manifest features are not a resolved platform installation plan."]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = inspect(args.build)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"{report['port_count']} ports, {report['sdk_file_bytes']} SDK file bytes, "
          f"{report['compilation_units']} compilation units; report: {args.output}")


if __name__ == "__main__":
    main()
