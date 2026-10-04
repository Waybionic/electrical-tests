# Original October 3 MKS CAN console

This is the complete three-motor sketch recovered from the original **Wiring
And Code Setup** chat. The `.ino` is preserved verbatim rather than rewritten
into the safer word-command interface.

## What was observed on hardware

This exact sketch was uploaded and running when the Serial Monitor repeatedly
reported:

```text
MKS RX id:0x1 dlc:3 remote:0 extended:0 data:0xF3 0x1 0xF5
```

That confirms that the UNO R4, 8 MHz MCP2515, 500 kbps CAN configuration, and
MKS controller at ID 1 were communicating. A simpler ID 1 test also moved the
motor with `F6 01 40 02 3A` after the controller was changed to `SR_vFOC`.

It does **not** prove that J2 and J3 had unique IDs or that every menu command
was individually exercised. All observed replies used CAN ID `0x1`.

## Original behavior

- Calls `enableAll()` during startup.
- Uses speed `200` and acceleration `20` for the `1`–`6` and `a` commands.
- Uses speed `300`, acceleration `20`, and 1000 pulses for `r`.
- Uses the original single-character command menu.

Because startup automatically enables IDs 1, 2, and 3, use this historical
version only with unloaded motors and an accessible hardware power cutoff.
For normal bench work, prefer the cleaned variant in
[`../mks-three-joint-console-experimental`](../mks-three-joint-console-experimental),
which sends nothing automatically at startup and uses readable commands.
