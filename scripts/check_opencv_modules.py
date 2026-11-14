"""Verify and report the actual OpenCV modules installed by vcpkg."""
import argparse
import json
from pathlib import Path
import re


def inspect(prefix, profile="imageops"):
    prefix = Path(prefix)
    exports = (prefix / "share/opencv4/OpenCVModules.cmake").read_text(encoding="utf-8")
    modules = set(re.findall(r"^add_library\((opencv_\w+) (?:STATIC|SHARED|UNKNOWN) IMPORTED\)", exports, re.M))
    header = (prefix / "include/opencv2/opencv_modules.hpp").read_text(encoding="utf-8")
    headers = {"opencv_" + name.lower() for name in re.findall(r"^#define HAVE_OPENCV_(\w+)\b", header, re.M)}
    if not modules or modules != headers:
        raise ValueError(f"OpenCV export/header modules differ: {sorted(modules)} / {sorted(headers)}")
    required = {"opencv_core", "opencv_imgproc"}
    if (profile == "imageops" and modules != required) or not required.issubset(modules):
        raise ValueError(f"Unexpected OpenCV modules for {profile}: {sorted(modules)}")
    version_header = (prefix / "include/opencv2/core/version.hpp").read_text(encoding="utf-8")
    version = ".".join(re.search(r"^#define CV_VERSION_" + part + r"\s+(\d+)", version_header, re.M)[1]
                       for part in ("MAJOR", "MINOR", "REVISION"))
    if version != "4.7.0":
        raise ValueError(f"Unexpected OpenCV version: {version}")
    status = (prefix.parent / "vcpkg/status").read_text(encoding="utf-8")
    features = set()
    for block in status.split("\n\n"):
        fields = dict(line.split(": ", 1) for line in block.splitlines() if ": " in line)
        if fields.get("Package") == "opencv4" and fields.get("Architecture") == prefix.name:
            if fields.get("Status") != "install ok installed":
                raise ValueError(f"OpenCV is not fully installed: {fields}")
            features.add(fields.get("Feature", "core"))
            if "Feature" not in fields and (fields.get("Version"), fields.get("Port-Version")) != ("4.7.0", "6"):
                raise ValueError(f"Unexpected OpenCV port identity: {fields}")
    if ("core" not in features or (("imageops" in features) != (profile == "imageops"))
            or (profile == "imageops" and features - {"core", "imageops", "openmp"})):
        raise ValueError(f"Unexpected OpenCV features for {profile}: {sorted(features)}")
    libraries, covered = [], set()
    # Android uses [debug/]sdk/native/staticlibs/<abi>; desktop uses lib/bin.
    for path in sorted(prefix.rglob("*opencv*")):
        if not path.is_file() or path.is_symlink() or not re.search(r"\.(?:a|lib|dll|so(?:\.\d+)*|dylib)$", path.name):
            continue
        module = next((name for name in modules if re.fullmatch(
            r"(?:lib)?" + re.escape(name) + r"(?:\d+d?|d)?(?:\.\d+)*\.(?:a|lib|dll|so(?:\.\d+)*|dylib)", path.name)), None)
        if module is None:
            raise ValueError(f"Unexported OpenCV library: {path}")
        covered.add(module)
        libraries.append({"path": path.relative_to(prefix).as_posix(), "bytes": path.stat().st_size})
    if covered != modules:
        raise ValueError(f"Missing installed OpenCV libraries: {sorted(modules - covered)}")
    return {"profile": profile, "version": version, "features": sorted(features), "modules": sorted(modules),
            "library_bytes": sum(item["bytes"] for item in libraries), "libraries": libraries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prefix", type=Path, help="vcpkg installed target-triplet directory")
    parser.add_argument("--profile", choices=("imageops", "baseline"), default="imageops")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = inspect(args.prefix, args.profile)
    data = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(data, encoding="utf-8")
    print(data, end="")


if __name__ == "__main__":
    main()
