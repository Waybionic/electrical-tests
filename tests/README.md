# Electrical test archive

This branch collects the WayBionic stepper and controller test programs with
their original Git histories and timestamps.

## Imported test lines

| Test line | Original history | Contents |
| --- | --- | --- |
| `l298n/` | Aug 12–19, 2025 | Early Arduino/L298N stepper experiments |
| `stepper-motors/` | Sep 6, 2025–Feb 7, 2026 | Multi-stepper, controller, TCP/UDP, limits, and motion experiments |
| `joystick-steppers/` | Jan 17–24, 2026 | Joystick-driven top-stepper tests |
| `servo-ik/` | Aug 22–Sep 5, 2026 | Uno R4 servo GUI, motion demo, IK calibration controller, and offline sweep tests |
| `can-bus/` | Sep 26, 2026 | Reconstructed Arduino MCP2515, two-node, and servo CAN demonstrations |
| `mark4/` | Source repository created Sep 5, 2026 | No tracked test files were present to import |

Every populated directory was imported from its original repository without
changing the recorded author or committer dates. Hard-coded network names and
passwords in historical Arduino test files were replaced with
`CHANGE_ME_SSID` and `CHANGE_ME_PASSWORD` before this public archive was made.

