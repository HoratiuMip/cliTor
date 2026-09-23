"""
CAUTION: This file is MOSTLY GENERATED, MOSTLY UNCHECKED black box.

NAME: cliTor//makepie - configure, build and install cliTor via a friendly python interface.

MANUAL: Run this script using "<your_python_interpreter> make.py" or "make gui".
        Select the desired built-in junctions via the checkboxes. Optionally, add your external
          junctions in the text box, one per line, "<junction_name> <path_to_junction>".
        Configure, Build and Run the project.

        The current status is displayed above the buttons. The buttons are BLOCKING the GUI, so be
          patient. If any commands (config, build, etc.) exit unsuccessfully, their STDOUT and STDERR
          will be printed to the console.

        Even though the GUI is blocked during command execution, you can click the other buttons and they
          will execute after the current command is finished.
    
AUTHORS(s): Claude Opus 5
            Vatca "Mipsan" Tudor-Horatiu
"""

import os
import sys
import subprocess
import platform

# from functools import partial as fnc_bind

import json
from pathlib import Path

import tkinter as tk

#————————————————————————— Config ———————————————————————————#
JUNCTIONS_DIR = Path("./src/junctions")
CONFIG_FILE   = Path("./junctions.json")
C             = {
    "bg":        "#070b12",   # window
    "panel":     "#0c1119",   # list / text panels
    "panel_hi":  "#111827",   # hovered row
    "inset":     "#05080d",   # deepest inset (textbox)
    "border":    "#1b2942",
    "cyan":      "#22d3ee",
    "magenta":   "#ff2bd1",
    "gold":      "#ffc14d",
    "text":      "#c3d2e6",
    "muted":     "#4e5f7a",
}
MAKES_TO_SEARCH = ["make", "mingw32-make"]
ENV_FORMAT_WIDTH = 32

def pick_font():
    """Pick the first available monospace family."""
    from tkinter import font as tkfont
    available = set(tkfont.families())
    for name in ("JetBrains Mono", "Fira Code", "DejaVu Sans Mono",
                 "Liberation Mono", "Consolas", "Menlo", "Courier New"):
        if name in available:
            return name
    return "TkFixedFont"

def fmt_kdv(key, val):
    return (key + ' ').ljust(ENV_FORMAT_WIDTH - len(val), '.') + ' ' + val

#————————————————————————— Data —————————————————————————————#
def discover_short_junctions():
    """Foldes directly under ./src/junctions, NOT recursive."""
    if not JUNCTIONS_DIR.is_dir():
        return []
    return sorted(
        entry.name for entry in JUNCTIONS_DIR.iterdir()
        if entry.is_dir() and not entry.name.startswith(".")
    )

def load_config():
    """Return (set_of_short_names, list_of_long_paths) from junctions.json."""
    short, long = set(), []
    if not CONFIG_FILE.is_file():
        return short, long
    try:
        with CONFIG_FILE.open(encoding="utf-8") as fh:
            data = json.load(fh)
    except (json.JSONDecodeError, OSError) as err:
        print(f"[cliTor] could not read {CONFIG_FILE}: {err}", file=sys.stderr)
        return short, long

    for entry in data.get("selected", []):
        path = entry.get("path", "") if isinstance(entry, dict) else str(entry)
        path = path.strip()
        if not path:
            continue
        if "/" in path or "\\" in path:
            name = entry.get("name", "") if isinstance(entry, dict) else str(entry)
            name = name.strip()
            long.append(' '.join([name, path])) # long junction -> absolute path
        else:
            short.add(path) # short junction -> folder name
    return short, long

def write_config(short_names, typed_lines):
    selected = [{"path": name} for name in short_names]
    for line in typed_lines:
        name, path = line.strip().split(' ', 1)
        if not name or not path:
            continue
        selected.append({"name": str(name), "path": str(Path(path).expanduser().resolve())})

    with CONFIG_FILE.open("w", encoding="utf-8") as fh:
        json.dump({"selected": selected}, fh, indent=2)
        fh.write("\n")
    return len(selected)

#————————————————————————— Widgets ———————————————————————————#
class JunctionRow(tk.Frame):
    """One checkbox row, drawn by hand so it matches the theme."""

    BOX = 15

    def __init__(self, master, name, checked, font):
        super().__init__(master, bg=C["panel"], cursor="hand2")
        self.var = tk.BooleanVar(value=checked)
        self.name = name

        self.box = tk.Canvas(self, width=self.BOX + 1, height=self.BOX + 1,
                             bg=C["panel"], highlightthickness=0, cursor="hand2")
        self.box.pack(side="left", padx=(14, 10), pady=6)

        self.label = tk.Label(self, text=name, font=(font, 11), anchor="w",
                              bg=C["panel"], fg=C["text"], cursor="hand2")
        self.label.pack(side="left", fill="x", expand=True, pady=6)

        for widget in (self, self.box, self.label):
            widget.bind("<Button-1>", self.toggle)
            widget.bind("<Enter>", self._enter)
            widget.bind("<Leave>", self._leave)

        self._draw()

    def _draw(self):
        self.box.delete("all")
        b = self.BOX
        self.box.create_rectangle(1, 1, b, b, outline=C["cyan"], width=1)
        if self.var.get():
            self.box.create_line(4, b // 2, b // 2 - 1, b - 4,
                                 fill=C["magenta"], width=2)
            self.box.create_line(b // 2 - 1, b - 4, b - 3, 4,
                                 fill=C["magenta"], width=2)
            self.label.configure(fg=C["gold"])
        else:
            self.label.configure(fg=C["text"])

    def toggle(self, _event=None):
        self.var.set(not self.var.get())
        self._draw()

    def _enter(self, _e):
        for w in (self, self.box, self.label):
            w.configure(bg=C["panel_hi"])

    def _leave(self, _e):
        for w in (self, self.box, self.label):
            w.configure(bg=C["panel"])

    @property
    def checked(self):
        return self.var.get()

class NeonButton(tk.Canvas):
    """Flat outlined button that inverts on hover."""

    def __init__(self, master, text, accent, command, font, width=150, height=40):
        super().__init__(master, width=width, height=height, bg=C["bg"],
                         highlightthickness=0, cursor="hand2")
        self.accent = accent
        self.command = command
        self.w, self.h = width, height

        self.rect = self.create_rectangle(1, 1, width - 2, height - 2,
                                          outline=accent, width=1, fill=C["bg"])
        self.text = self.create_text(width // 2, height // 2, text=text,
                                     fill=accent, font=(font, 11, "bold"))

        self.bind("<Button-1>", lambda _e: self.command())
        self.bind("<Enter>", self._enter)
        self.bind("<Leave>", self._leave)

    def _enter(self, _e):
        self.itemconfig(self.rect, fill=self.accent)
        self.itemconfig(self.text, fill=C["bg"])

    def _leave(self, _e):
        self.itemconfig(self.rect, fill=C["bg"])
        self.itemconfig(self.text, fill=self.accent)

def rule(master, color, pad=(0, 0)):
    """Horizontal separator line."""
    line = tk.Frame(master, height=1, bg=color)
    line.pack(fill="x", pady=pad)
    return line

#————————————————————————— Interface —————————————————————————#
class Interface(tk.Tk):
    def __init__(self):
        self.MAKE = ""
        for make in MAKES_TO_SEARCH:
            try:
                if subprocess.run([make, "--version"]).returncode == 0x0:
                    self.MAKE = make
                    break
            except:
                continue

        super().__init__()
        self.title("cliTor//makepie")
        self.configure(bg=C["bg"])
        self.geometry("640x840")
        self.minsize(440, 640)

        self.font = pick_font()
        self.rows = []

        short_selected, long_paths = load_config()
        self._build_header()
        self._build_list(short_selected)
        self._build_textbox(long_paths)
        self._build_footer()

    def _build_header(self):
        head = tk.Frame(self, bg=C["bg"])
        head.pack(fill="x", padx=22, pady=(20, 0))

        tk.Label(head, text="cliTor//makepie", font=(self.font, 19, "bold"), bg=C["bg"], fg=C["cyan"]).pack(anchor="w")
        rule(head, C["magenta"], pad=(10, 0))

        tk.Label(head, text="Environment", font=(self.font, 10), bg=C["bg"], fg=C["gold"]).pack(anchor="w", pady=(8, 0))
        tk.Label(head, text=fmt_kdv("OS:", platform.system()), font=(self.font,10), bg=C["bg"], fg=C["text"]).pack(anchor="w", pady=(8, 0))
        tk.Label(head, text=fmt_kdv("Make:", self.MAKE), font=(self.font,10), bg=C["bg"], fg=C["text"]).pack(anchor="w", pady=(8, 0))
        rule(head, C["magenta"], pad=(10, 0))

        txt = "Built-in junctions:" if JUNCTIONS_DIR.is_dir() else f"{JUNCTIONS_DIR} (missing)"
        tk.Label(head, text=txt, font=(self.font, 10), bg=C["bg"], fg=C["gold"]).pack(anchor="w", pady=(8, 0))
        
    def _build_list(self, selected):
        names = discover_short_junctions()

        wrap = tk.Frame(self, bg=C["border"], highlightthickness=0)
        wrap.pack(fill="both", expand=True, padx=22, pady=(12, 0))

        inner = tk.Frame(wrap, bg=C["panel"])
        inner.pack(fill="both", expand=True, padx=1, pady=1)

        canvas = tk.Canvas(inner, bg=C["panel"], highlightthickness=0)
        bar = tk.Scrollbar(inner, orient="vertical", command=canvas.yview,
                           bg=C["panel"], troughcolor=C["inset"],
                           activebackground=C["cyan"], borderwidth=0,
                           highlightthickness=0, width=10)
        holder = tk.Frame(canvas, bg=C["panel"])

        holder.bind("<Configure>",
                    lambda _e: canvas.configure(scrollregion=canvas.bbox("all")))
        window = canvas.create_window((0, 0), window=holder, anchor="nw")
        canvas.bind("<Configure>",
                    lambda e: canvas.itemconfig(window, width=e.width))
        canvas.configure(yscrollcommand=bar.set)

        canvas.pack(side="left", fill="both", expand=True, pady=6)
        bar.pack(side="right", fill="y", pady=6)

        def wheel(event):
            step = -1 if getattr(event, "delta", 0) > 0 or event.num == 4 else 1
            canvas.yview_scroll(step, "units")

        for seq in ("<MouseWheel>", "<Button-4>", "<Button-5>"):
            canvas.bind_all(seq, wheel)

        if not names:
            tk.Label(holder,
                     text="No junction folders found.\nCreate one under "
                          f"{JUNCTIONS_DIR} to get started.",
                     font=(self.font, 10), bg=C["panel"], fg=C["muted"],
                     justify="left").pack(anchor="w", padx=14, pady=16)
            return

        for index, name in enumerate(names):
            row = JunctionRow(holder, name, name in selected, self.font)
            row.pack(fill="x")
            self.rows.append(row)
            if index < len(names) - 1:
                tk.Frame(holder, height=1, bg=C["inset"]).pack(fill="x", padx=12)

    def _build_textbox(self, long_paths):
        block = tk.Frame(self, bg=C["bg"])
        block.pack(fill="x", padx=22, pady=(16, 0))

        rule(block, C["magenta"])
        tk.Label(block, text="Other junctions: <name> <path>", font=(self.font, 10), bg=C["bg"], fg=C["gold"]).pack(anchor="w", pady=(10, 6))

        frame = tk.Frame(block, bg=C["border"])
        frame.pack(fill="x")

        self.text = tk.Text(
            frame, height=6, font=(self.font, 10),
            bg=C["inset"], fg=C["text"],
            insertbackground=C["magenta"],
            selectbackground=C["border"],
            selectforeground=C["cyan"],
            relief="flat", padx=10, pady=8, wrap="none",
            highlightthickness=0
        )
        self.text.pack(fill="x", padx=1, pady=1)

        if long_paths:
            self.text.insert("1.0", "\n".join(long_paths))

    def _build_footer(self):
        block = tk.Frame(self, bg=C["bg"])
        block.pack(fill="x", padx=22, pady=(14, 18))

        rule(block, C["magenta"], pad=(0, 12))

        self.status = tk.Label(block, text="", font=(self.font, 9), bg=C["bg"], fg=C["muted"], anchor="w")
        self.status.pack(fill="x", pady=(0, 10))

        bar1 = tk.Frame(block, bg=C["bg"])
        bar1.pack(fill="x")
        NeonButton(bar1, "SELECT", C["cyan"], self.on_select, self.font).pack(side="left")
        NeonButton(bar1, "CONFIG", C["cyan"], self.on_config, self.font).pack(side="left")
        NeonButton(bar1, "QUIT", C["magenta"], self.destroy, self.font, width=110).pack(side="right")

        bar2 = tk.Frame(block, bg=C["bg"])
        bar2.pack(fill="x")
        NeonButton(bar2, "BUILD", C["cyan"], self.on_build, self.font).pack(side="left")
        NeonButton(bar2, "RUN", C["cyan"], self.on_run, self.font).pack(side="left")

    def on_select(self):
        checked = [row.name for row in self.rows if row.checked]
        typed = self.text.get("1.0", "end").splitlines()
        try:
            count = write_config(checked, typed)
        except OSError as err:
            self.status.configure(text=f"Write failed: {err}", fg=C["magenta"])
            return
        self.status.configure(
            text=f"Wrote {CONFIG_FILE} — {count} junction(s).", fg=C["gold"])

    def on_config(self):
        cmake_args = [
            f"-DRGH_TARGET_OS={platform.system()}"
        ]

        self.status.configure(text="Config...",fg=C["gold"])
        self.update_idletasks()

        ret = subprocess.run(["make", "config", f"CMAKE_PASS=\"{','.join(cmake_args)}\""], capture_output=True, text=True)

        if ret.returncode == 0x0:
            self.status.configure(text=f"Config — ok.", fg=C["gold"])
        else:
            self.status.configure(text=f"Config — error, exit code: {ret.returncode}.", fg=C["magenta"])
            print(f"[cliTor//makepie] STDOUT of command:\n{ret.stdout}\n\n[cliTor//makepie] STDERR of command:\n{ret.stderr}\n")

    def on_build(self):
        self.status.configure(text="Build...",fg=C["gold"])
        self.update_idletasks()

        ret = subprocess.run(["make"], capture_output=True, text=True)

        if ret.returncode == 0x0:
            self.status.configure(text=f"Build — ok.", fg=C["gold"])
        else:
            self.status.configure(text=f"Build — error, exit code: {ret.returncode}.", fg=C["magenta"])
            print(f"[cliTor//makepie] STDOUT of command:\n{ret.stdout}\n\n[cliTor//makepie] STDERR of command:\n{ret.stderr}\n")

    def on_run(self):
        self.status.configure(text="Run...",fg=C["gold"])
        self.update_idletasks()
        proc = subprocess.Popen(["make", "run"])
        ret = proc.wait()
        self.status.configure(text=f"Run — process returned {ret}.", fg=C["gold"] if ret == 0x0 else C["magenta"] )

if __name__ == "__main__":
    Interface().mainloop()