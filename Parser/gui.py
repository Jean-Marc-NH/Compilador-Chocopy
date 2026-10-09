#!/usr/bin/env python3
"""Vista educativa LR(1). Todo el analisis se ejecuta en el motor C++17."""
import argparse
import json
import os
from pathlib import Path
import queue
import shlex
import subprocess
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

HERE = Path(__file__).resolve().parent
DEFAULT = (HERE / "examples" / "parentheses.grammar").read_text(encoding="utf-8")


def build_engine():
    binary = HERE / "build" / ("lr1.exe" if os.name == "nt" else "lr1")
    sources = [HERE / name for name in ("main.cpp", "lr1.cpp", "lr1.hpp")]
    if not binary.exists() or any(p.stat().st_mtime > binary.stat().st_mtime for p in sources):
        binary.parent.mkdir(exist_ok=True)
        command = shlex.split(os.environ.get("CXX", "g++"))
        subprocess.run(command + ["-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
                                  str(sources[0]), str(sources[1]), "-o", str(binary)],
                       check=True, capture_output=True, text=True, encoding="utf-8", timeout=120)
    return binary


def run_engine(grammar, tokens, max_states, max_steps, analyze):
    binary = build_engine()
    command = [str(binary), "--json", "--max-states", str(max_states), "--max-steps", str(max_steps)]
    if analyze:
        command += ["--input", tokens]
    process = subprocess.run(command, input=grammar, capture_output=True, text=True,
                             encoding="utf-8", timeout=30)
    if process.returncode not in (0, 2, 3):
        raise RuntimeError(process.stderr.strip() or "El motor no pudo completar el analisis.")
    return json.loads(process.stdout)


class Table(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.tree = ttk.Treeview(self, show="headings", selectmode="browse")
        y = ttk.Scrollbar(self, orient="vertical", command=self.tree.yview)
        x = ttk.Scrollbar(self, orient="horizontal", command=self.tree.xview)
        self.tree.configure(yscrollcommand=y.set, xscrollcommand=x.set)
        self.tree.grid(row=0, column=0, sticky="nsew")
        y.grid(row=0, column=1, sticky="ns")
        x.grid(row=1, column=0, sticky="ew")
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.tree.tag_configure("accept", background="#dcf4e7", foreground="#155b3c")

    def fill(self, columns, rows, tags=None, widths=None):
        self.tree.delete(*self.tree.get_children())
        ids = [f"c{i}" for i in range(len(columns))]
        self.tree.configure(columns=ids)
        for i, (key, title) in enumerate(zip(ids, columns)):
            self.tree.heading(key, text=title)
            width = widths[i] if widths else (90 if i else 80)
            self.tree.column(key, width=width, minwidth=65, stretch=False, anchor="w")
        for i, row in enumerate(rows):
            self.tree.insert("", "end", values=row, tags=(tags[i],) if tags and tags[i] else ())


class App:
    PAGE_SIZE = 20

    def __init__(self, root):
        self.root = root
        self.data = None
        self.snapshot = None
        self.busy = False
        self.page = 0
        self.messages = queue.Queue()
        root.title("Laboratorio LR(1) · ChocoPy")
        root.geometry("1400x900")
        root.minsize(900, 700)
        style = ttk.Style(root)
        if "clam" in style.theme_names():
            style.theme_use("clam")
        style.configure("Treeview", rowheight=29, font=("DejaVu Sans Mono", 10))
        style.configure("Treeview.Heading", font=("DejaVu Sans", 10, "bold"))
        style.configure("Title.TLabel", font=("DejaVu Sans", 18, "bold"))
        outer = ttk.Frame(root, padding=14)
        outer.pack(fill="both", expand=True)
        ttk.Label(outer, text="Laboratorio LR(1)", style="Title.TLabel").pack(anchor="w")
        ttk.Label(outer, text="Gramática → FIRST → núcleos y cierres → ACTION / GOTO → seguimiento",
                  padding=(0, 4, 0, 12)).pack(anchor="w")
        upper = ttk.Frame(outer)
        upper.pack(fill="x")
        upper.columnconfigure(0, weight=3)
        upper.columnconfigure(1, weight=2)
        grammar_panel = ttk.LabelFrame(upper, text="1. Gramática", padding=8)
        grammar_panel.grid(row=0, column=0, sticky="nsew", padx=(0, 12))
        self.grammar = tk.Text(grammar_panel, height=8, width=60, wrap="none", undo=True,
                               font=("DejaVu Sans Mono", 11))
        grammar_scroll = ttk.Scrollbar(grammar_panel, command=self.grammar.yview)
        self.grammar.configure(yscrollcommand=grammar_scroll.set)
        self.grammar.grid(row=0, column=0, sticky="nsew")
        grammar_scroll.grid(row=0, column=1, sticky="ns")
        grammar_panel.columnconfigure(0, weight=1)
        self.grammar.insert("1.0", DEFAULT)
        ttk.Label(grammar_panel, text="Separa símbolos: ( A ). Alternativas: |. Vacío: ε o ''.\n"
                  "La primera regla define el inicio; se agrega una regla aumentada.").grid(
                      row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        controls = ttk.LabelFrame(upper, text="2. Construir y analizar", padding=10)
        controls.grid(row=0, column=1, sticky="nsew")
        controls.columnconfigure(0, weight=1)
        ttk.Label(controls, text="Entrada: tokens separados por espacios (vacía = ε)").grid(row=0, column=0, columnspan=3, sticky="w")
        self.tokens = tk.StringVar(value="( a )")
        ttk.Entry(controls, textvariable=self.tokens, font=("DejaVu Sans Mono", 11)).grid(row=1, column=0, columnspan=3, sticky="ew", pady=(5, 10))
        settings = ttk.Frame(controls)
        settings.grid(row=2, column=0, columnspan=3, sticky="w")
        ttk.Label(settings, text="Máx. estados:").pack(side="left")
        self.max_states = tk.StringVar(value="1000")
        ttk.Spinbox(settings, from_=1, to=5000, width=6, textvariable=self.max_states).pack(side="left", padx=(5, 15))
        ttk.Label(settings, text="Máx. pasos:").pack(side="left")
        self.max_steps = tk.StringVar(value="1000")
        ttk.Spinbox(settings, from_=1, to=10000, width=6, textvariable=self.max_steps).pack(side="left", padx=5)
        buttons = ttk.Frame(controls)
        buttons.grid(row=3, column=0, columnspan=3, sticky="w", pady=12)
        self.build_button = ttk.Button(buttons, text="Construir tablas", command=lambda: self.run(False))
        self.build_button.pack(side="left", padx=(0, 8))
        self.parse_button = ttk.Button(buttons, text="Construir y analizar", command=lambda: self.run(True))
        self.parse_button.pack(side="left")
        files = ttk.Frame(controls)
        files.grid(row=4, column=0, columnspan=3, sticky="w")
        ttk.Button(files, text="Abrir…", command=self.open_grammar).pack(side="left")
        ttk.Button(files, text="Guardar…", command=self.save_grammar).pack(side="left", padx=6)
        self.export_button = ttk.Button(files, text="Exportar JSON…", command=self.export, state="disabled")
        self.export_button.pack(side="left")
        self.status = tk.StringVar(value="Escribe una gramática y pulsa Construir tablas.")
        ttk.Label(outer, textvariable=self.status, wraplength=1320, padding=(0, 10)).pack(fill="x")
        self.notebook = ttk.Notebook(outer)
        self.notebook.pack(fill="both", expand=True)
        self.overview = self.add_table("Producciones y FIRST", custom=True)
        self.productions = Table(self.overview)
        self.productions.pack(side="left", fill="both", expand=True, padx=(0, 10))
        self.first = Table(self.overview)
        self.first.pack(side="left", fill="both", expand=True)
        self.closure_page = ttk.Frame(self.notebook, padding=6)
        self.notebook.add(self.closure_page, text="Closure · I0, I1, …")
        navigation = ttk.Frame(self.closure_page)
        navigation.pack(fill="x")
        self.prev = ttk.Button(navigation, text="← Anteriores", command=lambda: self.change_page(-1))
        self.prev.pack(side="left")
        self.page_label = ttk.Label(navigation, text="Sin estados")
        self.page_label.pack(side="left", padx=15)
        self.next = ttk.Button(navigation, text="Siguientes →", command=lambda: self.change_page(1))
        self.next.pack(side="left")
        ttk.Label(navigation, text="Kernel: punto avanzado. Closure: kernel + posibilidades añadidas.").pack(side="left", padx=20)
        canvas_frame = ttk.Frame(self.closure_page)
        canvas_frame.pack(fill="both", expand=True, pady=(8, 0))
        self.canvas = tk.Canvas(canvas_frame, background="white", highlightthickness=0)
        y = ttk.Scrollbar(canvas_frame, orient="vertical", command=self.canvas.yview)
        x = ttk.Scrollbar(canvas_frame, orient="horizontal", command=self.canvas.xview)
        self.canvas.configure(yscrollcommand=y.set, xscrollcommand=x.set)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        y.grid(row=0, column=1, sticky="ns")
        x.grid(row=1, column=0, sticky="ew")
        canvas_frame.columnconfigure(0, weight=1)
        canvas_frame.rowconfigure(0, weight=1)
        self.closure_grid = tk.Frame(self.canvas, background="#d4dce6")
        self.canvas.create_window((0, 0), window=self.closure_grid, anchor="nw")
        self.closure_grid.bind("<Configure>", lambda _: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.transitions = self.add_table("Transiciones GOTO")
        self.action = self.add_table("ACTION")
        self.goto = self.add_table("GOTO")
        self.lr = self.add_table("LR table · ACTION | GOTO")
        self.trace_page = self.add_table("Seguimiento", custom=True)
        self.trace = Table(self.trace_page)
        self.trace.pack(fill="both", expand=True)
        ttk.Label(outer, text="sN = consumir token e ir a IN   ·   rN = reducir por producción N   ·   acc = aceptar   ·   — = error",
                  padding=(0, 8, 0, 0)).pack(anchor="w")
        self.grammar.bind("<<Modified>>", self.edited)
        self.grammar.edit_modified(False)
        for variable in (self.tokens, self.max_states, self.max_steps):
            variable.trace_add("write", self.edited)
        root.after(100, self.poll)

    def add_table(self, title, custom=False):
        page = ttk.Frame(self.notebook, padding=6)
        self.notebook.add(page, text=title)
        if custom:
            return page
        table = Table(page)
        table.pack(fill="both", expand=True)
        return table

    def current(self):
        return (self.grammar.get("1.0", "end-1c"), self.tokens.get(), self.max_states.get(), self.max_steps.get())

    def edited(self, *_):
        if self.grammar.edit_modified():
            self.grammar.edit_modified(False)
        if self.data and self.current() != self.snapshot:
            self.status.set("Cambios pendientes: las tablas visibles corresponden a la ejecución anterior. Vuelve a construir.")

    def run(self, analyze):
        if self.busy:
            return
        request = self.current()
        try:
            states, steps = int(request[2]), int(request[3])
            if not 1 <= states <= 5000 or not 1 <= steps <= 10000:
                raise ValueError
        except ValueError:
            messagebox.showerror("Límites", "Estados: 1–5000. Pasos: 1–10000.")
            return
        self.busy = True
        self.build_button.configure(state="disabled")
        self.parse_button.configure(state="disabled")
        self.status.set("Construyendo LR(1)… La primera ejecución compila el motor C++.")

        def worker():
            try:
                data = run_engine(request[0], request[1], states, steps, analyze)
                self.messages.put((request, data, None))
            except Exception as exc:
                detail = str(exc)
                if isinstance(exc, subprocess.CalledProcessError):
                    detail = exc.stderr or detail
                self.messages.put((request, None, detail))
        threading.Thread(target=worker, daemon=True).start()

    def poll(self):
        try:
            request, data, error = self.messages.get_nowait()
        except queue.Empty:
            pass
        else:
            self.busy = False
            self.build_button.configure(state="normal")
            self.parse_button.configure(state="normal")
            if error:
                self.status.set("No se pudo construir. Las tablas anteriores, si existen, no corresponden a esta solicitud.")
                messagebox.showerror("Error", error)
            else:
                self.data, self.snapshot, self.page = data, request, 0
                self.render()
                self.edited()
        self.root.after(100, self.poll)

    def render(self):
        d = self.data
        self.export_button.configure(state="normal")
        self.productions.fill(["Regla", "Producción (r0: aumento automático)"],
                              [(i, p["text"]) for i, p in enumerate(d["productions"])], widths=[70, 650])
        self.first.fill(["No terminal", "FIRST", "Anulable"],
                        [(n, "{ " + ", ".join(d["first"][n]) + " }", "Sí" if n in d["nullable"] else "No")
                         for n in d["nonterminals"]], widths=[150, 300, 100])
        self.transitions.fill(["Origen", "Símbolo", "Destino"],
                              [(f'I{s["id"]}', token, f'I{target}') for s in d["states"]
                               for token, target in s["transitions"].items()], widths=[120, 220, 120])
        terminals = d["terminals"] + ["$"]
        nonterminals = d["nonterminals"][1:]
        actions, gotos, combined, tags = [], [], [], []
        for i in range(len(d["states"])):
            a = [" / ".join(d["action"][i].get(t, [])) or "—" for t in terminals]
            g = [d["goto"][i].get(n, "—") for n in nonterminals]
            actions.append([f"I{i}"] + a)
            gotos.append([f"I{i}"] + g)
            combined.append([f"I{i}"] + a + g)
            tags.append("accept" if "acc" in a else "")
        self.action.fill(["Estado"] + terminals, actions, tags)
        self.goto.fill(["Estado"] + nonterminals, gotos)
        self.lr.fill(["Estado"] + [f"A: {t}" for t in terminals] + [f"G: {n}" for n in nonterminals], combined, tags)
        self.render_closure()
        parse = d.get("parse")
        trace_rows = []
        if parse:
            for step, row in enumerate(parse["trace"], 1):
                stack = str(row["states"][0])
                for token, state in zip(row["symbols"], row["states"][1:]):
                    stack += f" {token} {state}"
                trace_rows.append([step, stack, " ".join(parse["tokens"][row["position"]:]), row["action"]])
        self.trace.fill(["Paso", "Pila (símbolos y estados)", "Entrada pendiente", "Acción"], trace_rows,
                        widths=[50, 200, 140, 260])
        self.root.after_idle(self.fit_trace)
        summary = f'{len(d["states"])} estados · {len(d["productions"])-1} producciones · '
        summary += "Gramática LR(1)." if d["lr1"] else "La gramática no es LR(1); no se puede analizar la entrada."
        if parse:
            summary += "  " + parse["message"]
        self.status.set(summary)
        if not d["lr1"]:
            self.notebook.select(self.lr.master)
        elif parse:
            self.notebook.select(self.trace_page)
        else:
            self.notebook.select(self.closure_page)

    def fit_trace(self):
        available = self.trace.winfo_width()
        if available > 100:
            width = max(600, available - 25)
            for col, fraction in enumerate((0.07, 0.30, 0.22, 0.41)):
                self.trace.tree.column(f"c{col}", width=max(50, int(width * fraction)))

    def render_closure(self):
        for child in self.closure_grid.winfo_children():
            child.destroy()
        if not self.data:
            return
        states = self.data["states"]
        first = self.page * self.PAGE_SIZE
        last = min(first + self.PAGE_SIZE, len(states))
        self.page_label.configure(text=f"I{first}–I{last-1} de {len(states)} estados")
        self.prev.configure(state="normal" if first else "disabled")
        self.next.configure(state="normal" if last < len(states) else "disabled")
        incoming = {s["id"]: [] for s in states}
        incoming[0].append("Inicio")
        for s in states:
            for token, target in s["transitions"].items():
                incoming[target].append(f'goto(I{s["id"]}, {token})')
        available = max(850, self.canvas.winfo_width() - 20)
        widths = [int(available * 0.19), int(available * 0.32), 65, int(available * 0.49) - 65]
        for col, title in enumerate(["GOTO de entrada", "Kernel / núcleo", "Estado", "Closure / cierre completo"]):
            label = tk.Label(self.closure_grid, text=title, background="#19324f", foreground="white",
                             font=("DejaVu Sans", 10, "bold"), padx=8, pady=10, anchor="w", wraplength=widths[col]-18)
            label.grid(row=0, column=col, sticky="nsew", padx=1, pady=1)
            self.closure_grid.columnconfigure(col, minsize=widths[col])
        for row, s in enumerate(states[first:last], 1):
            values = ["\n".join(incoming[s["id"]]), "\n".join(i["text"] for i in s["kernel"]),
                      f'I{s["id"]}', "\n".join(i["text"] for i in s["closure"])]
            for col, text in enumerate(values):
                label = tk.Label(self.closure_grid, text=text, background="#f0f5fa" if row % 2 == 0 else "white",
                                 foreground="#143557" if col == 2 else "#172536", anchor="nw", justify="left",
                                 font=("DejaVu Sans Mono", 10), wraplength=widths[col]-18, padx=8, pady=10)
                label.grid(row=row, column=col, sticky="nsew", padx=1, pady=1)
        self.canvas.yview_moveto(0)

    def change_page(self, delta):
        if self.data:
            maximum = (len(self.data["states"])-1) // self.PAGE_SIZE
            self.page = max(0, min(maximum, self.page + delta))
            self.render_closure()

    def open_grammar(self):
        path = filedialog.askopenfilename(initialdir=HERE / "examples", filetypes=[("Gramáticas", "*.grammar *.txt"), ("Todos", "*")])
        if path:
            try:
                content = Path(path).read_text(encoding="utf-8")
                self.grammar.delete("1.0", "end")
                self.grammar.insert("1.0", content)
            except (OSError, UnicodeError) as exc:
                messagebox.showerror("Abrir", str(exc))

    def save_grammar(self):
        path = filedialog.asksaveasfilename(defaultextension=".grammar", filetypes=[("Gramática", "*.grammar")])
        if path:
            self.write_file(path, self.grammar.get("1.0", "end-1c"))

    def export(self):
        if not self.data:
            return
        path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON", "*.json")])
        if path:
            payload = dict(self.data)
            payload["source"] = {"grammar": self.snapshot[0], "input": self.snapshot[1],
                                 "max_states": self.snapshot[2], "max_steps": self.snapshot[3]}
            self.write_file(path, json.dumps(payload, ensure_ascii=False, indent=2))

    @staticmethod
    def write_file(path, text):
        try:
            Path(path).write_text(text, encoding="utf-8")
        except OSError as exc:
            messagebox.showerror("Guardar", str(exc))


def main():
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("--grammar", type=Path, help="Archivo de gramática inicial")
    options = args.parse_args()
    root = tk.Tk()
    app = App(root)
    if options.grammar:
        app.grammar.delete("1.0", "end")
        app.grammar.insert("1.0", options.grammar.read_text(encoding="utf-8"))
    root.mainloop()


if __name__ == "__main__":
    main()
