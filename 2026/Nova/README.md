# Nova — Avionics SW

STM32G4 firmware for the Nova avionics board. Runs FreeRTOS with microROS over USB CDC.

---

## Setup

Clone with submodules:

```bash
git clone --recurse-submodules <repo-url>
# or if already cloned:
git submodule update --init
```

Open in STM32CubeIDE. The microROS static library is built automatically as a pre-build step via Docker — see below if you need to rebuild it.

---

## microROS Library

The static library lives in `micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros/`. The build script skips generation if the folder exists, so to force a rebuild:

```bash
rm -rf micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros
```

Then build from CubeIDE. Docker pulls the ROS2 build environment, compiles the library and drops the result back into `libmicroros/`.

**Requirements:** Docker must be running. The `ERC_SE_CustomMessages` submodule must be initialized (see above) — the build copies the avionics messages from it directly so no GitHub auth is needed inside the container.

---

## Custom Messages

Messages are defined in `ERC_SE_CustomMessages/msg/avionics/` (submodule, `git@github.com:EPFLXplore/ERC_SE_CustomMessages.git`). The microROS build picks up all `.msg` files from that folder automatically.

On the ROS2 agent side, source the same submodule from your workspace:

```bash
ln -s <path-to-nova>/ERC_SE_CustomMessages <ros2_ws>/src/custom_msg
colcon build --packages-select custom_msg
```

Package name is `custom_msg`. Avionics msgs are under `custom_msg/msg/avionics/`.

If you add a new `.msg` file to the submodule, add it to `extra_packages/custom_msg/CMakeLists.txt` and rebuild the library.

---

## Servo Driver

4 servos on GPIOB, all running at 50 Hz. The driver forces the correct prescaler/ARR on init regardless of CubeMX config.

| ID | Pin  | Timer     |
|----|------|-----------|
| 0  | PB15 | TIM15_CH2 |
| 1  | PB14 | TIM15_CH1 |
| 2  | PB13 | TIM1_CH1N |
| 3  | PB11 | TIM2_CH4  |

Zero position is 1500 µs (configurable per servo in `ServoConfigs.h`). Angle range is 0–180°, mapped to 500–2500 µs.

To command a servo over ROS2:

```bash
# Set servo 0 to 90 degrees
ros2 topic pub --once /servo_angle custom_msg/msg/avionics/ServoRequest "{id: 0, angle: 90, go_to_zero: false}"

# Send servo 2 to zero position
ros2 topic pub --once /servo_angle custom_msg/msg/avionics/ServoRequest "{id: 2, angle: 0, go_to_zero: true}"
```

---

## ROS2 Topics

| Topic          | Type                                    | Direction     |
|----------------|-----------------------------------------|---------------|
| `/servo_angle` | `custom_msg/msg/avionics/ServoRequest`  | agent → MCU   |
| `/mass`        | `custom_msg/msg/avionics/MassPacket`    | MCU → agent   |
| `/beat`        | `std_msgs/Float32`                      | MCU → agent   |
| `/subs`        | `std_msgs/Int32`                        | MCU → agent   |
| `/test_sub`    | `std_msgs/Int32`                        | agent → MCU   |

ROS node name: `cubemx_node`
