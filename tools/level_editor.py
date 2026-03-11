#!/usr/bin/env python3
"""Simple GUI editor for PS2 platformer level files.

Level format:
    ; comments
    width=80
    height=14
    spawn=2,10
    data:
    ....
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk


COMMENT_LINE = "; # = solid, . = empty, G = goal, X = trap, ? = coin block, E = empty block, C = coin, M = mushroom, 1 = 1up"
DEFAULT_WIDTH = 40
DEFAULT_HEIGHT = 14
CELL_SIZE = 24
MAX_WIDTH = 128
MAX_HEIGHT = 32

TILES = {
    ".": {"name": "Empty", "color": "#f2f2f2"},
    "#": {"name": "Solid", "color": "#3d3d3d"},
    "X": {"name": "Trap", "color": "#d64545"},
    "G": {"name": "Goal", "color": "#4ea756"},
    "?": {"name": "Coin Block", "color": "#e0b020"},
    "E": {"name": "Empty Block", "color": "#504030"},
    "C": {"name": "Coin", "color": "#ffd700"},
    "M": {"name": "Mushroom", "color": "#e03030"},
    "1": {"name": "1UP", "color": "#30c030"},
}


@dataclass
class MovingEntityData:
    type: str       # 'P' = platform, 'T' = trap
    tx: int
    ty: int
    direction: str  # 'H' or 'V'
    range: int
    speed: float = 1.0


@dataclass
class LevelData:
    width: int
    height: int
    spawn_x: int
    spawn_y: int
    rows: list[list[str]]
    moving_entities: list[MovingEntityData]


def normalize_rows(rows: list[str], width: int, height: int) -> list[list[str]]:
    normalized: list[list[str]] = []
    for y in range(height):
        row = rows[y] if y < len(rows) else ""
        clipped = row[:width]
        clipped += "." * max(0, width - len(clipped))
        clean = [ch if ch in TILES else "." for ch in clipped]
        normalized.append(clean)
    return normalized


def parse_level_file(path: Path) -> LevelData:
    width = DEFAULT_WIDTH
    height = DEFAULT_HEIGHT
    spawn_x, spawn_y = 2, 2
    rows: list[str] = []
    moving_entities: list[MovingEntityData] = []
    in_data = False
    in_moving = False

    with path.open("r", encoding="utf-8") as fp:
        for raw_line in fp:
            line = raw_line.strip()
            if not line or line.startswith(";"):
                continue

            if not in_data and not in_moving:
                if line.startswith("width="):
                    width = max(1, min(MAX_WIDTH, int(line[6:])))
                elif line.startswith("height="):
                    height = max(1, min(MAX_HEIGHT, int(line[7:])))
                elif line.startswith("spawn="):
                    lhs, rhs = line[6:].split(",", 1)
                    spawn_x = int(lhs)
                    spawn_y = int(rhs)
                elif line == "data:":
                    in_data = True
                elif line == "moving:":
                    in_moving = True
            elif in_data:
                if line == "moving:":
                    in_data = False
                    in_moving = True
                else:
                    rows.append(line)
            elif in_moving:
                parts = line.split()
                if len(parts) >= 4:
                    etype = parts[0].upper()
                    coords = parts[1].split(",")
                    direction = parts[2].upper()
                    erange = int(parts[3])
                    espeed = float(parts[4]) if len(parts) > 4 else 1.0
                    if etype in ("P", "T") and len(coords) == 2:
                        moving_entities.append(MovingEntityData(
                            type=etype, tx=int(coords[0]), ty=int(coords[1]),
                            direction=direction, range=erange, speed=espeed,
                        ))

    if rows:
        width = min(MAX_WIDTH, max(width, max(len(row) for row in rows)))
        height = min(MAX_HEIGHT, max(height, len(rows)))

    spawn_x = max(0, min(width - 1, spawn_x))
    spawn_y = max(0, min(height - 1, spawn_y))
    return LevelData(width, height, spawn_x, spawn_y, normalize_rows(rows, width, height), moving_entities)


def make_default_level() -> LevelData:
    rows = normalize_rows([], DEFAULT_WIDTH, DEFAULT_HEIGHT)
    return LevelData(DEFAULT_WIDTH, DEFAULT_HEIGHT, 2, 10, rows, [])


class LevelEditorApp:
    def __init__(self, root: tk.Tk, initial_path: Path | None):
        self.root = root
        self.root.title("PS2 Level Editor")

        self.current_path: Path | None = None
        self.level = make_default_level()

        self.active_tile = tk.StringVar(value="#")
        self.place_mode = tk.StringVar(value="tile")
        self.entity_type = tk.StringVar(value="P")
        self.entity_dir = tk.StringVar(value="H")
        self.entity_range = tk.IntVar(value=3)
        self.entity_speed = tk.DoubleVar(value=1.0)
        self.width_var = tk.IntVar(value=self.level.width)
        self.height_var = tk.IntVar(value=self.level.height)
        self.spawn_var = tk.StringVar(value="2,10")
        self.status_var = tk.StringVar(value="Ready")
        self._drag_entity: MovingEntityData | None = None
        self._drag_mode: str = ""  # "move" or "range"
        self._selected_entity: MovingEntityData | None = None

        self._build_ui()
        self._refresh_all()

        if initial_path is not None:
            self.load_level(initial_path)

    def _build_ui(self) -> None:
        self.root.columnconfigure(1, weight=1)
        self.root.rowconfigure(0, weight=1)

        side = ttk.Frame(self.root, padding=10)
        side.grid(row=0, column=0, sticky="ns")

        canvas_frame = ttk.Frame(self.root, padding=(0, 10, 10, 10))
        canvas_frame.grid(row=0, column=1, sticky="nsew")
        canvas_frame.columnconfigure(0, weight=1)
        canvas_frame.rowconfigure(0, weight=1)

        self.canvas = tk.Canvas(canvas_frame, bg="#ffffff", highlightthickness=1, highlightbackground="#777")
        self.canvas.grid(row=0, column=0, sticky="nsew")

        self.h_scroll = ttk.Scrollbar(canvas_frame, orient="horizontal", command=self.canvas.xview)
        self.h_scroll.grid(row=1, column=0, sticky="ew")
        self.v_scroll = ttk.Scrollbar(canvas_frame, orient="vertical", command=self.canvas.yview)
        self.v_scroll.grid(row=0, column=1, sticky="ns")
        self.canvas.configure(xscrollcommand=self.h_scroll.set, yscrollcommand=self.v_scroll.set)

        ttk.Label(side, text="Tiles", font=("Segoe UI", 10, "bold")).grid(row=0, column=0, sticky="w")
        row = 1
        for tile, info in TILES.items():
            ttk.Radiobutton(
                side,
                text=f"{tile}  {info['name']}",
                value=tile,
                variable=self.active_tile,
                command=self._update_status,
            ).grid(row=row, column=0, sticky="w")
            row += 1

        ttk.Separator(side, orient="horizontal").grid(row=row, column=0, sticky="ew", pady=8)
        row += 1

        ttk.Label(side, text="Placement", font=("Segoe UI", 10, "bold")).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Radiobutton(side, text="Paint tile", value="tile", variable=self.place_mode, command=self._update_status).grid(
            row=row, column=0, sticky="w"
        )
        row += 1
        ttk.Radiobutton(side, text="Set spawn", value="spawn", variable=self.place_mode, command=self._update_status).grid(
            row=row, column=0, sticky="w"
        )
        row += 1
        ttk.Radiobutton(side, text="Entity (drag range)", value="entity", variable=self.place_mode, command=self._update_status).grid(
            row=row, column=0, sticky="w"
        )
        row += 1

        ttk.Separator(side, orient="horizontal").grid(row=row, column=0, sticky="ew", pady=8)
        row += 1

        ttk.Label(side, text="Entity Options", font=("Segoe UI", 10, "bold")).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Radiobutton(side, text="P  Platform", value="P", variable=self.entity_type).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Radiobutton(side, text="T  Trap", value="T", variable=self.entity_type).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Label(side, text="Direction").grid(row=row, column=0, sticky="w")
        row += 1
        ent_dir_frame = ttk.Frame(side)
        ent_dir_frame.grid(row=row, column=0, sticky="w")
        ttk.Radiobutton(ent_dir_frame, text="H", value="H", variable=self.entity_dir).pack(side="left")
        ttk.Radiobutton(ent_dir_frame, text="V", value="V", variable=self.entity_dir).pack(side="left")
        row += 1
        ttk.Label(side, text="Range (tiles)").grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Spinbox(side, from_=1, to=20, textvariable=self.entity_range, width=10).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Label(side, text="Speed").grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Spinbox(side, from_=0.5, to=5.0, increment=0.5, textvariable=self.entity_speed, width=10).grid(row=row, column=0, sticky="w")
        row += 1

        ttk.Separator(side, orient="horizontal").grid(row=row, column=0, sticky="ew", pady=8)
        row += 1

        ttk.Label(side, text="Level Size", font=("Segoe UI", 10, "bold")).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Label(side, text="Width").grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Spinbox(side, from_=1, to=MAX_WIDTH, textvariable=self.width_var, width=10).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Label(side, text="Height").grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Spinbox(side, from_=1, to=MAX_HEIGHT, textvariable=self.height_var, width=10).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Button(side, text="Apply Size", command=self.apply_size).grid(row=row, column=0, sticky="ew", pady=(4, 0))
        row += 1

        ttk.Separator(side, orient="horizontal").grid(row=row, column=0, sticky="ew", pady=8)
        row += 1

        ttk.Label(side, text="File", font=("Segoe UI", 10, "bold")).grid(row=row, column=0, sticky="w")
        row += 1
        ttk.Button(side, text="Open", command=self.open_dialog).grid(row=row, column=0, sticky="ew")
        row += 1
        ttk.Button(side, text="Save", command=self.save).grid(row=row, column=0, sticky="ew", pady=(4, 0))
        row += 1
        ttk.Button(side, text="Save As", command=self.save_as).grid(row=row, column=0, sticky="ew", pady=(4, 0))
        row += 1
        ttk.Button(side, text="New", command=self.new_level).grid(row=row, column=0, sticky="ew", pady=(4, 0))

        ttk.Label(side, textvariable=self.spawn_var, foreground="#24539b").grid(row=row + 1, column=0, sticky="w", pady=(10, 0))
        ttk.Label(side, textvariable=self.status_var, foreground="#555").grid(row=row + 2, column=0, sticky="w", pady=(2, 0))

        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)
        self.canvas.bind("<Button-3>", self.on_right_click)

    def _refresh_all(self) -> None:
        self.width_var.set(self.level.width)
        self.height_var.set(self.level.height)
        self.spawn_var.set(f"spawn={self.level.spawn_x},{self.level.spawn_y}")
        self._redraw_canvas()
        self._update_title()
        self._update_status()

    def _update_title(self) -> None:
        suffix = str(self.current_path) if self.current_path else "(unsaved)"
        self.root.title(f"PS2 Level Editor - {suffix}")

    def _update_status(self) -> None:
        mode = self.place_mode.get()
        if mode == "spawn":
            self.status_var.set("Click grid to set player spawn")
        elif mode == "entity":
            self.status_var.set("Click=add | Drag handle=resize | Right-click=delete")
        else:
            tile = self.active_tile.get()
            self.status_var.set(f"Painting tile '{tile}'")

    def _redraw_canvas(self) -> None:
        self.canvas.delete("all")
        w = self.level.width * CELL_SIZE
        h = self.level.height * CELL_SIZE
        self.canvas.configure(scrollregion=(0, 0, w, h))

        for y in range(self.level.height):
            for x in range(self.level.width):
                tile = self.level.rows[y][x]
                color = TILES.get(tile, TILES["."])["color"]
                x0 = x * CELL_SIZE
                y0 = y * CELL_SIZE
                x1 = x0 + CELL_SIZE
                y1 = y0 + CELL_SIZE
                self.canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline="#c7c7c7")
                if tile in ("G", "X", "?", "C", "M", "1", "E"):
                    self.canvas.create_text(
                        x0 + CELL_SIZE // 2,
                        y0 + CELL_SIZE // 2,
                        text=tile,
                        fill="#ffffff",
                        font=("Segoe UI", 10, "bold"),
                    )

        self._draw_spawn_marker()
        self._draw_entity_markers()

    def _draw_spawn_marker(self) -> None:
        sx = self.level.spawn_x
        sy = self.level.spawn_y
        x0 = sx * CELL_SIZE
        y0 = sy * CELL_SIZE
        x1 = x0 + CELL_SIZE
        y1 = y0 + CELL_SIZE
        self.canvas.create_oval(x0 + 4, y0 + 4, x1 - 4, y1 - 4, outline="#1a57c8", width=2)
        self.canvas.create_text(x0 + CELL_SIZE // 2, y0 + CELL_SIZE // 2, text="S", fill="#1a57c8", font=("Segoe UI", 10, "bold"))

    def _draw_entity_markers(self) -> None:
        for ent in self.level.moving_entities:
            x0 = ent.tx * CELL_SIZE
            y0 = ent.ty * CELL_SIZE
            is_selected = (ent is self._selected_entity)
            color = "#c02020" if ent.type == "T" else "#3060a0"
            sel_outline = "#ffcc00" if is_selected else color
            label = f"{'T' if ent.type == 'T' else 'P'}{ent.direction}{ent.range}"

            # Draw entity origin
            self.canvas.create_rectangle(x0 + 1, y0 + 1, x0 + CELL_SIZE - 1, y0 + CELL_SIZE - 1,
                                          outline=sel_outline, width=4 if is_selected else 3)
            self.canvas.create_text(x0 + CELL_SIZE // 2, y0 + CELL_SIZE // 2,
                                     text=label, fill=color, font=("Segoe UI", 7, "bold"))

            # Draw range line + handle
            if ent.direction == "H":
                ex = (ent.tx + ent.range) * CELL_SIZE
                cy = y0 + CELL_SIZE // 2
                self.canvas.create_line(x0, cy, ex, cy, fill=color, width=2, dash=(4, 2))
                # Range handle (diamond shape)
                hx = ex
                hy = cy
                self.canvas.create_polygon(
                    hx - 6, hy, hx, hy - 6, hx + 6, hy, hx, hy + 6,
                    fill=sel_outline, outline="#000", width=1,
                )
            else:
                ey = (ent.ty + ent.range) * CELL_SIZE
                cx = x0 + CELL_SIZE // 2
                self.canvas.create_line(cx, y0, cx, ey, fill=color, width=2, dash=(4, 2))
                # Range handle (diamond shape)
                hx = cx
                hy = ey
                self.canvas.create_polygon(
                    hx - 6, hy, hx, hy - 6, hx + 6, hy, hx, hy + 6,
                    fill=sel_outline, outline="#000", width=1,
                )

    def _event_to_cell(self, event: tk.Event) -> tuple[int, int] | None:
        canvas_x = int(self.canvas.canvasx(event.x))
        canvas_y = int(self.canvas.canvasy(event.y))
        tx = canvas_x // CELL_SIZE
        ty = canvas_y // CELL_SIZE
        if tx < 0 or ty < 0 or tx >= self.level.width or ty >= self.level.height:
            return None
        return tx, ty

    def _find_entity_at(self, tx: int, ty: int):
        """Return the entity whose origin is at (tx, ty), or None."""
        for ent in self.level.moving_entities:
            if ent.tx == tx and ent.ty == ty:
                return ent
        return None

    def _find_handle_at(self, tx: int, ty: int):
        """Return entity whose range-end handle is at (tx, ty), or None."""
        for ent in self.level.moving_entities:
            if ent.direction == "H":
                hx, hy = ent.tx + ent.range, ent.ty
            else:
                hx, hy = ent.tx, ent.ty + ent.range
            if hx == tx and hy == ty:
                return ent
        return None

    def on_click(self, event: tk.Event) -> None:
        mode = self.place_mode.get()
        if mode == "entity":
            cell = self._event_to_cell(event)
            if cell is None:
                return
            tx, ty = cell

            # Priority: handle hit → start range drag
            handle_ent = self._find_handle_at(tx, ty)
            if handle_ent:
                self._drag_entity = handle_ent
                self._drag_mode = "range"
                self._selected_entity = handle_ent
                self._redraw_canvas()
                return

            # Origin hit → start move drag
            origin_ent = self._find_entity_at(tx, ty)
            if origin_ent:
                self._drag_entity = origin_ent
                self._drag_mode = "move"
                self._selected_entity = origin_ent
                self._redraw_canvas()
                return

            # Empty cell → create new entity
            ent = MovingEntityData(
                type=self.entity_type.get(),
                tx=tx, ty=ty,
                direction=self.entity_dir.get(),
                range=max(1, self.entity_range.get()),
                speed=max(0.5, self.entity_speed.get()),
            )
            self.level.moving_entities.append(ent)
            self._selected_entity = ent
            self.status_var.set(f"Placed {ent.type} at ({tx},{ty})")
            self._redraw_canvas()
        else:
            self._apply_tool(event)

    def on_drag(self, event: tk.Event) -> None:
        mode = self.place_mode.get()
        if mode == "entity" and self._drag_entity is not None:
            cell = self._event_to_cell(event)
            if cell is None:
                return
            tx, ty = cell
            ent = self._drag_entity

            if self._drag_mode == "range":
                if ent.direction == "H":
                    new_range = max(1, tx - ent.tx)
                else:
                    new_range = max(1, ty - ent.ty)
                ent.range = new_range
                self.status_var.set(f"Range → {new_range}")
            elif self._drag_mode == "move":
                ent.tx = tx
                ent.ty = ty
                self.status_var.set(f"Moved to ({tx},{ty})")
            self._redraw_canvas()
        elif mode == "tile":
            self._apply_tool(event)

    def on_release(self, event: tk.Event) -> None:
        if self._drag_entity is not None:
            self._drag_entity = None
            self._drag_mode = None

    def on_right_click(self, event: tk.Event) -> None:
        cell = self._event_to_cell(event)
        if cell is None:
            return
        tx, ty = cell

        mode = self.place_mode.get()
        if mode == "entity":
            before = len(self.level.moving_entities)
            self.level.moving_entities = [
                e for e in self.level.moving_entities if not (e.tx == tx and e.ty == ty)
            ]
            removed = before - len(self.level.moving_entities)
            if self._selected_entity and self._selected_entity not in self.level.moving_entities:
                self._selected_entity = None
            self.status_var.set(f"Deleted {removed} entity at ({tx},{ty})")
        else:
            self.level.rows[ty][tx] = "."
        self._redraw_canvas()

    def _apply_tool(self, event: tk.Event) -> None:
        cell = self._event_to_cell(event)
        if cell is None:
            return

        tx, ty = cell
        mode = self.place_mode.get()

        if mode == "spawn":
            self.level.spawn_x = tx
            self.level.spawn_y = ty
            self.spawn_var.set(f"spawn={tx},{ty}")
        else:
            self.level.rows[ty][tx] = self.active_tile.get()

        self._redraw_canvas()

    def load_level(self, path: Path) -> None:
        try:
            loaded = parse_level_file(path)
        except Exception as exc:
            messagebox.showerror("Open failed", f"Cannot open level file:\n{path}\n\n{exc}")
            return

        self.level = loaded
        self.current_path = path
        self._refresh_all()

    def open_dialog(self) -> None:
        initial_dir = Path("levels")
        selected = filedialog.askopenfilename(
            title="Open level",
            initialdir=initial_dir if initial_dir.exists() else Path.cwd(),
            filetypes=[("Level files", "*.txt"), ("All files", "*.*")],
        )
        if selected:
            self.load_level(Path(selected))

    def new_level(self) -> None:
        self.level = make_default_level()
        self.current_path = None
        self._refresh_all()

    def apply_size(self) -> None:
        try:
            new_w = max(1, min(MAX_WIDTH, int(self.width_var.get())))
            new_h = max(1, min(MAX_HEIGHT, int(self.height_var.get())))
        except Exception:
            messagebox.showwarning("Invalid size", "Width and height must be numbers.")
            return

        old_rows = ["".join(row) for row in self.level.rows]
        self.level.width = new_w
        self.level.height = new_h
        self.level.rows = normalize_rows(old_rows, new_w, new_h)
        self.level.spawn_x = min(self.level.spawn_x, new_w - 1)
        self.level.spawn_y = min(self.level.spawn_y, new_h - 1)
        # Filter entities outside new bounds
        self.level.moving_entities = [
            e for e in self.level.moving_entities if e.tx < new_w and e.ty < new_h
        ]
        self._refresh_all()

    def _serialize(self) -> str:
        lines = [
            COMMENT_LINE,
            f"width={self.level.width}",
            f"height={self.level.height}",
            f"spawn={self.level.spawn_x},{self.level.spawn_y}",
            "data:",
        ]

        lines.extend("".join(row) for row in self.level.rows)

        if self.level.moving_entities:
            lines.append("moving:")
            for ent in self.level.moving_entities:
                lines.append(f"{ent.type} {ent.tx},{ent.ty} {ent.direction} {ent.range} {ent.speed}")

        return "\n".join(lines) + "\n"

    def _validate_level(self) -> bool:
        goals = sum(row.count("G") for row in self.level.rows)
        if goals == 0:
            return messagebox.askyesno("No goal tile", "This level has no 'G' tile. Save anyway?")
        return True

    def save(self) -> None:
        if self.current_path is None:
            self.save_as()
            return
        self._save_to_path(self.current_path)

    def save_as(self) -> None:
        initial_dir = Path("levels")
        selected = filedialog.asksaveasfilename(
            title="Save level as",
            initialdir=initial_dir if initial_dir.exists() else Path.cwd(),
            defaultextension=".txt",
            filetypes=[("Level files", "*.txt"), ("All files", "*.*")],
        )
        if not selected:
            return
        self._save_to_path(Path(selected))

    def _save_to_path(self, path: Path) -> None:
        if not self._validate_level():
            return
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(self._serialize(), encoding="utf-8", newline="\n")
        except Exception as exc:
            messagebox.showerror("Save failed", f"Cannot save level file:\n{path}\n\n{exc}")
            return

        self.current_path = path
        self._update_title()
        self.status_var.set(f"Saved: {path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="PS2 platformer level GUI editor")
    parser.add_argument("level_file", nargs="?", help="Optional level file path to open")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = tk.Tk()
    initial = Path(args.level_file) if args.level_file else None
    LevelEditorApp(root, initial)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
