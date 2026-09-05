# Arduino Uno R4 WiFi USB Servo Controller

This beginner project controls four 270-degree hobby servos from desktop sliders. The automatic/manual arm demo expands the system to six servos by adding a two-servo differential controlled by a third joystick. The Uno communicates through its normal USB port.

The project also includes `arm_motion_demo/arm_motion_demo.ino`, a slow automatic demonstration with manual control from three joysticks. Moving any joystick takes control immediately. After ten seconds with all sticks centered, the automatic demonstration restarts. Joystick 1's SW button toggles automatic-motion pause.

For gradual inverse-kinematics development, `ik_prototype/` contains a separate Stage 1 controller and desktop calibration GUI. It accepts approximate link lengths, previews reachable X/Y/Z targets, checks joint limits, and only moves after an explicit safety confirmation.

## What you need

- Arduino Uno R4 WiFi and a data-capable USB-C cable
- Up to six hobby servos
- Arduino IDE
- Python 3 with Tkinter (included with standard Windows Python installations)
- A regulated external 5 V supply sized for the servos is strongly recommended

## Wiring

| Servo | Signal wire | Power wire | Ground wire |
|---|---:|---|---|
| 1 | Uno D3 | External +5 V | External GND |
| 2 | Uno D5 | External +5 V | External GND |
| 3 | Uno D6 | External +5 V | External GND |
| 4 | Uno D9 | External +5 V | External GND |
| 5 | Uno D10 | External +5 V | External GND |
| 6 | Uno D11 | External +5 V | External GND |

Connect **external-supply GND to Arduino GND** so the signal has a common reference. Servo colors vary, but signal is often orange/yellow/white, power is red, and ground is brown/black.

> Do not power several servos from the Uno's 5 V pin. Servo current spikes can reset or damage the board. Never connect an external supply's +5 V to the Uno 5 V pin unless you fully understand the power arrangement.

## 1. Upload the Arduino sketch

1. Open `arduino_servo_controller/arduino_servo_controller.ino` in Arduino IDE.
2. Choose **Tools > Board > Arduino UNO R4 Boards > Arduino UNO R4 WiFi**.
3. Choose the Uno under **Tools > Port**.
4. Click **Upload**.
5. Close Arduino IDE's Serial Monitor before using the GUI; only one program can use the serial port at a time.

The sketch and GUI both use **115200 baud**. On startup, all servos move to approximately 90° of physical travel.

### Automatic motion demo with three joysticks

To run the arm demonstration with manual override, upload `arm_motion_demo/arm_motion_demo.ino`. It moves between automatic poses over four seconds with a short pause at each pose. Change `MOVE_TIME_MS` to adjust the automatic speed; a larger number is slower.

Wire the joysticks as follows. Joystick 1's switch uses the Uno's internal pull-up resistor, so no external resistor is required.

| Joystick connection | Uno R4 WiFi | Controls |
|---|---|---|
| Joystick 1 VRx/X | A0 | Joint 1 |
| Joystick 1 VRy/Y | A1 | Joint 2 |
| Joystick 2 VRx/X | A2 | Joint 3 |
| Joystick 2 VRy/Y | A3 | Joint 4 |
| Joystick 3 VRx/X | A4 | Differential axis A |
| Joystick 3 VRy/Y | A5 | Differential axis B |
| All joystick VCC/+ | 5V | Joystick power |
| All joystick GND/- | GND | Common ground |
| Joystick 1 SW | D7 | Pause/resume automatic motion |
| Joystick 2 SW | Not connected | Unused |
| Joystick 3 SW | Not connected | Unused |

Keep all three sticks centered while switching on or resetting the Uno. The program measures their center positions during its first half-second. Full stick deflection commands about 45 physical degrees per second. Change `MAX_MANUAL_SPEED` to adjust that rate. If an axis moves backward, change its corresponding value in `JOYSTICK_DIRECTION` from `1.0` to `-1.0`, or vice versa.

Joystick 3 uses differential mixing: `Servo 5 = center + A + B` and `Servo 6 = center + A - B`. The program limits the combined axes so the two outputs remain away from their endpoints. If a differential servo is mechanically reversed, change its entry in `DIFFERENTIAL_SERVO_DIRECTION` from `1.0` to `-1.0`.

Servos 5 and 6 also move throughout the automatic demonstration. Their automatic physical-angle pairs progress approximately through `135°/135°`, `125°/65°`, `205°/155°`, `120°/210°`, and `65°/135°`, with smooth easing between each pair.

Press Joystick 1's SW button once to pause automatic motion; the built-in LED turns on and the current pose is held. The joysticks continue to provide manual control. The ten-second AFK timer will not restart automatic movement while paused. Press SW again to turn off the LED, smoothly return to neutral, and restart the demo.

Keep clear of the arm when powering it for the first time. Although the demo stays inside the software limits, the controller cannot detect mechanical collisions. Be ready to switch off the external servo supply if the mechanism approaches an obstruction or hard stop.

## Servo angle limits

The requested limits were converted from a 180° scale to the servos' physical 270° scale by multiplying by 1.5.

| Servo | Original limit | Physical GUI limit |
|---|---:|---:|
| 1 | 0–180 | 0–270° |
| 2 | 0–75 | 0–112.5° |
| 3 | 60–180 | 90–270° |
| 4 | 0–180 | 0–270° |

The GUI sends physical angles. The Arduino converts them to the Servo library's logical 0–180 command scale and rejects commands outside each servo's limit.

After changing either program, upload the Arduino sketch again before running the updated GUI. The GUI limits rapid slider updates and drains incoming serial data so extended testing does not fill the serial buffers.

## 2. Install and run the desktop GUI

Open PowerShell or Command Prompt in this project folder, then run:

```powershell
py -m pip install -r requirements.txt
py servo_gui.py
```

If `py` is unavailable, use `python` instead:

```powershell
python -m pip install -r requirements.txt
python servo_gui.py
```

In the app:

1. Select the Arduino's port (often `COM3`, `COM4`, etc.).
2. Click **Connect** and wait about two seconds for the Uno to restart.
3. Move each slider within its labeled physical-angle range.

## Serial command format

The GUI sends one ASCII line per movement:

```text
servoNumber,angle
```

For example, `3,135` requests a physical angle of 135° from Servo 3. Servo numbers are 1–4, and the allowed physical angle depends on the servo table above.

## Adjusting the project

- To change pins, edit `SERVO_PINS` in the Arduino sketch and the displayed pin tuple `(3, 5, 6, 9)` in `servo_gui.py`.
- To change the servo count, update `SERVO_COUNT` and both pin lists.
- If a servo buzzes or hits its mechanical stop near an endpoint, avoid that range and tighten its software limit. Actual travel and pulse-width requirements vary by servo model.

## Troubleshooting

- **No ports shown:** use a data USB cable, install the board's USB driver if required, then click **Refresh**.
- **Access denied / port busy:** close Arduino Serial Monitor and any other serial program.
- **Servos twitch or reset the Uno:** use a stronger external 5 V supply and confirm all grounds are connected together.
- **Slider moves but servo does not:** confirm the signal pin, servo power, common ground, selected port, and 115200 baud setting.
- **Movement stops after extended slider testing:** make sure both the updated Arduino sketch and updated GUI are being used. Close any older GUI instances before starting the new one.
