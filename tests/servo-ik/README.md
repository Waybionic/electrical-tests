# Servo inverse-kinematics tests

This directory preserves the Uno R4 servo-control and inverse-kinematics
prototype developed between August 22 and September 5, 2026. The files were
recovered from the original local Codex project and imported into this
repository later; the historical commit dates record the saved development
milestones, not the creation date of this combined repository.

## Contents

- [`arduino-servo-control`](arduino-servo-control) contains the desktop servo
  GUI, direct four-servo controller, six-servo automatic/manual motion demo,
  and the later IK calibration prototype.
- [`tests/test_wide_auto.py`](tests/test_wide_auto.py) exercises the IK GUI's
  automatic sweep generation and duration limits without opening a serial port
  or moving hardware.

The IK prototype includes a saved calibration snapshot. Treat those dimensions,
zero positions, directions, and limits as test data for the original arm—not as
safe defaults for a different mechanism. Support the arm, verify every joint
direction at low speed, and keep an emergency power cutoff available.
