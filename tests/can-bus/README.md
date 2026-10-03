# Arduino CAN demo tests

This folder reconstructs the concrete Arduino sketches and design notes from
the shared ChatGPT conversation **Arduino CAN Demo**. The work was discussed
and tested on September 26, 2026, then imported into this repository later.
The historical commit date records the conversation/test date; it does not
mean this combined repository already existed then.

## Hardware used in the conversation

- Two Arduino Uno R4 boards
- Two DAOKI MCP2515 + TJA1050 CAN modules with 8 MHz crystals
- CANH connected to CANH and CANL connected to CANL
- MCP2515 SPI wiring on each R4:
  - VCC to 5 V
  - GND to GND
  - CS to D10
  - SI/MOSI to D11
  - SO/MISO to D12
  - SCK to D13

With power off, the completed bus should measure approximately 60 ohms
between CANH and CANL when both 120-ohm end terminators are installed.

## Recovered tests

| Folder | Purpose |
|---|---|
| `mcp2515_init_aa` | Minimal Uno R4 initialization diagnostic using `AA_MCP2515` |
| `mcp2515_init_autowp` | Slower 1 MHz SPI diagnostic using the autowp `mcp2515` library |
| `button_led_transmitter` / `button_led_receiver` | Send a D2 pushbutton state over CAN ID `0x100` and mirror it on the receiving board's LED |
| `led_blink_transmitter` / `led_blink_receiver` | Periodically send ON/OFF frames and verify two-node communication |
| `servo_sequence_master` / `servo_sequence_node2` | Alternate two D9 servos across the CAN bus |
| `FIVE_NODE_EMULATOR_NOTES.md` | Design for emulating five MKS SERVO42D addresses with the two-node bench hardware |
| `mks-j1-bringup` | Confirmed ID 1 MKS enable and speed-mode bring-up from October 3, 2026 |
| `mks-three-joint-console-experimental` | Clearly labeled experimental terminal console for J1/J2/J3 |

The MCP_CAN_lib sketches assume **MCP_CAN_lib by Cory J. Fowler**, 500 kbps,
and an 8 MHz MCP2515 oscillator. Confirm the crystal marking before use.

## Status note

These files preserve the sketches and architecture discussed in the chat.
The conversation included successful two-node communication and additional
troubleshooting, but not every reconstructed sketch should be treated as a
production-qualified motor-control implementation. Verify wiring, termination,
power, and emergency-stop behavior before connecting real arm hardware.

