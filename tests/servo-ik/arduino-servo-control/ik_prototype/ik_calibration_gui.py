"""Comprehensive manual, automatic, and IK control for a four-servo Uno R4 arm."""

from __future__ import annotations

import json
import math
from pathlib import Path
import time
import tkinter as tk
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports


BAUD_RATE = 115200
CALIBRATION_FILE = Path(__file__).with_name("ik_calibration.json")
DIRECT_LIMITS = ((0.0, 270.0), (0.0, 112.5), (90.0, 270.0), (0.0, 270.0))


def build_auto_sweep(pose, sweep_percent=100.0):
    """One bounded, repeatable joint-space dance; never clip individual poses."""
    if len(pose) != 4 or not all(math.isfinite(v) for v in pose):
        raise ValueError("Automatic motion needs four finite starting angles.")
    if not math.isfinite(sweep_percent) or not 25 <= sweep_percent <= 100:
        raise ValueError("Sweep size must be between 25 and 100 percent.")
    amplitudes = [value * sweep_percent / 100 for value in (40, 25, 40, 45)]
    centers = [
        max(lo + 5 + amplitude, min(hi - 5 - amplitude, angle))
        for angle, amplitude, (lo, hi) in zip(pose, amplitudes, DIRECT_LIMITS)
    ]
    phases = (
        (0, -.4, .3, 0), (.7, -1, -.5, .6), (1, -.2, -1, 1),
        (.65, .7, -.4, .35), (0, 1, .35, -.4),
        (-.75, .4, 1, -1), (-1, -.45, .5, -.65), (-.55, -.9, -.2, 0),
    )
    return [tuple(c + a * p for c, a, p in zip(centers, amplitudes, phase))
            for phase in phases]


def auto_move_duration_ms(start, finish, peak_speed):
    # The peak derivative of quintic smoothstep is 1.875.
    travel = max(abs(a - b) for a, b in zip(start, finish))
    duration = max(1800, math.ceil(1875 * travel / peak_speed))
    if duration > 15000:
        raise ValueError("Automatic transition exceeds the controller's timing limit.")
    return duration


class IKCalibrationApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Four-Servo Arm — Manual, Automatic, and IK Control")
        self.root.resizable(False, False)
        self.connection: serial.Serial | None = None
        self.ready = False
        self.serial_buffer = ""
        self.last_solution: dict[str, object] | None = None
        self.map_transform: tuple[float, float, float, float, float] | None = None
        self.manual_send_job: str | None = None
        self.map_drag_job: str | None = None
        self.pending_map_xy: tuple[int, int] | None = None
        self.map_is_dragging = False
        self.manual_updating = False
        self.programmatic_slider_values = [33.5, 112.5, 151.5, 27.5]
        self.auto_running = False
        self.auto_waypoints: list[tuple[float, float, float, float]] = []
        self.auto_waypoint_index = 0
        self.auto_next_move_at = 0.0
        self.auto_last_target = (33.5, 112.5, 151.5, 27.5)
        self.last_user_activity = time.monotonic()

        self.port = tk.StringVar()
        self.status = tk.StringVar(value="Disconnected — preview mode is available")
        self.elbow_solution = tk.StringVar(value="Negative")
        self.arm_clear = tk.BooleanVar(value=False)
        self.direction_aware_jogs = tk.BooleanVar(value=False)
        self.auto_enabled = tk.BooleanVar(value=True)
        self.auto_sweep_percent = tk.StringVar(value="100")
        self.auto_pace = tk.StringVar(value="Flow")
        self.auto_status = tk.StringVar(value="Automatic motion starts after 15 s of inactivity")

        defaults = {
            "base_height": "70",
            "shoulder_offset": "25",
            "upper_arm": "120",
            "forearm": "50",
            "tool_inline": "0",
            "target_x": "190",
            "target_y": "0",
            "target_z": "70",
            "base_zero": "33.5",
            "shoulder_zero": "112.5",
            "elbow_zero": "151.5",
            "base_direction": "1",
            "shoulder_direction": "-1",
            "elbow_direction": "1",
            "joint4": "27.5",
            "move_seconds": "3",
            "jog_degrees": "3",
        }
        self.values = {name: tk.StringVar(value=value) for name, value in defaults.items()}
        self.manual_angles = [
            tk.DoubleVar(value=value) for value in (33.5, 112.5, 151.5, 27.5)
        ]
        self.manual_angle_text = [
            tk.StringVar(value=f"{value:.1f}°") for value in (33.5, 112.5, 151.5, 27.5)
        ]
        self.result_text = tk.StringVar(value="Test servo directions first, then click Preview IK.")

        self._build_ui()
        self.refresh_ports()
        self.draw_preview(None)
        self.root.after(50, self.poll_serial)
        self.root.after(250, self.check_automatic_motion)
        self.root.protocol("WM_DELETE_WINDOW", self.close)

    def _build_ui(self) -> None:
        main = ttk.Frame(self.root, padding=12)
        main.grid()

        connection = ttk.LabelFrame(main, text="Arduino connection", padding=8)
        connection.grid(row=0, column=0, columnspan=2, sticky="ew")
        self.port_box = ttk.Combobox(connection, textvariable=self.port, width=20, state="readonly")
        self.port_box.grid(row=0, column=0, padx=(0, 6))
        ttk.Button(connection, text="Refresh", command=self.refresh_ports).grid(row=0, column=1)
        self.connect_button = ttk.Button(connection, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=2, padx=6)
        ttk.Label(connection, textvariable=self.status).grid(row=0, column=3, padx=(8, 0))

        inputs = ttk.Frame(main)
        inputs.grid(row=1, column=0, sticky="n", pady=(10, 0), padx=(0, 10))

        geometry = ttk.LabelFrame(inputs, text="Approximate geometry (mm)", padding=8)
        geometry.grid(row=0, column=0, sticky="ew")
        self._entry_row(geometry, 0, "Base height", "base_height")
        self._entry_row(geometry, 1, "Base-to-shoulder radial", "shoulder_offset")
        self._entry_row(geometry, 2, "Shoulder-to-elbow", "upper_arm")
        self._entry_row(geometry, 3, "Elbow-to-wrist", "forearm")
        self._entry_row(geometry, 4, "Inline wrist-to-tool", "tool_inline")

        target = ttk.LabelFrame(inputs, text="Requested tool position (mm)", padding=8)
        target.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self._entry_row(target, 0, "X", "target_x")
        self._entry_row(target, 1, "Y", "target_y")
        self._entry_row(target, 2, "Z", "target_z")
        ttk.Label(target, text="Elbow solution").grid(row=3, column=0, sticky="w")
        elbow_box = ttk.Combobox(
            target,
            textvariable=self.elbow_solution,
            values=("Positive", "Negative"),
            width=11,
            state="readonly",
        )
        elbow_box.grid(row=3, column=1, sticky="ew", pady=2)
        elbow_box.bind("<<ComboboxSelected>>", lambda _event: self.preview())

        calibration = ttk.LabelFrame(inputs, text="Servo calibration", padding=8)
        calibration.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        self._entry_row(calibration, 0, "Base zero angle", "base_zero")
        self._entry_row(calibration, 1, "Shoulder zero angle", "shoulder_zero")
        self._entry_row(calibration, 2, "Elbow zero angle", "elbow_zero")
        self._entry_row(calibration, 3, "Base direction (+1/−1)", "base_direction")
        self._entry_row(calibration, 4, "Shoulder direction", "shoulder_direction")
        self._entry_row(calibration, 5, "Elbow direction", "elbow_direction")

        held = ttk.LabelFrame(inputs, text="Motion settings", padding=8)
        held.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        self._entry_row(held, 0, "Joint 4 physical angle", "joint4")
        self._entry_row(held, 1, "IK move duration (seconds)", "move_seconds")
        self._entry_row(held, 2, "Direction-test step (degrees)", "jog_degrees")
        presets = ttk.Frame(held)
        presets.grid(row=3, column=0, columnspan=2, sticky="w", pady=(5, 0))
        ttk.Label(presets, text="Motion preset:").grid(row=0, column=0, padx=(0, 5))
        ttk.Button(
            presets, text="Fast 2 s", command=lambda: self.values["move_seconds"].set("2")
        ).grid(row=0, column=1, padx=2)
        ttk.Button(
            presets, text="Smooth 3 s", command=lambda: self.values["move_seconds"].set("3")
        ).grid(row=0, column=2, padx=2)
        ttk.Button(
            presets, text="Slow 6 s", command=lambda: self.values["move_seconds"].set("6")
        ).grid(row=0, column=3, padx=2)

        output = ttk.Frame(main)
        output.grid(row=1, column=1, sticky="n", pady=(10, 0))
        preview_frame = ttk.LabelFrame(output, text="Planar preview", padding=8)
        preview_frame.grid(row=0, column=0)
        self.canvas = tk.Canvas(preview_frame, width=430, height=300, bg="white", highlightthickness=1)
        self.canvas.grid()
        self.canvas.bind("<Button-1>", self.map_press)
        self.canvas.bind("<B1-Motion>", self.map_drag)
        self.canvas.bind("<ButtonRelease-1>", self.map_release)

        result = ttk.LabelFrame(output, text="IK result", padding=8)
        result.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        ttk.Label(result, textvariable=self.result_text, justify="left", width=57).grid(sticky="w")

        mode_tabs = ttk.Notebook(output)
        mode_tabs.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        manual_tab = ttk.Frame(mode_tabs, padding=8)
        calibration_tab = ttk.Frame(mode_tabs, padding=8)
        mode_tabs.add(manual_tab, text="Manual + automatic")
        mode_tabs.add(calibration_tab, text="Direction calibration")

        manual_frame = ttk.LabelFrame(manual_tab, text="Manual synchronized servo motion", padding=8)
        manual_frame.grid(row=0, column=0, sticky="ew")
        manual_names = ("S1 / D3 base", "S2 / D4 shoulder", "S3 / D5 elbow", "S4 / D6 wrist")
        for index, (name, limits) in enumerate(zip(manual_names, DIRECT_LIMITS)):
            ttk.Label(manual_frame, text=name, width=18).grid(row=index, column=0, sticky="w")
            scale = tk.Scale(
                manual_frame,
                from_=limits[0],
                to=limits[1],
                resolution=0.5,
                orient="horizontal",
                length=270,
                showvalue=False,
                variable=self.manual_angles[index],
                command=lambda value, servo=index: self.manual_slider_changed(servo, value),
            )
            scale.grid(row=index, column=1, padx=5)
            ttk.Label(manual_frame, textvariable=self.manual_angle_text[index], width=8).grid(
                row=index, column=2, sticky="e"
            )
        manual_buttons = ttk.Frame(manual_frame)
        manual_buttons.grid(row=4, column=0, columnspan=3, sticky="w", pady=(5, 0))
        ttk.Button(manual_buttons, text="Send manual pose", command=self.send_manual_pose).grid(
            row=0, column=0
        )
        ttk.Button(
            manual_buttons,
            text="Copy current IK target",
            command=self.copy_ik_target_to_manual,
        ).grid(row=0, column=1, padx=6)

        automatic = ttk.LabelFrame(manual_tab, text="Idle automatic motion", padding=8)
        automatic.grid(row=1, column=0, sticky="ew", pady=(7, 0))
        ttk.Checkbutton(
            automatic,
            text="Enable wide automatic motion after 15 seconds AFK",
            variable=self.auto_enabled,
            command=self.automatic_setting_changed,
        ).grid(row=0, column=0, sticky="w")
        ttk.Label(automatic, textvariable=self.auto_status).grid(row=1, column=0, sticky="w")
        auto_settings = ttk.Frame(automatic)
        auto_settings.grid(row=2, column=0, sticky="w", pady=4)
        ttk.Label(auto_settings, text="Sweep size (%)").grid(row=0, column=0)
        sweep_box = ttk.Combobox(auto_settings, textvariable=self.auto_sweep_percent,
                                values=(25, 50, 75, 100), state="readonly", width=5)
        sweep_box.grid(row=0, column=1, padx=6)
        ttk.Label(auto_settings, text="Pace").grid(row=0, column=2)
        pace_box = ttk.Combobox(auto_settings, textvariable=self.auto_pace,
                               values=("Slow", "Flow", "Brisk"), state="readonly", width=7)
        pace_box.grid(row=0, column=3, padx=6)
        for box in (sweep_box, pace_box):
            box.bind("<<ComboboxSelected>>", lambda _event: self.automatic_setting_changed())
        ttk.Label(
            automatic,
            text="Manual sliders, map input, HOLD, or D7 immediately cancels auto motion.",
        ).grid(row=3, column=0, sticky="w")

        direction = ttk.LabelFrame(
            calibration_tab, text="Direct servo direction test — no IK", padding=8
        )
        direction.grid(row=0, column=0, sticky="ew")
        ttk.Label(
            direction,
            text="Physical mode discovers each sign. Verification mode applies the selected +1/−1 sign.",
        ).grid(row=0, column=0, columnspan=5, sticky="w", pady=(0, 3))
        ttk.Checkbutton(
            direction,
            text="Verification mode: apply direction value to S1–S3 jogs",
            variable=self.direction_aware_jogs,
        ).grid(row=1, column=0, columnspan=5, sticky="w", pady=(0, 5))
        direction_notes = (
            "S1 / D3 — positive model direction: counter-clockwise viewed from above",
            "S2 / D4 — positive model direction: upper arm rises",
            "S3 / D5 — positive model direction: forearm bends upward relative to upper arm",
            "S4 / D6 — wrist roll only; its sign is not used by position IK yet",
        )
        direction_keys = ("base_direction", "shoulder_direction", "elbow_direction")
        for index, note in enumerate(direction_notes, start=1):
            row = index + 1
            ttk.Label(direction, text=note, width=70).grid(row=row, column=0, sticky="w")
            ttk.Button(
                direction,
                text="− jog",
                width=7,
                command=lambda servo=index: self.jog_servo(servo, -1),
            ).grid(row=row, column=1, padx=(8, 3), pady=2)
            ttk.Button(
                direction,
                text="+ jog",
                width=7,
                command=lambda servo=index: self.jog_servo(servo, 1),
            ).grid(row=row, column=2, pady=2)
            if index <= 3:
                ttk.Button(
                    direction,
                    text="Set dir +1",
                    width=10,
                    command=lambda key=direction_keys[index - 1]: self.set_direction(key, 1),
                ).grid(row=row, column=3, padx=(8, 3), pady=2)
                ttk.Button(
                    direction,
                    text="Set dir −1",
                    width=10,
                    command=lambda key=direction_keys[index - 1]: self.set_direction(key, -1),
                ).grid(row=row, column=4, pady=2)

        ttk.Button(
            direction,
            text="Return to calibration pose",
            command=self.return_to_calibration_pose,
        ).grid(row=6, column=0, sticky="w", pady=(6, 0))

        controls = ttk.Frame(output)
        controls.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        ttk.Button(controls, text="Preview IK", command=self.preview).grid(row=0, column=0)
        ttk.Button(controls, text="Send synchronized move", command=self.send_move).grid(
            row=0, column=1, padx=6
        )
        ttk.Button(controls, text="HOLD", command=self.hold).grid(row=0, column=2)
        ttk.Checkbutton(
            controls,
            text="Arm area is clear",
            variable=self.arm_clear,
            command=self.safety_setting_changed,
        ).grid(
            row=1, column=0, columnspan=3, sticky="w", pady=(7, 0)
        )
        ttk.Button(controls, text="Save calibration", command=self.save_calibration).grid(
            row=2, column=0, pady=(7, 0)
        )
        ttk.Button(controls, text="Load calibration", command=self.load_calibration).grid(
            row=2, column=1, pady=(7, 0)
        )

    def _entry_row(self, parent: ttk.Widget, row: int, label: str, key: str) -> None:
        ttk.Label(parent, text=label).grid(row=row, column=0, sticky="w", padx=(0, 8))
        ttk.Entry(parent, textvariable=self.values[key], width=12).grid(row=row, column=1, pady=2)

    def number(self, key: str) -> float:
        try:
            return float(self.values[key].get())
        except ValueError as error:
            raise ValueError(f"{key.replace('_', ' ').title()} must be a number.") from error

    def solve(self) -> dict[str, object]:
        base_height = self.number("base_height")
        shoulder_offset = self.number("shoulder_offset")
        upper = self.number("upper_arm")
        lower = self.number("forearm") + self.number("tool_inline")
        x = self.number("target_x")
        y = self.number("target_y")
        z = self.number("target_z")

        if upper <= 0 or lower <= 0:
            raise ValueError("Arm lengths must be greater than zero.")

        radial_from_base = math.hypot(x, y)
        radial = radial_from_base - shoulder_offset
        vertical = z - base_height
        if radial < 0:
            raise ValueError("Target is inside the base-to-shoulder radial offset.")

        cosine_elbow = (
            radial * radial + vertical * vertical - upper * upper - lower * lower
        ) / (2 * upper * lower)
        if cosine_elbow < -1.0 or cosine_elbow > 1.0:
            distance = math.hypot(radial, vertical)
            raise ValueError(
                f"Target is unreachable: shoulder distance is {distance:.1f} mm; "
                f"model reach is {abs(upper-lower):.1f}–{upper+lower:.1f} mm."
            )

        sign = 1.0 if self.elbow_solution.get() == "Positive" else -1.0
        elbow = math.atan2(sign * math.sqrt(max(0.0, 1.0 - cosine_elbow**2)), cosine_elbow)
        shoulder = math.atan2(vertical, radial) - math.atan2(
            lower * math.sin(elbow), upper + lower * math.cos(elbow)
        )
        base = math.atan2(y, x)

        joint_degrees = tuple(math.degrees(value) for value in (base, shoulder, elbow))
        zeroes = tuple(self.number(key) for key in ("base_zero", "shoulder_zero", "elbow_zero"))
        directions = tuple(
            self.number(key) for key in ("base_direction", "shoulder_direction", "elbow_direction")
        )
        if any(direction not in (-1.0, 1.0) for direction in directions):
            raise ValueError("Every direction must be exactly +1 or -1.")

        servo_angles = tuple(
            zero + direction * joint
            for zero, direction, joint in zip(zeroes, directions, joint_degrees)
        )
        for index, (angle, limits) in enumerate(zip(servo_angles, DIRECT_LIMITS), start=1):
            if not limits[0] <= angle <= limits[1]:
                raise ValueError(
                    f"IK is geometric, but Servo {index} would be {angle:.1f}° "
                    f"outside its {limits[0]:g}–{limits[1]:g}° limit."
                )

        joint4 = self.number("joint4")
        if not DIRECT_LIMITS[3][0] <= joint4 <= DIRECT_LIMITS[3][1]:
            raise ValueError("Joint 4 is outside its 0–270° limit.")

        elbow_r = shoulder_offset + upper * math.cos(shoulder)
        elbow_z = base_height + upper * math.sin(shoulder)
        tool_r = elbow_r + lower * math.cos(shoulder + elbow)
        tool_z = elbow_z + lower * math.sin(shoulder + elbow)

        return {
            "joint_degrees": joint_degrees,
            "controls": (*servo_angles, joint4),
            "points": ((shoulder_offset, base_height), (elbow_r, elbow_z), (tool_r, tool_z)),
            "target": (radial_from_base, z),
        }

    def preview(self) -> dict[str, object] | None:
        try:
            solution = self.solve()
        except ValueError as error:
            self.last_solution = None
            self.result_text.set(f"NOT READY\n{error}")
            self.draw_preview(None)
            return None

        joints = solution["joint_degrees"]
        controls = solution["controls"]
        assert isinstance(joints, tuple) and isinstance(controls, tuple)
        self.result_text.set(
            "REACHABLE — IK target calculated\n"
            f"Model joints: base {joints[0]:.1f}°, shoulder {joints[1]:.1f}°, "
            f"elbow {joints[2]:.1f}°\n"
            f"Servo targets: S1 {controls[0]:.1f}°, S2 {controls[1]:.1f}°, "
            f"S3 {controls[2]:.1f}°, S4 {controls[3]:.1f}°"
        )
        self.last_solution = solution
        self.draw_preview(solution)
        return solution

    def draw_preview(self, solution: dict[str, object] | None) -> None:
        self.canvas.delete("all")
        width, height, margin = 430, 300, 30
        self.canvas.create_text(
            10,
            8,
            anchor="nw",
            text="Click or drag across green positions to move — radial distance vs Z (mm)",
        )

        try:
            shoulder_offset = self.number("shoulder_offset")
            base_height = self.number("base_height")
            upper = self.number("upper_arm")
            lower = self.number("forearm") + self.number("tool_inline")
        except ValueError:
            self.map_transform = None
            self.canvas.create_text(width / 2, height / 2, text="Enter valid geometry", fill="gray")
            return

        maximum_reach = max(1.0, upper + lower)
        radial_min = 0.0
        radial_max = max(50.0, shoulder_offset + maximum_reach + 10.0)
        z_min = 0.0
        z_max = max(50.0, base_height + maximum_reach + 10.0)
        available_width = width - 2 * margin
        available_height = height - 2 * margin
        scale = min(
            available_width / (radial_max - radial_min),
            available_height / (z_max - z_min),
        )
        plot_width = (radial_max - radial_min) * scale
        plot_height = (z_max - z_min) * scale
        left = margin + (available_width - plot_width) / 2
        bottom = height - margin - (available_height - plot_height) / 2
        self.map_transform = (left, bottom, scale, radial_min, z_min)

        def screen(point: tuple[float, float]) -> tuple[float, float]:
            radial, z = point
            return left + (radial - radial_min) * scale, bottom - (z - z_min) * scale

        top_right = screen((radial_max, z_max))
        self.canvas.create_rectangle(left, top_right[1], top_right[0], bottom, outline="#999")
        self.canvas.create_text(top_right[0], bottom + 14, anchor="e", text=f"R {radial_max:.0f}")
        self.canvas.create_text(left - 5, top_right[1], anchor="e", text=f"Z {z_max:.0f}")
        self.canvas.create_text(left - 5, bottom, anchor="e", text="0")

        legal_points = self.legal_workspace_points(shoulder_offset, base_height, upper, lower)
        for radial, z in legal_points:
            px, py = screen((radial, z))
            self.canvas.create_rectangle(
                px - 2,
                py - 2,
                px + 2,
                py + 2,
                fill="#bfe8c8",
                outline="",
            )

        if not legal_points:
            self.canvas.create_text(
                width / 2,
                height / 2,
                text="Set all three directions to +1 or −1 to draw legal positions",
                fill="gray",
            )

        if solution is None:
            return

        points = solution["points"]
        target = solution["target"]
        assert isinstance(points, tuple) and isinstance(target, tuple)
        origin = screen((0.0, 0.0))
        shoulder = points[0]
        base_top = (0.0, shoulder[1])
        base_top_screen = screen(base_top)
        shoulder_screen = screen(shoulder)

        # The fixed base transform has two components: vertical height, then
        # the optional horizontal base-to-shoulder radial offset. Drawing one
        # diagonal line between them incorrectly made the base look tilted.
        self.canvas.create_line(*origin, *base_top_screen, width=5, fill="#555")
        self.canvas.create_oval(
            base_top_screen[0] - 4,
            base_top_screen[1] - 4,
            base_top_screen[0] + 4,
            base_top_screen[1] + 4,
            fill="#555",
        )
        if abs(shoulder[0]) > 0.01:
            self.canvas.create_line(
                *base_top_screen,
                *shoulder_screen,
                width=4,
                fill="#777",
                dash=(5, 3),
            )
        self.canvas.create_oval(
            shoulder_screen[0] - 5,
            shoulder_screen[1] - 5,
            shoulder_screen[0] + 5,
            shoulder_screen[1] + 5,
            fill="#555",
        )

        previous = shoulder_screen
        for point, color in zip(points[1:], ("#1473e6", "#16a085")):
            current = screen(point)
            self.canvas.create_line(*previous, *current, width=5, fill=color)
            self.canvas.create_oval(
                current[0] - 5,
                current[1] - 5,
                current[0] + 5,
                current[1] + 5,
                fill=color,
            )
            previous = current

        target_screen = screen(target)
        self.canvas.create_oval(
            target_screen[0] - 7,
            target_screen[1] - 7,
            target_screen[0] + 7,
            target_screen[1] + 7,
            outline="red",
            width=2,
        )

    def legal_workspace_points(
        self,
        shoulder_offset: float,
        base_height: float,
        upper: float,
        lower: float,
    ) -> list[tuple[float, float]]:
        try:
            base_zero = self.number("base_zero")
            shoulder_zero = self.number("shoulder_zero")
            elbow_zero = self.number("elbow_zero")
            base_direction = self.number("base_direction")
            shoulder_direction = self.number("shoulder_direction")
            elbow_direction = self.number("elbow_direction")
            joint4 = self.number("joint4")
        except ValueError:
            return []
        if any(value not in (-1.0, 1.0) for value in (
            base_direction,
            shoulder_direction,
            elbow_direction,
        )):
            return []
        if not DIRECT_LIMITS[0][0] <= base_zero <= DIRECT_LIMITS[0][1]:
            return []
        if not DIRECT_LIMITS[3][0] <= joint4 <= DIRECT_LIMITS[3][1]:
            return []

        def model_range(zero: float, direction: float, limits: tuple[float, float]) -> tuple[float, float]:
            endpoints = ((limits[0] - zero) / direction, (limits[1] - zero) / direction)
            return min(endpoints), max(endpoints)

        shoulder_min, shoulder_max = model_range(
            shoulder_zero, shoulder_direction, DIRECT_LIMITS[1]
        )
        elbow_min, elbow_max = model_range(elbow_zero, elbow_direction, DIRECT_LIMITS[2])
        if self.elbow_solution.get() == "Positive":
            elbow_min = max(elbow_min, 0.0)
            elbow_max = min(elbow_max, 180.0)
        else:
            elbow_min = max(elbow_min, -180.0)
            elbow_max = min(elbow_max, 0.0)
        if shoulder_min > shoulder_max or elbow_min > elbow_max:
            return []

        points: list[tuple[float, float]] = []
        shoulder = shoulder_min
        while shoulder <= shoulder_max + 0.001:
            elbow = elbow_min
            shoulder_radians = math.radians(shoulder)
            while elbow <= elbow_max + 0.001:
                elbow_radians = math.radians(elbow)
                radial = shoulder_offset + upper * math.cos(shoulder_radians)
                radial += lower * math.cos(shoulder_radians + elbow_radians)
                z = base_height + upper * math.sin(shoulder_radians)
                z += lower * math.sin(shoulder_radians + elbow_radians)
                if radial >= shoulder_offset and z >= 0.0:
                    points.append((radial, z))
                elbow += 3.0
            shoulder += 3.0
        return points

    def map_press(self, event: tk.Event) -> None:
        self.mark_user_activity("Map control")
        self.map_is_dragging = False
        self.pending_map_xy = None
        self.select_map_position(event.x, event.y, streaming=False)

    def map_drag(self, event: tk.Event) -> None:
        self.map_is_dragging = True
        self.pending_map_xy = (event.x, event.y)
        if self.map_drag_job is None:
            self.map_drag_job = self.root.after(100, self.flush_map_drag)

    def map_release(self, event: tk.Event) -> None:
        if not self.map_is_dragging:
            return
        self.map_is_dragging = False
        self.pending_map_xy = (event.x, event.y)
        if self.map_drag_job is not None:
            self.root.after_cancel(self.map_drag_job)
            self.map_drag_job = None
        self.flush_map_drag()

    def flush_map_drag(self) -> None:
        self.map_drag_job = None
        if self.pending_map_xy is None:
            return
        x, y = self.pending_map_xy
        self.pending_map_xy = None
        self.select_map_position(x, y, streaming=True)

    def select_map_position(self, screen_x: int, screen_y: int, streaming: bool) -> None:
        if self.map_transform is None:
            return
        left, bottom, scale, radial_min, z_min = self.map_transform
        radial = radial_min + (screen_x - left) / scale
        z = z_min + (bottom - screen_y) / scale
        if radial < 0.0 or z < 0.0:
            self.status.set("That click is outside the map")
            return

        old_target = (
            self.values["target_x"].get(),
            self.values["target_y"].get(),
            self.values["target_z"].get(),
        )
        self.values["target_x"].set(f"{radial:.1f}")
        self.values["target_y"].set("0")
        self.values["target_z"].set(f"{z:.1f}")
        solution = self.preview()
        if solution is None:
            self.values["target_x"].set(old_target[0])
            self.values["target_y"].set(old_target[1])
            self.values["target_z"].set(old_target[2])
            self.preview()
            self.status.set("Illegal point — click inside the green workspace")
            return
        controls = solution["controls"]
        assert isinstance(controls, tuple)
        self.update_manual_angles(controls)
        self.mark_user_activity("Map control")
        if not self.arm_clear.get():
            self.status.set(
                f"Selected X={radial:.1f}, Y=0, Z={z:.1f} mm — check Arm area is clear to move"
            )
            return
        if not self.ready or not self.connection or not self.connection.is_open:
            self.status.set(
                f"Selected X={radial:.1f}, Y=0, Z={z:.1f} mm — connect to move"
            )
            return
        duration_ms = 450 if streaming else 700
        self.send_controls(controls, duration_ms)
        action = "Dragging" if streaming else "Moving"
        self.status.set(f"{action} to X={radial:.1f}, Y=0, Z={z:.1f} mm")

    def manual_slider_changed(self, servo_index: int, value: str) -> None:
        numeric_value = float(value)
        self.manual_angle_text[servo_index].set(f"{numeric_value:.1f}°")
        if self.manual_updating:
            return
        # Tk Scale may deliver its command after programmatic updates complete.
        # Those callbacks must not interrupt auto or inject a 400 ms manual move.
        expected = self.programmatic_slider_values[servo_index]
        if expected is not None and abs(numeric_value - expected) <= 0.26:
            return
        self.programmatic_slider_values[servo_index] = None
        self.mark_user_activity("Manual control")
        if self.manual_send_job is not None:
            self.root.after_cancel(self.manual_send_job)
        self.manual_send_job = self.root.after(75, lambda: self.send_manual_pose(streaming=True))

    def update_manual_angles(self, controls: tuple[float, ...]) -> None:
        self.manual_updating = True
        try:
            for index, value in enumerate(controls[:4]):
                self.programmatic_slider_values[index] = float(value)
                self.manual_angles[index].set(float(value))
                self.manual_angle_text[index].set(f"{float(value):.1f}°")
        finally:
            self.manual_updating = False

    def send_manual_pose(self, streaming: bool = False) -> None:
        self.manual_send_job = None
        controls = tuple(variable.get() for variable in self.manual_angles)
        for index, (angle, limits) in enumerate(zip(controls, DIRECT_LIMITS), start=1):
            if not limits[0] <= angle <= limits[1]:
                if not streaming:
                    messagebox.showerror(
                        "Manual limit", f"Servo {index} is outside {limits[0]:g}–{limits[1]:g}°."
                    )
                return
        self.mark_user_activity("Manual control")
        if streaming:
            if not self.arm_clear.get() or not self.ready:
                self.status.set("Manual pose ready — connect and clear the arm to move")
                return
        elif not self.motion_is_ready():
            return
        self.send_controls(controls, 400 if streaming else 700)
        self.status.set("Manual synchronized command sent")

    def copy_ik_target_to_manual(self) -> None:
        solution = self.preview()
        if solution is None:
            return
        controls = solution["controls"]
        assert isinstance(controls, tuple)
        self.update_manual_angles(controls)
        self.mark_user_activity("Manual control")
        self.status.set("IK target copied to the manual sliders — not moved yet")

    def send_move(self) -> None:
        solution = self.preview()
        if solution is None:
            return
        if not self.motion_is_ready():
            return

        duration_ms = round(self.number("move_seconds") * 1000)
        duration_ms = max(300, min(15000, duration_ms))
        controls = solution["controls"]
        assert isinstance(controls, tuple)
        self.update_manual_angles(controls)
        self.mark_user_activity("IK control")
        self.send_controls(controls, duration_ms)

    def send_controls(self, controls: tuple[float, ...], duration_ms: int) -> None:
        command = "MOVE," + ",".join(f"{value:.2f}" for value in controls[:4])
        self.write_serial(f"{command},{duration_ms}\n")

    def mark_user_activity(self, source: str) -> None:
        self.last_user_activity = time.monotonic()
        if self.auto_running:
            self.auto_running = False
            self.auto_waypoints = []
        if self.auto_enabled.get():
            self.auto_status.set(f"{source} override — automatic motion in 15.0 s")

    def automatic_setting_changed(self) -> None:
        self.last_user_activity = time.monotonic()
        if self.auto_running:
            self.write_serial("HOLD\n")
        self.auto_running = False
        self.auto_waypoints = []
        if not self.auto_enabled.get():
            if self.auto_running:
                self.write_serial("HOLD\n")
            self.auto_running = False
            self.auto_waypoints = []
            self.auto_status.set("Automatic motion disabled")
        else:
            self.auto_running = False
            self.auto_status.set("Automatic motion enabled — starts after 15.0 s AFK")

    def safety_setting_changed(self) -> None:
        self.last_user_activity = time.monotonic()
        if not self.arm_clear.get() and self.auto_running:
            self.write_serial("HOLD\n")
            self.auto_running = False
            self.auto_waypoints = []
        if self.arm_clear.get() and self.auto_enabled.get():
            self.auto_status.set("Safety confirmed — automatic motion starts after 15.0 s AFK")

    def check_automatic_motion(self) -> None:
        try:
            now = time.monotonic()
            if not self.auto_enabled.get():
                self.auto_status.set("Automatic motion disabled")
                return
            if not self.ready or not self.connection or not self.connection.is_open:
                self.auto_running = False
                self.auto_status.set("Automatic motion waiting for Arduino connection")
                return
            if not self.arm_clear.get():
                if self.auto_running:
                    self.write_serial("HOLD\n")
                    self.auto_running = False
                    self.auto_waypoints = []
                self.auto_status.set("Automatic motion waiting for Arm area is clear")
                return

            idle_seconds = now - self.last_user_activity
            if not self.auto_running:
                if idle_seconds < 15.0:
                    self.auto_status.set(
                        f"Automatic motion starts in {15.0 - idle_seconds:.1f} s"
                    )
                    return
                self.start_automatic_motion(now)
                return

            if now >= self.auto_next_move_at:
                self.send_next_automatic_waypoint(now)
        finally:
            self.root.after(50, self.check_automatic_motion)

    def start_automatic_motion(self, now: float) -> None:
        center = tuple(variable.get() for variable in self.manual_angles)
        try:
            waypoints = build_auto_sweep(center, float(self.auto_sweep_percent.get()))
        except ValueError as error:
            self.auto_enabled.set(False)
            self.auto_status.set(str(error))
            return
        self.auto_waypoints = waypoints
        self.auto_last_target = center
        self.auto_waypoint_index = 0
        self.auto_running = True
        self.auto_next_move_at = now
        self.auto_status.set("Automatic motion active")

    def send_next_automatic_waypoint(self, now: float) -> None:
        if not self.auto_waypoints:
            self.auto_running = False
            return
        sent_index = self.auto_waypoint_index
        waypoint = self.auto_waypoints[sent_index]
        speed = {"Slow": 12.0, "Flow": 18.0, "Brisk": 25.0}[self.auto_pace.get()]
        try:
            duration_ms = auto_move_duration_ms(self.auto_last_target, waypoint, speed)
        except ValueError as error:
            self.hold()
            self.auto_status.set(str(error))
            return
        self.auto_waypoint_index = (self.auto_waypoint_index + 1) % len(self.auto_waypoints)
        self.update_manual_angles(waypoint)
        self.send_controls(waypoint, duration_ms)
        self.auto_last_target = waypoint
        self.auto_next_move_at = time.monotonic() + duration_ms / 1000 + 0.04
        self.auto_status.set(
            f"Wide sweep {self.auto_sweep_percent.get()}%, {self.auto_pace.get()} — pose {sent_index + 1}/"
            f"{len(self.auto_waypoints)}"
        )

    def motion_is_ready(self) -> bool:
        if not self.arm_clear.get():
            messagebox.showwarning("Safety check", "Confirm that the arm area is clear before moving.")
            return False
        if not self.ready or not self.connection or not self.connection.is_open:
            messagebox.showwarning(
                "Not connected",
                "Connect the Uno running the four-servo IK4 firmware first.",
            )
            return False
        return True

    def set_direction(self, key: str, direction: int) -> None:
        self.values[key].set(str(direction))
        self.last_solution = None
        label = key.replace("_direction", "").title()
        self.result_text.set(
            f"{label} direction set to {direction:+d}.\n"
            "The previous IK result is now stale; click Preview IK to recalculate."
        )
        self.draw_preview(None)

    def jog_servo(self, servo_number: int, sign: int) -> None:
        if not self.motion_is_ready():
            return
        try:
            step = abs(self.number("jog_degrees"))
        except ValueError as error:
            messagebox.showerror("Invalid test step", str(error))
            return
        if step <= 0 or step > 10:
            messagebox.showwarning("Unsafe test step", "Use a direction-test step from 0.5° to 10°.")
            return
        applied_direction = 1.0
        jog_kind = "physical"
        if self.direction_aware_jogs.get() and servo_number <= 3:
            direction_key = ("base_direction", "shoulder_direction", "elbow_direction")[
                servo_number - 1
            ]
            try:
                applied_direction = self.number(direction_key)
            except ValueError as error:
                messagebox.showwarning("Direction not set", str(error))
                return
            if applied_direction not in (-1.0, 1.0):
                messagebox.showwarning(
                    "Direction not set",
                    f"Set Servo {servo_number}'s direction to +1 or -1 before verification.",
                )
                return
            jog_kind = "model"
        delta = sign * step * applied_direction
        self.mark_user_activity("Direction jog")
        self.write_serial(f"JOG,{servo_number},{delta:.2f},1500\n")
        tracked = [variable.get() for variable in self.manual_angles]
        limits = DIRECT_LIMITS[servo_number - 1]
        tracked[servo_number - 1] = max(
            limits[0], min(limits[1], tracked[servo_number - 1] + delta)
        )
        self.update_manual_angles(tuple(tracked))
        self.status.set(
            f"S{servo_number} {jog_kind} jog {sign * step:+.1f}° "
            f"→ physical command {delta:+.1f}°"
        )

    def return_to_calibration_pose(self) -> None:
        if not self.motion_is_ready():
            return
        try:
            angles = (
                self.number("base_zero"),
                self.number("shoulder_zero"),
                self.number("elbow_zero"),
                self.number("joint4"),
            )
            for index, (angle, limits) in enumerate(zip(angles, DIRECT_LIMITS), start=1):
                if not limits[0] <= angle <= limits[1]:
                    raise ValueError(
                        f"Calibration angle S{index} is outside {limits[0]:g}–{limits[1]:g}°."
                    )
            duration_ms = max(2000, min(15000, round(self.number("move_seconds") * 1000)))
        except ValueError as error:
            messagebox.showerror("Invalid calibration pose", str(error))
            return
        command = "MOVE," + ",".join(f"{angle:.2f}" for angle in angles)
        self.update_manual_angles(angles)
        self.mark_user_activity("Calibration pose")
        self.write_serial(f"{command},{duration_ms}\n")

    def hold(self) -> None:
        self.auto_enabled.set(False)
        self.auto_running = False
        self.auto_waypoints = []
        self.last_user_activity = time.monotonic()
        self.auto_status.set("Automatic motion disabled by HOLD")
        self.write_serial("HOLD\n")

    def write_serial(self, command: str) -> None:
        if not self.ready or not self.connection or not self.connection.is_open:
            return
        try:
            self.connection.write(command.encode("ascii"))
        except serial.SerialException as error:
            messagebox.showerror("Serial error", str(error))
            self.disconnect()

    def refresh_ports(self) -> None:
        ports = [item.device for item in list_ports.comports()]
        self.port_box["values"] = ports
        if ports and self.port.get() not in ports:
            self.port.set(ports[0])

    def toggle_connection(self) -> None:
        if self.connection and self.connection.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self) -> None:
        if not self.port.get():
            messagebox.showwarning("No port", "Select the Uno's USB serial port.")
            return
        try:
            self.connection = serial.Serial(
                self.port.get(), BAUD_RATE, timeout=0, write_timeout=0.25
            )
        except serial.SerialException as error:
            messagebox.showerror("Connection failed", str(error))
            return
        self.ready = False
        self.serial_buffer = ""
        self.connect_button.config(text="Disconnect")
        self.port_box.config(state="disabled")
        self.status.set("Connected; verifying IK4 controller…")
        self.connection.reset_input_buffer()
        self.root.after(500, self.query_controller)
        self.root.after(1800, self.query_controller)
        self.root.after(4000, self.check_handshake)

    def query_controller(self) -> None:
        if not self.ready and self.connection and self.connection.is_open:
            try:
                self.connection.write(b"ID\n")
            except serial.SerialException:
                self.disconnect()

    def check_handshake(self) -> None:
        if self.connection and self.connection.is_open and not self.ready:
            self.status.set("Wrong firmware or no IK4 response — upload the included IK4 sketch")

    def disconnect(self) -> None:
        self.ready = False
        self.auto_running = False
        self.auto_waypoints = []
        if self.connection and self.connection.is_open:
            self.connection.close()
        self.connection = None
        self.connect_button.config(text="Connect")
        self.port_box.config(state="readonly")
        self.status.set("Disconnected — preview mode is available")

    def poll_serial(self) -> None:
        try:
            if self.connection and self.connection.is_open and self.connection.in_waiting:
                chunk = self.connection.read(self.connection.in_waiting).decode(
                    "ascii", errors="replace"
                )
                self.serial_buffer += chunk.replace("\r", "")
                while "\n" in self.serial_buffer:
                    line, self.serial_buffer = self.serial_buffer.split("\n", 1)
                    line = line.strip()
                    if not line:
                        continue
                    if line == "READY,IK4,1":
                        self.ready = True
                        self.last_user_activity = time.monotonic()
                        self.status.set(f"IK4 controller verified at {BAUD_RATE} baud")
                    elif line == "OK,SWITCH HOLD":
                        self.auto_enabled.set(False)
                        self.auto_running = False
                        self.auto_waypoints = []
                        self.last_user_activity = time.monotonic()
                        self.auto_status.set("Automatic motion disabled by D7 hold switch")
                        self.status.set(line)
                    elif line.startswith("ERROR") and not self.ready:
                        self.status.set("Wrong firmware on this port — upload the IK4 sketch")
                    else:
                        self.status.set(line)
        except serial.SerialException:
            self.disconnect()
        finally:
            self.root.after(50, self.poll_serial)

    def save_calibration(self) -> None:
        data = {key: value.get() for key, value in self.values.items()}
        data["elbow_solution"] = self.elbow_solution.get()
        CALIBRATION_FILE.write_text(json.dumps(data, indent=2), encoding="utf-8")
        self.status.set(f"Saved {CALIBRATION_FILE.name}")

    def load_calibration(self) -> None:
        if not CALIBRATION_FILE.exists():
            messagebox.showinfo("No calibration", "No saved calibration file exists yet.")
            return
        data = json.loads(CALIBRATION_FILE.read_text(encoding="utf-8"))
        for key, variable in self.values.items():
            if key in data:
                variable.set(str(data[key]))
        if "elbow_solution" in data:
            self.elbow_solution.set(str(data["elbow_solution"]))
        solution = self.preview()
        if solution is not None:
            controls = solution["controls"]
            assert isinstance(controls, tuple)
            self.update_manual_angles(controls)
        self.last_user_activity = time.monotonic()
        self.status.set(f"Loaded {CALIBRATION_FILE.name}")

    def close(self) -> None:
        self.disconnect()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    IKCalibrationApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
