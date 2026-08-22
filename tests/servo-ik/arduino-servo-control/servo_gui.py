"""Beginner-friendly USB servo controller for an Arduino Uno R4 WiFi."""

import tkinter as tk
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports


BAUD_RATE = 115200
SERVO_COUNT = 4
START_ANGLE = 90
SEND_DELAY_MS = 30
SERIAL_POLL_MS = 50
SERVO_PINS = (3, 5, 6, 9)

# Physical angles shown by the GUI for the four 270-degree servos.
# These are the original 0-180 limits multiplied by 1.5.
SERVO_RANGES = (
    (0.0, 270.0),
    (0.0, 112.5),
    (90.0, 270.0),
    (0.0, 270.0),
)


class ServoControllerApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Arduino Uno Servo Control")
        self.root.resizable(False, False)

        self.connection: serial.Serial | None = None
        self.ready = False
        self.sliders: list[ttk.Scale] = []
        self.angle_labels: list[ttk.Label] = []
        self.pending_angles: dict[int, float] = {}
        self.send_jobs: dict[int, str] = {}

        self.port_name = tk.StringVar()
        self.status = tk.StringVar(value="Disconnected")

        self._build_interface()
        self.refresh_ports()
        self.root.after(SERIAL_POLL_MS, self.poll_serial)
        self.root.protocol("WM_DELETE_WINDOW", self.close_app)

    def _build_interface(self) -> None:
        outer = ttk.Frame(self.root, padding=16)
        outer.grid(sticky="nsew")

        ttk.Label(outer, text="Arduino USB port:").grid(row=0, column=0, sticky="w")
        self.port_box = ttk.Combobox(
            outer, textvariable=self.port_name, width=28, state="readonly"
        )
        self.port_box.grid(row=0, column=1, padx=8)

        ttk.Button(outer, text="Refresh", command=self.refresh_ports).grid(row=0, column=2)
        self.connect_button = ttk.Button(outer, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=3, padx=(8, 0))

        ttk.Separator(outer).grid(row=1, column=0, columnspan=4, sticky="ew", pady=14)

        for index in range(SERVO_COUNT):
            servo_number = index + 1
            pin = SERVO_PINS[index]
            minimum_angle, maximum_angle = SERVO_RANGES[index]
            ttk.Label(outer, text=f"Servo {servo_number} (D{pin})").grid(
                row=index + 2, column=0, sticky="w", pady=8
            )

            slider = ttk.Scale(
                outer,
                from_=minimum_angle,
                to=maximum_angle,
                value=START_ANGLE,
                length=280,
                command=lambda value, n=servo_number: self.send_angle(n, value),
            )
            slider.state(["disabled"])
            slider.grid(row=index + 2, column=1, columnspan=2, padx=8)
            self.sliders.append(slider)

            angle_label = ttk.Label(outer, text=f"{START_ANGLE}°", width=7)
            angle_label.grid(row=index + 2, column=3)
            self.angle_labels.append(angle_label)

        ttk.Separator(outer).grid(
            row=SERVO_COUNT + 2, column=0, columnspan=4, sticky="ew", pady=14
        )
        ttk.Label(outer, textvariable=self.status).grid(
            row=SERVO_COUNT + 3, column=0, columnspan=4, sticky="w"
        )

    def refresh_ports(self) -> None:
        ports = [port.device for port in list_ports.comports()]
        self.port_box["values"] = ports
        if ports and self.port_name.get() not in ports:
            self.port_name.set(ports[0])
        elif not ports:
            self.port_name.set("")
            self.status.set("No serial ports found. Connect the Uno and press Refresh.")

    def toggle_connection(self) -> None:
        if self.connection and self.connection.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self) -> None:
        port = self.port_name.get()
        if not port:
            messagebox.showwarning("No port selected", "Connect the Arduino and choose its USB port.")
            return

        try:
            self.connection = serial.Serial(
                port,
                BAUD_RATE,
                timeout=0,
                write_timeout=0.25,
            )
        except serial.SerialException as error:
            messagebox.showerror("Connection failed", str(error))
            return

        # Opening the port resets most Uno boards. Give the sketch two seconds to start.
        self.ready = False
        self.connect_button.config(text="Disconnect")
        self.port_box.config(state="disabled")
        self.status.set(f"Connected to {port}; waiting for Arduino...")
        self.root.after(2000, self._finish_connection)

    def _finish_connection(self) -> None:
        if not self.connection or not self.connection.is_open:
            return

        self.connection.reset_input_buffer()
        self.ready = True
        for slider in self.sliders:
            slider.state(["!disabled"])
        self.status.set(f"Connected at {BAUD_RATE} baud")

        # Apply all positions shown in the GUI after the Arduino restarts.
        for servo_number, slider in enumerate(self.sliders, start=1):
            self.send_angle(servo_number, slider.get())

    def send_angle(self, servo_number: int, value: str | float) -> None:
        # Half-degree precision preserves Servo 2's converted 112.5° limit.
        angle = round(float(value) * 2) / 2
        angle_text = f"{angle:g}"
        self.angle_labels[servo_number - 1].config(text=f"{angle_text}°")

        if not self.ready or not self.connection or not self.connection.is_open:
            return

        # A slider can generate hundreds of callbacks per second. Keep only the
        # newest angle and send it after a short pause instead of flooding USB.
        self.pending_angles[servo_number] = angle
        previous_job = self.send_jobs.pop(servo_number, None)
        if previous_job is not None:
            self.root.after_cancel(previous_job)

        self.send_jobs[servo_number] = self.root.after(
            SEND_DELAY_MS,
            lambda n=servo_number: self._send_pending_angle(n),
        )

    def _send_pending_angle(self, servo_number: int) -> None:
        self.send_jobs.pop(servo_number, None)
        angle = self.pending_angles.pop(servo_number, None)

        if angle is None:
            return
        if not self.ready or not self.connection or not self.connection.is_open:
            return

        try:
            command = f"{servo_number},{angle:g}\n"
            self.connection.write(command.encode("ascii"))
        except serial.SerialException as error:
            messagebox.showerror("Serial error", str(error))
            self.disconnect()

    def poll_serial(self) -> None:
        """Drain replies so the Arduino-to-PC serial buffer cannot fill."""
        try:
            if self.connection and self.connection.is_open:
                waiting = self.connection.in_waiting
                if waiting:
                    self.connection.read(waiting)
        except serial.SerialException as error:
            messagebox.showerror("Serial error", str(error))
            self.disconnect()
        finally:
            self.root.after(SERIAL_POLL_MS, self.poll_serial)

    def disconnect(self) -> None:
        self.ready = False

        for job in self.send_jobs.values():
            self.root.after_cancel(job)
        self.send_jobs.clear()
        self.pending_angles.clear()

        if self.connection and self.connection.is_open:
            self.connection.close()
        self.connection = None

        for slider in self.sliders:
            slider.state(["disabled"])
        self.port_box.config(state="readonly")
        self.connect_button.config(text="Connect")
        self.status.set("Disconnected")

    def close_app(self) -> None:
        self.disconnect()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    ServoControllerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
