#!python
import hashlib
import json
import shutil
import sys
import urllib.request
from pathlib import Path

URL = "https://github.com/LeagueToolkit/mimir/releases/latest/download/"
TABLES = ("binentries", "binhashes", "bintypes", "binfields", "game", "lcu")


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        while chunk := file.read(1 << 20):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "hashes"
    out.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(URL + "manifest.json") as response:
        manifest = json.load(response)
    manifest["tables"] = {name: manifest["tables"][name] for name in TABLES if name in manifest["tables"]}

    for table in manifest["tables"].values():
        name = table["file"]
        if Path(name).name != name:
            sys.exit(f"{name}: not a plain file name")
        path = out / name
        if path.exists() and sha256(path) == table["sha256"]:
            print(f"{name}: up to date")
            continue
        print(f"{name}: downloading")
        tmp = out / (name + ".tmp")
        with urllib.request.urlopen(URL + name) as response, open(tmp, "wb") as file:
            shutil.copyfileobj(response, file)
        if sha256(tmp) != table["sha256"]:
            tmp.unlink()
            sys.exit(f"{name}: checksum mismatch")
        tmp.replace(path)

    # manifest goes last, so a reader never sees it name a table that is not there yet
    tmp = out / "manifest.json.tmp"
    tmp.write_text(json.dumps(manifest, indent=2) + "\n")
    tmp.replace(out / "manifest.json")

    keep = {table["file"] for table in manifest["tables"].values()}
    for path in out.glob("*.lhdb"):
        if path.name not in keep:
            try:
                path.unlink()
            except OSError:
                pass


if __name__ == "__main__":
    main()
