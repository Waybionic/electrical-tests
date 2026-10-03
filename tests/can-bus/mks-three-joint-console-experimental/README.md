# Experimental MKS three-joint console

> [!CAUTION]
> This sketch is implemented from the October 3, 2026 bench notes but has not
> been re-validated as a complete three-motor system. Test with motors unloaded
> and an accessible hardware power cutoff.

## Implemented but not yet validated

- Three intended CAN IDs: J1 = 1, J2 = 2, J3 = 3.
- Human-readable Serial Monitor commands replacing the earlier single-letter
  and numeric command labels.
- Motor enable/disable with `F3`.
- Per-joint and all-joint speed commands with `F6`.
- Normal speed-mode stop with a zero-speed `F6` frame.
- Emergency stop with `F7`.
- Relative-position demonstration with `FD`.
- Status query with `F1`.
- Printing all received CAN frames for acknowledgement checks.

The implementation uses the checksum described by the Makerbase CAN protocol:
the low byte of the motor CAN ID plus all command bytes before the checksum.

## Commands

Use 115200 baud with a newline ending.

| Command | Action |
| --- | --- |
| `enable` / `disable` | Enable or disable J1, J2, and J3 |
| `j1 cw`, `j1 ccw` | Run J1 in one direction |
| `j2 cw`, `j2 ccw` | Run J2 in one direction |
| `j3 cw`, `j3 ccw` | Run J3 in one direction |
| `all cw`, `all ccw` | Run all three motors |
| `relative` | Move each motor 1000 relative pulses |
| `stop` | Normal stop for all motors |
| `estop` | Emergency stop for all motors |
| `query` | Query IDs 1, 2, and 3 |
| `help` | Print the command list |

No motor command is sent at startup.

## Required controller setup

Configure each MKS controller individually before joining them to the shared
bus:

| Joint | CAN ID |
| --- | --- |
| J1 | `01` |
| J2 | `02` |
| J3 | `03` |

Each controller also requires `Mode = SR_vFOC`, `CanRate = 500K`, and
`CanRSP = Enable`.

The physical wiring, 8 MHz oscillator requirement, and confirmed ID 1 result
are documented in [`../mks-j1-bringup`](../mks-j1-bringup).

## Next test

1. With motors unloaded, run `query` and confirm responses from CAN IDs `0x1`,
   `0x2`, and `0x3`.
2. Enable and move one joint at a time, starting with the lowest practical
   speed.
3. Verify `stop` and the hardware power cutoff before testing `all cw`.
4. Validate `estop` and `relative` separately; do not infer those paths are
   working from the earlier `F3`/`F6` result.

## Recovery note

The Arduino IDE temporary path recorded during the session still exists, but
the file contains only the empty Arduino template. These sketches are therefore
a conservative reconstruction from the recorded command bytes and observed
responses, not a copy of a complete saved original.
