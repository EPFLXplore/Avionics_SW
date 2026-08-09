# STM32CubeIDE configuration

Settings this project needs that **are not stored in git**. They live in the IDE
workspace or in CubeMX-generated files, so they are lost on a fresh clone, on a
new workspace, and — for the last section — every time the `.ioc` is regenerated.

Reapply them after any of those.

---

## 1. C++ standard: `gnu++17`

**Project Properties → C/C++ Build → Settings → Tool Settings → MCU G++ Compiler
→ General → Language standard → `gnu++17`**

Do it for **every build configuration** (Debug *and* Release), or the one you
did not touch will fail on the first `inline constexpr` it meets.

### Why

`System/Config/` relies on two C++17 features:

- **`inline constexpr`** for `PROFILES`, `CONN_PADS`, `PWM_MUX` and friends. At
  C++14 these are internal-linkage, so every translation unit that includes
  `BoardProfile.h` gets its own copy of the tables. Eight files do. Verified in
  the map: three objects emitted a 48-byte `PROFILES` and the linker kept one.
- **Designated initializers** (`.device = ...`, `.clk = pin::PC8`). These are a
  GNU extension before C++17, which is why the field order in an initializer
  must still match the declaration order — GCC rejects a reordering.

### Caveat

The standard is **not stored in `.cproject`** unless you set it through the UI
above. The toolchain default is `gnu++14`, and that default is what gets baked
into the generated `Debug/*/subdir.mk`. Editing those makefiles by hand works
for a command-line `make`, but the setting reverts the moment the IDE
regenerates them. Set it in the UI so it persists.

### Checking it took

```bash
grep -rho "std=gnu++[0-9]*" Debug --include=subdir.mk | sort -u
# want: std=gnu++17
```

---

## 2. Editor folding

**Window → Preferences → C/C++ → Editor → Folding**

- Tick **Enable folding**
- Under **Initially fold these region types**, tick **Comments**
- Leave **Header Comments** *un*ticked
- Optionally tick **Inactive Preprocessor Branches** (the HAL is full of
  `#ifdef HAL_x_MODULE_ENABLED`)

### Why that split

The two categories are different in this codebase:

- **Header Comments** — the block at the top of each file. This is the
  orientation: why `BoardProfile.h` holds choices and not consequences, why
  `BoardChecks.h` is included from the *bottom* of `BoardProfile.h`, why
  `Pins.h` is the only file that turns an enum into a HAL call. Worth having
  open.
- **Comments** — everything else. Per-item detail (datasheet register layouts,
  timing derivations, the reasoning behind a magic constant). Worth having
  collapsed until you are editing that item.

`System/Core/Src/ADS1114.cpp` is the clearest case: with comments expanded it
reads as more prose than code.

### Shortcuts

| Action | Keys |
| --- | --- |
| Collapse all | `Ctrl` + `Numpad /` |
| Expand all | `Ctrl` + `Numpad *` |
| Collapse / expand current | `Ctrl` + `Numpad -` / `Numpad +` |

Rebind under **Preferences → General → Keys**, search "folding".

---

## 3. CubeMX regeneration — things that get wiped

Regenerating from the `.ioc` reverts hand edits to generated files. These have
bitten more than once.

### I2C3 must be enabled in the `.ioc`

The pH meter's ADS1114 talks I2C3 on **PC8/PC9**. If the `.ioc` does not declare
the peripheral, regeneration:

1. comments out `HAL_I2C_MODULE_ENABLED` in `Core/Inc/stm32g4xx_hal_conf.h`, and
2. deletes `stm32g4xx_hal_i2c.h` / `.c` (and `_ex`) from `Drivers/`.

The build then fails with a cascade of `'I2C_HandleTypeDef' does not name a
type`, which does not obviously point at the cause.

**Fix:** enable I2C3 in CubeMX and assign PC8/PC9 to it. Board 0 bit-bangs an
HX711 on those same pads, and that still works — `HX711::begin()` reclaims them
as GPIO, the same way every driver here configures its own pads.

### Recommended: generate peripheral init as `.c`/`.h` pairs

**Project Manager → Code Generator → "Generate peripheral initialization as a
pair of .c/.h files"**

CubeMX then emits `tim.h`, `i2c.h` etc. with the handle `extern`s. Without it,
every peripheral lives in `main.c` and nothing outside it can name `htim15` —
which is why `Pins.h` currently carries a hand-written block:

```cpp
extern "C" {
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim15;
}
```

Turning the option on lets those three lines be deleted.

### Pads owned by the slot table, not by the `.ioc`

Every driver configures its own pads, so CubeMX's GPIO setup for them is
overwritten moments later. The `.ioc` should keep those pads **quiet** rather
than try to configure them — they were switched to open-drain for this reason.

The pads the firmware claims for itself are listed in `CONN_PADS` and `PWM_PADS`
in `System/Config/Inc/BoardProfile.h`. The `.ioc` still owns everything else:
USB `PA11`/`PA12`, the timers' base config, and any peripheral no driver
self-configures.

### Unused peripherals

`SPI3`, `TIM5` and `ADC3` are initialised in `main.c` but referenced nowhere in
`System/`. `SPI3` is the notable one: it owns **PB4/PB5**, the board-id straps,
so `Board_MasterId()` has to reclaim them from AF6 before it can read them.
Dropping SPI3 from the `.ioc` would remove that step.

---

## Quick verification after a regen

```bash
grep -n "HAL_I2C_MODULE_ENABLED" Core/Inc/stm32g4xx_hal_conf.h   # want it uncommented
ls Drivers/STM32G4xx_HAL_Driver/Src/ | grep i2c                  # want 2 files
grep -rho "std=gnu++[0-9]*" Debug --include=subdir.mk | sort -u  # want gnu++17
```
