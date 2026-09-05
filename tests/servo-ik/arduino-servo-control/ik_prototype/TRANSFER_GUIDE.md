# Four-servo robot arm handoff guide

This folder is the complete current project for an Arduino Uno R4 WiFi arm with
four 270-degree servos. It includes manual control, an idle automatic demo, and
clickable/drag-controlled inverse kinematics (IK).

## Files to use

- `ik_prototype_controller/ik_prototype_controller.ino` — upload this to the Uno R4 WiFi.
- `ik_calibration_gui.py` — run this desktop GUI with Python.
- `ik_calibration.json` — the current measured geometry and calibration.
- `requirements.txt` — Python dependency list.
- `README.md` — detailed operating and calibration instructions.

## Mechanical model

The arm is modeled as a base-yaw joint followed by a two-link planar arm:

1. **S1 / base yaw:** rotates the entire arm around the vertical base axis.
2. **S2 / shoulder pitch:** raises or lowers the 120 mm upper arm.
3. **S3 / elbow pitch:** bends the 50 mm forearm relative to the upper arm.
4. **S4 / wrist roll:** rotates around the forearm axis. It does not change the
   calculated wrist-centre X/Y/Z position and is held at a chosen physical angle
   during IK motion.

Measured/model dimensions, in millimetres:

| Geometry item | Value | Meaning |
|---|---:|---|
| Base height | 70 | Base plate to shoulder-axis height |
| Base-to-shoulder radial | 25 | Horizontal offset from base axis to shoulder axis |
| Shoulder-to-elbow | 120 | Upper-arm joint-centre distance |
| Elbow-to-wrist | 50 | Forearm joint-centre distance |
| Inline wrist-to-tool | 0 | Extra length beyond the wrist axis |

With the inline tool length at zero, IK targets the centre of the S4 wrist-roll
axis. Remeasure centre-to-centre distances and update the GUI fields if the printed
parts or tool change.

Coordinate convention:

- `X` points forward from the base.
- `Y` points sideways.
- `Z` points upward from the base plate.
- The clickable 2D map deliberately sets `Y = 0`; typed targets may use Y.
- Base model theta is zero when the arm faces forward.
- Shoulder model theta is zero when the upper arm is horizontal and outward.
- Elbow model theta is zero when the forearm is straight and collinear with the
  upper arm.

The kinematic conversion is:

`physical servo angle = zero angle + direction × model theta`

## Calibration and theta constraints

| Joint | Pin | Physical range | Zero physical angle | Direction | Model-theta range implied by calibration |
|---|---:|---:|---:|---:|---:|
| S1 base | D3 | 0–270° | 33.5° | +1 | -33.5° to +236.5° |
| S2 shoulder | D4 | 0–112.5° | 112.5° | -1 | 0° to +112.5° |
| S3 elbow | D5 | 90–270° | 151.5° | +1 | -61.5° to +118.5° |
| S4 wrist roll | D6 | 0–270° | held at 27.5° | not used by position IK | not applicable |

The saved IK branch is **Negative**, so normal mapped solutions restrict elbow
theta to the negative part of its allowed range: approximately -61.5° to 0°.
Selecting Positive uses approximately 0° to +118.5°. These are software limits,
not proof of collision-free mechanical travel.

The straight calibration pose commands S1=33.5°, S2=112.5°, S3=151.5°, and
S4=27.5°. Support the arm before commanding this pose because ordinary hobby
servos provide no position feedback to the program.

## Electrical wiring

| Connection | Uno R4 WiFi pin |
|---|---:|
| Servo 1 signal | D3 |
| Servo 2 signal | D4 |
| Servo 3 signal | D5 |
| Servo 4 signal | D6 |
| Joystick switch | D7 and GND |
| USB serial | Uno R4 USB port |

Use a suitable external servo power supply. Connect external-supply ground,
every servo ground, and Uno GND together. Do not power four high-current servos
from the Uno 5 V pin. D7 uses the Uno's internal pull-up resistor; pressing the
switch connects D7 to GND and cancels the active trajectory.

## Setup on a new computer

1. Install Arduino IDE and select **Arduino Uno R4 WiFi** as the board.
2. Open and upload `ik_prototype_controller/ik_prototype_controller.ino`.
3. Install Python 3 with Tkinter, then open a terminal in this folder and run:
   `py -m pip install -r requirements.txt`
4. Start the GUI with: `py ik_calibration_gui.py`
5. Choose the Uno's COM port and click **Connect**. Do not proceed unless the
   status says `IK4 controller verified at 115200 baud`.
6. Leave **Arm area is clear** unchecked while inspecting the calibration. Turn
   off automatic mode initially, or be aware that it begins after 15 seconds of
   inactivity once connected and safety is confirmed.
7. Support the mechanism, clear cables and obstacles, turn on the external servo
   supply, check **Arm area is clear**, and test small manual or 3-degree jogs.
8. Start the automatic demo at 25% sweep and Flow pace. Inspect the complete path
   before increasing it to 50–100%.

## Motion and safety behavior

- Serial baud rate: 115200.
- All four servos share one quintic motion timeline and update every 20 ms.
- Manual sliders send synchronized physical angles.
- A legal map click sends IK immediately; holding and dragging streams targets.
- Automatic motion starts after 15 seconds without a user motion command.
- Manual, map, IK, calibration, or direction-jog input overrides automatic motion.
- GUI **HOLD** and the D7 switch cancel the current trajectory but do **not**
  remove servo torque. Cut external servo power for an emergency stop.
- There is no encoder feedback, homing sensor, collision checking, cable-twist
  model, load compensation, or physical arrival verification.

The included values are a working prototype calibration, not final metrology.
Recheck joint-centre lengths, zero positions, signs, mechanical stops, and tool
offsets on the receiving arm before relying on IK accuracy.
