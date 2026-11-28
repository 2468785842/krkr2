"""检查桌面收敛后的平台入口、依赖清单和生产源码。"""
import json
from pathlib import Path
import re
import subprocess


def check(root):
    errors = []
    names = subprocess.check_output(["git", "ls-files", "-z"], cwd=root).decode().split("\0")
    tracked = [name for name in names if name and (root / name).is_file()]
    manifest = json.loads((root / "vcpkg.json").read_text(encoding="utf-8"))
    if any(name in manifest["features"] for name in ("cocos", "crash-upload")):
        errors.append("旧宿主 feature 不得恢复")
    if "cocos2dx" in json.dumps(manifest):
        errors.append("vcpkg 清单仍包含 Cocos")
    presets = (root / "CMakePresets.json").read_text(encoding="utf-8")
    if re.search(r"cocos|KRKR2_SDL3_ONLY", presets, re.I):
        errors.append("构建预设仍包含旧宿主配置")
    for relative in ("vcpkg/ports/cocos2dx", "cpp/core/environ/cocos2d",
                     "cpp/core/environ/ui", "platforms/apple",
                     "cmake/CocosBuildHelpers.cmake"):
        if any(name == relative or name.startswith(relative + "/") for name in tracked):
            errors.append(f"旧宿主路径仍存在：{relative}")
    for name in tracked:
        if name.startswith("platforms/android/") and "cocos" in name.lower():
            errors.append(f"Android 路径仍包含旧宿主：{name}")
        if name.startswith("platforms/android/") and name.endswith(".gradle"):
            if re.search(r"\bcocos2dx?\b", (root / name).read_text(encoding="utf-8"), re.I):
                errors.append(f"Android Gradle 仍依赖旧宿主：{name}")
        if name.startswith(("cpp/", "platforms/", "cmake/")) or name == "CMakeLists.txt":
            path = root / name
            if not path.is_file() or path.suffix not in {".h", ".hpp", ".cpp", ".mm", ".cmake", ".txt"}:
                continue
            content = path.read_text(encoding="utf-8-sig")
            if path.suffix in {".cmake", ".txt"}:
                active = re.sub(r"#[^\n]*", "", content)
            else:
                active = re.sub(r"/\*.*?\*/|//[^\n]*", "", content, flags=re.S)
            if re.search(r"\bcocos2d(?:x)?\b|KRKR2_(?:SDL3_ONLY|COCOS_FREE|USE_COCOS_UI)|"
                         r'#\s*include\s*[<"](?:cocos/|(?:[^">]*/)?CC\w+\.h)', active, re.I):
                errors.append(f"生产源码或构建仍依赖旧宿主：{path.relative_to(root)}")
    return errors


if __name__ == "__main__":
    errors = check(Path(__file__).resolve().parents[1])
    if errors:
        raise SystemExit("\n".join(errors))
    print("桌面平台入口、vcpkg features 和生产源码检查通过")
