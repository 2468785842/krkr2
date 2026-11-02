"""检查 foundation 的显式 CMake 链接和直接平台包含；不声称消除 domain 层既有环。"""
import argparse
from pathlib import Path
import re
import sys

MODULES = {
    'core_log': 'log', 'tjs2': 'tjs2', 'core_msg': 'msg', 'core_io': 'io',
    'core_storage_archive': 'storage', 'archive_xp3': 'archive/xp3',
    'archive_zip': 'archive/zip', 'archive_7z': 'archive/7z', 'archive_tar': 'archive/tar',
    'core_imageops': 'imageops',
}
ALLOWED = {
    'core_log': set(), 'tjs2': {'core_log'}, 'core_msg': {'tjs2'},
    'core_io': {'tjs2', 'core_msg'}, 'core_storage_archive': {'tjs2', 'core_io', 'core_msg'},
    'core_imageops': set(),
    **{name: {'core_storage_archive'} for name in MODULES if name.startswith('archive_')},
}


def check(root):
    errors = []
    for target, relative in MODULES.items():
        directory = root / 'cpp/core' / relative
        cmake = directory / 'CMakeLists.txt'
        if not cmake.is_file():
            errors.append(f'缺少 {cmake.relative_to(root)}')
            continue
        text = re.sub(r'#[^\n]*', '', cmake.read_text(encoding='utf-8'))
        project = re.search(r'project\s*\(\s*([^\s)]+)', text, re.I)
        if project:
            text = text.replace('${PROJECT_NAME}', project[1])
        links = [body.split()[1:] for body in re.findall(r'target_link_libraries\s*\((.*?)\)', text, re.S | re.I)
                 if body.split() and body.split()[0] == target]
        if not links:
            errors.append(f'{target}: 未找到显式链接声明')
        for dependency in (token.strip('"') for link in links for token in link):
            internal = dependency == 'tjs2' or dependency.startswith(('core_', 'archive_'))
            if internal and dependency not in ALLOWED[target]:
                errors.append(f'{target} 不得依赖 {dependency}')
            if 'cocos' in dependency.lower() or '${' in dependency or '$<' in dependency:
                errors.append(f'{target}: 需复核链接表达式 {dependency}')
        # storage 同目录还包含完整引擎实现，仅检查共享归档源码。
        if target == 'core_storage_archive':
            sources = list((directory / 'impl').glob('Archive*.*'))
            sources += [directory / 'impl' / name for name in ('MemoryStream.cpp', 'StorageGlobals.cpp')]
        else:
            sources = [p for p in directory.rglob('*') if p.suffix in ('.cpp', '.h', '.hpp')]
        for source in sources:
            content = source.read_text(encoding='utf-8')
            includes = re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)', content, re.M)
            for include in includes:
                if include.startswith(('cocos/', 'unzip/', 'environ/', 'visual/', 'movie/', 'sound/', 'plugin/')) or 'cocos2d' in include:
                    errors.append(f'{source.relative_to(root)}: 反向包含 {include}')
            if target == 'core_imageops' and source.name != 'ImageOps_opencv.cpp':
                active = re.sub(r'/\*.*?\*/|//[^\n]*', '', content, flags=re.S)
                if re.search(r'#\s*include\s*[<"]opencv|\bcv::', active):
                    errors.append(f'{source.relative_to(root)}: OpenCV 仅允许出现在后端实现')
    # 渲染器及插件只通过 ImageOps 使用图像算法，OpenCV 留在后端实现。
    for relative in ('visual', '../plugins'):
        for source in (root / 'cpp/core' / relative).rglob('*'):
            if source.suffix not in ('.cpp', '.h', '.hpp'):
                continue
            content = re.sub(r'/\*.*?\*/|//[^\n]*', '', source.read_text(encoding='utf-8'), flags=re.S)
            if re.search(r'#\s*include\s*[<"]opencv|\bcv::', content):
                errors.append(f'{source.relative_to(root)}: OpenCV 调用应放入 ImageOps 后端')
    return errors


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    errors = check(parser.parse_args().root.resolve())
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print(f'{len(MODULES)} 个 foundation 目标的显式链接及直接平台包含检查通过；未检查 domain 环或传递头文件。')
    return 0


if __name__ == '__main__':
    sys.exit(main())
