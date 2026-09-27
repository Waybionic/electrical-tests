# Five-address MKS emulator concept

The two-node bench setup was intended to approximate the eventual WayBionic
base controller plus five MKS SERVO42D CAN nodes.

```text
Base Uno R4
    |
DAOKI MCP2515/TJA1050
    |
CANH / CANL
    |
DAOKI MCP2515/TJA1050
    |
Emulator Uno R4
    |
Servo output on D9
```

The receiving R4 can emulate five logical motor addresses even though it is a
single physical CAN node:

```cpp
switch (motorID) {
  case 1:
    servo1.write(angle);
    break;
  case 2:
    servo2.write(angle);
    break;
  case 3:
    servo3.write(angle);
    break;
  case 4:
    servo4.write(angle);
    break;
  case 5:
    servo5.write(angle);
    break;
}
```

With one available servo, the same actuator can visibly represent each logical
address in sequence while the receiver prints the selected virtual MKS node.
The proposed sequence was:

```text
0.0 s -> Motor 1, 90 degrees
1.5 s -> Motor 2, 45 degrees
3.0 s -> Motor 3
4.5 s -> Motor 4
6.0 s -> Motor 5
```

The next intended step was an acknowledgement frame from the emulator back to
the base after each command. The chat established the architecture but did not
produce a complete five-address implementation, so this file intentionally
records the design without inventing untested source code.

## Simplifications

1. Five physical MKS nodes become one physical emulator with five software IDs.
2. Closed-loop stepper motors become one or more hobby servos.
3. The real Makerbase protocol becomes a small demonstration protocol.

The physical CAN bus, frame transport, node addressing, acknowledgement,
timeouts, and error-handling behavior can still be exercised with this setup.
