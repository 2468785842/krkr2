"""Check Android package contents, ABI, JNI entry points and ELF closure."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile


def verify(apk, abi, readelf):
    ndk_machine = {"arm64-v8a": "AArch64", "x86_64": "Advanced Micro Devices X86-64"}
    with zipfile.ZipFile(apk) as package, tempfile.TemporaryDirectory() as directory:
        entries = set(package.namelist())
        required = {"classes.dex", "AndroidManifest.xml", "assets/locale/en_us.json",
                    "assets/NotoSansCJK-Regular.ttc"}
        if required - entries:
            raise ValueError(f"APK missing {sorted(required - entries)}")
        libraries = {entry for entry in entries if entry.startswith("lib/") and entry.endswith(".so")}
        if {entry.split("/")[1] for entry in libraries} != {abi}:
            raise ValueError(f"Unexpected APK ABI entries: {sorted(libraries)}")
        packaged = {Path(entry).name for entry in libraries}
        exported = {
            "libkrkr2.so": {
                "SDL_main", "JNI_OnLoad",
                "Java_org_github_krkr2_GameActivity_requestGameClose",
                "Java_org_libsdl_app_KrkrTextInput_commit",
            },
            "libkrkr2_settings.so": {
                "Java_org_github_krkr2_EngineSettings_read",
                "Java_org_github_krkr2_EngineSettings_save",
                "Java_org_github_krkr2_EngineSettings_checkLaunch",
            },
        }
        if required_libs := (set(exported) | {"libc++_shared.so"}) - packaged:
            raise ValueError(f"APK missing {sorted(required_libs)}")
        if any("cocos" in name.lower() for name in packaged):
            raise ValueError("APK contains a Cocos library")
        sysroot = Path(readelf).resolve().parent.parent / "sysroot/usr/lib"
        triple = {"arm64-v8a": "aarch64-linux-android", "x86_64": "x86_64-linux-android"}[abi]
        system = {file.name for file in (sysroot / triple / "28").glob("*.so")}

        for entry in sorted(libraries):
            path = Path(directory) / Path(entry).name
            path.write_bytes(package.read(entry))
            info = subprocess.check_output(
                [readelf, "--file-header", "--dynamic", "--dyn-syms", "--wide", path], text=True)
            if not re.search(rf"Machine:\s+{re.escape(ndk_machine[abi])}", info):
                raise ValueError(f"{path.name} has the wrong ELF machine")
            needed = set(re.findall(r"\(NEEDED\).*\[([^\]]+)\]", info))
            if missing := needed - packaged - system:
                raise ValueError(f"{path.name} missing dependencies {sorted(missing)}")
            defined = {line.split()[-1] for line in info.splitlines()
                       if re.match(r"\s*\d+:\s+\S+\s+\d+\s+", line) and " UND " not in line}
            if missing := exported.get(path.name, set()) - defined:
                raise ValueError(f"{path.name} missing exports {sorted(missing)}")
        print(f"{apk}: {abi}, {len(libraries)} native libraries, assets and JNI exports verified")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk", type=Path)
    parser.add_argument("abi", choices=["arm64-v8a", "x86_64"])
    parser.add_argument("readelf")
    args = parser.parse_args()
    verify(args.apk, args.abi, args.readelf)
