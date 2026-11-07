"""Verify a complete Debug runtime's CMake identity and packaged ELF closure."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zipfile


ARCHITECTURES = {
    "x86_64": (62, "x64-android", "x86_64-linux-android"),
    "arm64-v8a": (183, "arm64-android", "aarch64-linux-android"),
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def cache_values(path):
    result = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith(("#", "//")) and "=" in line and ":" in line.split("=", 1)[0]:
            name, value = line.split("=", 1)
            result[name.split(":", 1)[0]] = value
    return result


def verify(abi):
    machine, triplet, triple = ARCHITECTURES[abi]
    root = Path.cwd()
    matches = []
    for path in Path("out/android/cmake-build").rglob("CMakeCache.txt"):
        cache = cache_values(path)
        if cache.get("ANDROID_ABI") == abi and Path(cache.get("CMAKE_HOME_DIRECTORY", "")).resolve() == root:
            matches.append((path, cache))
    if len(matches) != 1:
        raise ValueError(f"Expected one application CMake cache for {abi}, got {[str(p) for p, _ in matches]}")
    path, cache = matches[0]
    expected = {"CMAKE_BUILD_TYPE": "Debug", "KRKR2_BUILD_APP": "ON", "ENABLE_TESTS": "OFF",
                "BUILD_TOOLS": "OFF", "KRKR2_ENABLE_PSD": "ON", "KRKR2_ENABLE_CRASH_UPLOAD": "OFF",
                "KRKR2_ENABLE_UI_TEST_DRIVER": "OFF", "VCPKG_TARGET_TRIPLET": triplet}
    for key, value in expected.items():
        if cache.get(key) != value:
            raise ValueError(f"Unexpected {key}: {cache.get(key)!r} != {value!r}")
    commands = json.loads((path.parent / "compile_commands.json").read_text(encoding="utf-8"))
    sources = [c["file"].replace("\\", "/") for c in commands]
    for name in ("platforms/android/cpp/krkr2_android.cpp", "cpp/core/input/TextInput.cpp",
                 "cpp/core/environ/cocos2d/MainScene.cpp", "cpp/core/visual/RenderManager.cpp",
                 "cpp/core/visual/ogl/RenderManager_ogl.cpp", "cpp/core/imageops/ImageOps.cpp",
                 "cpp/core/imageops/ImageOps_opencv.cpp"):
        if sum(s.endswith("/" + name) for s in sources) != 1:
            raise ValueError(f"Application compilation is missing/duplicates {name}")
    status = (path.parent / "vcpkg_installed/vcpkg/status").read_text(encoding="utf-8")
    ports = {line.split(": ", 1)[1] for line in status.splitlines() if line.startswith("Package: ")}
    if not {"cocos2dx", "ffmpeg", "opencv4", "breakpad", "boost-spirit"}.issubset(ports):
        raise ValueError(f"Missing full runtime dependencies: {sorted(ports)}")
    if ports.intersection({"catch2", "argparse"}):
        raise ValueError("Android runtime unexpectedly installs desktop tests/tools")
    ndk = Path(os.environ["ANDROID_NDK"])
    toolchain = ndk / "toolchains/llvm/prebuilt/linux-x86_64"
    readelf = toolchain / "bin/llvm-readelf"
    system = {p.name for p in (toolchain / "sysroot/usr/lib" / triple / "29").glob("*.so")}
    if not system:
        raise ValueError("Cannot identify Android API 29 system library stubs")
    apks = list(Path("platforms/android/out/android/app/outputs/apk/debug").glob("*.apk"))
    if len(apks) != 1:
        raise ValueError(f"Expected one ABI-specific runtime APK, got {apks}")
    apk = apks[0]
    libraries = {}
    with zipfile.ZipFile(apk) as package, tempfile.TemporaryDirectory() as directory:
        names = package.namelist()
        if not {"AndroidManifest.xml", "classes.dex", "resources.arsc"}.issubset(names):
            raise ValueError("Incomplete application APK")
        packaged_abis = {name.split("/")[1] for name in names if name.startswith("lib/") and name.endswith(".so")}
        if packaged_abis != {abi}:
            raise ValueError(f"Unexpected APK ABIs: {packaged_abis}")
        native = {name.rsplit("/", 1)[1]: name for name in names if name.startswith(f"lib/{abi}/") and name.endswith(".so")}
        if not {"libkrkr2.so", "libc++_shared.so"}.issubset(native):
            raise ValueError(f"Missing runtime/STL libraries: {native}")
        for name, entry in native.items():
            data = package.read(entry)
            if data[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", data, 18)[0] != machine:
                raise ValueError(f"Wrong ELF architecture: {entry}")
            elf = Path(directory) / name
            elf.write_bytes(data)
            dynamic = subprocess.check_output([str(readelf), "-d", str(elf)], text=True)
            needed = re.findall(r"\(NEEDED\).*Shared library: \[([^]]+)\]", dynamic)
            missing = set(needed) - native.keys() - system
            if missing:
                raise ValueError(f"{entry} needs unpackaged libraries: {sorted(missing)}")
            libraries[name] = {"size": len(data), "sha256": digest(data), "needed": needed}
    return {"source": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
            "abi": abi, "cmake_cache": str(path), "configuration": expected,
            "ports": sorted(ports), "project_sources": len(sources),
            "apk": str(apk), "apk_sha256": digest(apk.read_bytes()), "apk_size": apk.stat().st_size,
            "libraries": libraries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("abi", choices=ARCHITECTURES)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = verify(args.abi)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
