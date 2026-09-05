# Four-servo comprehensive arm controller

This version combines manual synchronized sliders, automatic idle motion,
direction calibration, and clickable/drag-controlled position IK for the four
lower arm servos.

## Uno R4 wiring

| Part | Uno R4 pin | Purpose |
|---|---:|---|
| Servo 1 signal | D3 | Base yaw |
| Servo 2 signal | D4 | Shoulder pitch |
| Servo 3 signal | D5 | Elbow pitch |
| Servo 4 signal | D6 | Wrist roll |
| Joystick SW | D7 | Physical motion hold |
| Joystick GND | GND | Switch return |

Power the servos from a suitable external servo supply. Join the supply ground,
servo grounds, and Uno GND. Do not power four high-current servos from the Uno's
5 V pin. The D7 switch uses `INPUT_PULLUP`, so it needs only D7 and GND.

## Install and start

1. Support the arm and switch off external servo power.
2. Upload `ik_prototype_controller/ik_prototype_controller.ino` to the Uno R4.
3. Install the Python requirement with `py -m pip install -r ../requirements.txt`.
4. Run `py ik_calibration_gui.py`.
5. Select the Uno port and click **Connect**. The status must say
   **IK4 controller verified**. The GUI rejects the older incompatible firmware.
6. Set the direction-test step to 3 degrees and start with the 3-second smooth preset.
7. Support the arm, clear the mechanism, turn on servo power, and check
   **Arm area is clear**.

The controller starts at the measured calibration pose:

- S1: 33.5 degrees
- S2: 112.5 degrees
- S3: 151.5 degrees
- S4: 27.5 degrees

Keep a hand near the external servo-power switch during the first test.

## Determine each direction sign

Direction means which way the mechanism moves when its physical servo command
increases. With **Verification mode** unchecked, the jog buttons always send raw
physical angle changes. This is intentional: changing a direction field does not
and must not change the raw test used to discover that direction.

After choosing each sign, check **Verification mode: apply direction value to
S1–S3 jogs**. The jog buttons then represent model `−` and model `+`, and the GUI
multiplies the requested jog by the selected direction. The status line shows
both the model jog and the physical command that was sent.

### Servo 1 / base

With verification mode off, use **+ jog** and view the arm from above.

- Counter-clockwise means Base direction = `+1`.
- Clockwise means Base direction = `-1`.

### Servo 2 / shoulder

S2 starts at its current upper software limit of 112.5 degrees, so use
**- jog first** with verification mode off.

- If the negative jog lowers the upper arm, Shoulder direction = `+1`.
- If the negative jog raises the upper arm, Shoulder direction = `-1`.

Then use **+ jog** once to return to the original position.

### Servo 3 / elbow

Use **+ jog** with verification mode off while watching the angle between the
upper arm and forearm.

- If the forearm bends upward relative to the upper arm, Elbow direction = `+1`.
- If it bends downward, Elbow direction = `-1`.

Use **- jog** once to return.

### Servo 4 / wrist roll

Use the two jog buttons to verify wiring and safe rotation. Stage 1 IK holds S4
at the entered physical angle, so its direction sign does not affect X/Y/Z yet.

Pressing the joystick switch on D7 or clicking **HOLD** cancels the current
interpolated motion. Neither removes servo torque; use the external power switch
for an emergency stop.

## Zero-pose and IK convention

The current zero pose is the photographed straight pose:

- Base faces forward.
- The shoulder-to-elbow link is horizontal and outward.
- The elbow-to-wrist link is collinear with the upper arm.

The conversion is:

`physical servo angle = zero angle + direction * model joint angle`

After recording all three signs, save calibration. X points forward, Y points
sideways, and Z is height above the base plate. The target is the centre of the
Servo 4 wrist-roll axis.

## Manual servo motion

Open the **Manual + automatic** tab. Each S1–S4 slider sends throttled,
synchronized commands while it is moved. Slider changes override automatic
motion immediately and restart the 15-second idle timer.

- **Send manual pose** sends the displayed four physical angles together.
- **Copy current IK target** copies the calculated IK servo targets into the
  sliders without moving.
- Continuous slider commands use a short 400 ms trajectory for smooth tracking.

Manual sliders command physical servo angles, so they do not use the IK
direction signs.

## Clickable 2D workspace

The planar map displays legal wrist positions as a light-green region. It uses
the selected elbow solution, geometry, zero angles, direction signs, and servo
software limits.

Click or drag through green positions. The GUI sets:

- `X` to the clicked radial distance from the base axis.
- `Y` to `0`, so Servo 1 stays facing forward and does not spin.
- `Z` to the clicked height above the base plate.

When connected and **Arm area is clear** is checked, a legal click sends the IK
target immediately. Holding the left mouse button and dragging streams legal
targets at up to 10 updates per second. Clicking uses a 700 ms synchronized
trajectory; dragging uses 450 ms trajectories. Illegal positions are rejected
and the most recent valid target is retained.

Uncheck **Arm area is clear**, click **HOLD**, or press D7 to stop streamed motion.

## Automatic motion after 15 seconds AFK

In the **Manual + automatic** tab, enable **wide automatic motion after 15
seconds AFK**. After 15 seconds without a manual-slider or map command, the GUI
runs an eight-pose coordinated sweep around the most recently displayed physical
servo pose. At 100% sweep size, the intended joint spans are approximately 80°
for S1, 50° for S2, 80° for S3, and 90° for S4. The entire pattern shifts inward
when necessary so it remains five degrees away from each software endpoint.

Choose a **Sweep size** of 25%, 50%, 75%, or 100%. Choose **Slow**, **Flow**, or
**Brisk** pacing. The default Flow pace adjusts each transition time according
to how far the busiest joint must travel, so all four servos start and finish
together without the old long pause between poses.

- Every automatic waypoint stays inside the four software angle limits.
- A manual slider, map click/drag, IK command, direction jog, or calibration-pose
  command overrides automatic motion and restarts the timer.
- Unchecking **Arm area is clear** stops active automatic motion.
- Clicking **HOLD** or pressing D7 stops motion and disables automatic mode. Use
  the checkbox to enable it again.
- Automatic mode requires the GUI to remain open and connected.

The software angle limits do not guarantee mechanical clearance: there is no
collision, cable-twist, or self-intersection model. Begin at 25% with Flow pace,
keep a hand near the external servo-power switch, and inspect the full path before
trying 50–100%.

## Faster synchronized motion

The controller sends all four servo targets through one shared motion timeline,
so every moving joint starts together and reaches its target together. The
firmware uses a quintic acceleration curve with zero commanded velocity and
acceleration at both ends. Servo commands are refreshed together every 20 ms.
The updated sketch also commands servo pulse widths directly, removing the old
roughly 1.5-degree command stair steps while retaining the same endpoints. Upload
the included sketch again to get this firmware-side smoothness improvement.

Use the GUI presets as a starting point:

- **Fast 2 s** for tested, nearby points.
- **Smooth 3 s** for normal movement.
- **Slow 6 s** for first-time or larger movements.

These are synchronized commands, but ordinary hobby servos have no position
feedback to the Uno. Load, supply voltage, and differences between servos can
still make their physical motion lag slightly.

## Chicken-head stabilization limitation

A chicken-head effect in the side plane requires a wrist-pitch joint that can
counter-rotate by approximately `-(shoulder + elbow)` while the arm moves. In
this mechanism Servo 4 is wrist roll: its axis runs along the forearm and cannot
change the tool's up/down pitch. Reintroduce one differential wrist servo, or
add another perpendicular wrist-pitch servo, before enabling physical
chicken-head stabilization. Applying the compensation to the current S4 would
only roll the wrist and could twist attached wiring.

## Current limitations

- No collision detection or encoder feedback.
- S4 orientation is held; it is not solved by Stage 1 position IK.
- The preview is a side view and does not draw base rotation in 3D.
- Measurements and printed-part flex will limit positional accuracy.
