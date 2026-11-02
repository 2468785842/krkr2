"""Copy the selected runtime resource set; prune only explicitly excluded files."""
import argparse
from pathlib import Path, PurePosixPath
import shutil


def entries(manifest):
    result = set()
    for line in manifest.read_text(encoding="utf-8").splitlines():
        if not line:
            continue
        path = PurePosixPath(line)
        if path.is_absolute() or ".." in path.parts or "\\" in line or ":" in line:
            raise ValueError(f"Invalid resource path: {line}")
        result.add(line)
    return result


def within(root, name):
    path = root.joinpath(*PurePosixPath(name).parts)
    path.resolve().relative_to(root.resolve())
    return path


def sync(source, destination, all_files, selected):
    if not selected <= all_files:
        raise ValueError("Selected resources must belong to the complete resource set")
    # Validate the complete operation before copying or pruning anything.
    for name in all_files:
        src, dst = within(source, name), within(destination, name)
        if not src.is_file():
            raise ValueError(f"Missing resource: {src}")
        if dst.exists() and not dst.is_file():
            raise ValueError(f"Resource destination is not a file: {dst}")
    for name in sorted(selected):
        src, dst = within(source, name), within(destination, name)
        dst.parent.mkdir(parents=True, exist_ok=True)
        if not dst.exists() or (src.stat().st_size, src.stat().st_mtime_ns) != (dst.stat().st_size, dst.stat().st_mtime_ns):
            shutil.copy2(src, dst)
    for name in sorted(all_files - selected):
        dst = within(destination, name)
        if dst.exists():
            dst.unlink()
    print(f"Runtime assets: {len(selected)}/{len(all_files)} selected")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--all", type=Path, required=True)
    parser.add_argument("--selected", type=Path, required=True)
    args = parser.parse_args()
    sync(args.source, args.destination, entries(args.all), entries(args.selected))


if __name__ == "__main__":
    main()
