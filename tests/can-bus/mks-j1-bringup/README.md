# MKS J1 confirmed bring-up

This sketch preserves the smallest motor-control path confirmed on the bench on
October 3, 2026. It targets one MKS SERVO42D/SERVO57D at CAN ID 1 and never
moves the motor automatically at startup.

## Confirmed working

- Arduino UNO R4 WiFi communicating through a DAOKI MCP2515/TJA1050 module.
- The MCP2515's oscillator is marked `8.000` and is configured for 8 MHz.
- CAN bus rate is 500 kbps.
- The MKS controller is configured with `Mode = SR_vFOC`, `CanRate = 500K`,
  `CanRSP = Enable`, and `CanID = 01`.
- Command `F3 01 F5` enabled the motor and produced this acknowledgement:

  ```text
  MKS RX id:0x1 dlc:3 remote:0 extended:0 data:0xF3 0x1 0xF5
  ```

- Speed frame `F6 01 40 02 3A` caused the motor to run after `SR_vFOC` was
  enabled.
- A zero-speed `F6` frame is used for normal speed-mode stop.

The line-oriented Serial Monitor commands are `enable`, `run`, `reverse`,
`stop`, and `help`. Set the monitor to 115200 baud and a newline ending.

## Wiring

| UNO R4 WiFi | MCP2515 module |
| --- | --- |
| 5V | VCC |
| GND | GND |
| D13 | SCK |
| D12 | SO / MISO |
| D11 | SI / MOSI |
| D10 | CS |
| D2 | INT |

Connect CANH to CANH, CANL to CANL, and use appropriate common grounds and bus
termination. Configure each MKS controller individually before placing multiple
controllers on the same bus; the controllers appear to default to ID 1.

## Scope

Only the ID 1 acknowledgement and the documented speed-mode bring-up were
observed. This folder does not claim that IDs 2 and 3, relative motion, status
queries, or emergency stop were validated.
