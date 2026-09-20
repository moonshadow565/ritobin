#!python
import hashlib
import json
import sys
import tkinter
import urllib.request
from pathlib import Path
from tkinter import filedialog, messagebox

URL = "https://github.com/LeagueToolkit/mimir/releases/latest/download/"
TABLES = ("binentries", "binhashes", "bintypes", "binfields", "game", "lcu")
TITLE = "ritobin hashes"


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        while chunk := file.read(1 << 20):
            digest.update(chunk)
    return digest.hexdigest()


def update(out, show):
    show("fetching manifest")
    with urllib.request.urlopen(URL + "manifest.json") as response:
        manifest = json.load(response)
    manifest["tables"] = {name: manifest["tables"][name] for name in TABLES if name in manifest["tables"]}

    for table in manifest["tables"].values():
        name = table["file"]
        if Path(name).name != name:
            raise ValueError(f"{name}: not a plain file name")
        path = out / name
        show(f"{name}: checking")
        if path.exists() and sha256(path) == table["sha256"]:
            continue
        tmp = out / (name + ".tmp")
        with urllib.request.urlopen(URL + name) as response, open(tmp, "wb") as file:
            done = 0
            while chunk := response.read(1 << 16):
                file.write(chunk)
                done += len(chunk)
                show(f"{name}: {done >> 20} of {table['size_bytes'] >> 20} MB")
        if sha256(tmp) != table["sha256"]:
            tmp.unlink()
            raise ValueError(f"{name}: checksum mismatch")
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


def main():
    root = tkinter.Tk()
    root.title(TITLE)
    root.withdraw()
    hashes = Path(__file__).parent / "hashes"
    start = hashes if hashes.is_dir() else hashes.parent
    out = sys.argv[1] if len(sys.argv) > 1 else filedialog.askdirectory(title=TITLE, initialdir=start, mustexist=True)
    if not out:
        return
    out = Path(out)
    if not out.is_dir():
        messagebox.showerror(TITLE, f"{out}\nis not a folder")
        return

    # no console under pythonw, the window is the only place progress can go
    label = tkinter.Label(root, width=48, padx=16, pady=16)
    label.pack()
    root.protocol("WM_DELETE_WINDOW", sys.exit)
    root.deiconify()

    def show(text):
        label.config(text=text)
        root.update()

    try:
        update(out, show)
    except Exception as error:
        messagebox.showerror(TITLE, str(error))
        return
    messagebox.showinfo(TITLE, f"updated\n{out}")


if __name__ == "__main__":
    main()
