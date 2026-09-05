# WayBionic electrical tests

Public archive of electrical integration experiments for the WayBionic arm.
It brings the team's Servo Mk2, stepper, joystick, TCP/UDP, and early L298N
work into one repository while retaining the dates recorded in the original
Git histories.

> [!NOTE]
> This repository was created on October 8, 2026. Older commit dates are
> preserved imports from the original WayBionic test repositories; they do not
> imply that this combined archive existed on those earlier dates.

## Test archive

| Area | Recorded development period | Location |
| --- | --- | --- |
| L298N stepper experiments | Aug 12–19, 2025 | [`tests/l298n`](tests/l298n) |
| Servo Mk2 controller | Sep 7, 2025 | [`tests/servo-mk2`](tests/servo-mk2) |
| Multi-stepper and controller variants | Sep 6, 2025–Feb 7, 2026 | [`tests/stepper-motors`](tests/stepper-motors) |
| Joystick-driven steppers | Jan 17–24, 2026 | [`tests/joystick-steppers`](tests/joystick-steppers) |
| Servo GUI and inverse-kinematics prototype | Aug 22–Sep 5, 2026 | [`tests/servo-ik`](tests/servo-ik) |
| Arduino CAN demo tests | Sep 26, 2026 | [`tests/can-bus`](tests/can-bus) |
| Mark 4 placeholder | Source repo created Sep 5, 2026; no commits | [`tests/mark4`](tests/mark4) |

The StepperMotors archive includes the main line plus the controller,
stepper-mover, playground, joystick, TCP, UDP, and delay-reduction variants
that were present in the source repository.

## Branches

- [`main`](../../tree/main) — complete combined archive
- [`servo-mk2`](../../tree/servo-mk2) — isolated Servo Mk2 history
- [`stepper-can-mk4`](../../tree/stepper-can-mk4) — isolated stepper, joystick,
  L298N, CAN, and Mark 4 work

## Public-history note

The imported commits retain their original authors, messages, author dates,
and committer dates. Before publication, hard-coded network names and passwords
in historical Arduino test files were replaced throughout the imported history
with `CHANGE_ME_SSID` and `CHANGE_ME_PASSWORD`.
