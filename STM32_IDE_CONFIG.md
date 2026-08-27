# STM32CubeIDE configuration

What the project needs from the IDE, and where each thing actually lives.

Some of it **is** in git (`.cproject` carries the include paths and the pre-build
step, so a clone gets them). Some of it is **not** — the C++ standard and the
editor settings live in the workspace, and the `.ioc` section is reverted every
time CubeMX regenerates.

Each section says which it is.

---

## 0. Submodules — clone them first

**In git.** Two, and the build fails without both:

| Path | Repo | Branch | Holds |
| --- | --- | --- | --- |
| `2026/Nova/Avionics_Common` | `Avionics_Common` | `ERC2026` | `device_ids.h`, `packets.h`, `SerialProtocol.h`, `BoardProfile.h`, `BoardUtils.h`, `gen_av_packets.sh` |
| `2026/Nova/ERC_SE_CustomMessages` | `ERC_SE_CustomMessages` | `MVP_June_HDS_and_NAV` | the `.msg` files |

```bash
git clone --recurse-submodules git@github.com:EPFLXplore/Avionics_SW.git
# or, in a clone that already exists:
git submodule update --init --recursive
```

**They must stay siblings under `2026/Nova/`.** `gen_av_packets.sh` lives in
`Avionics_Common` and reads the `.msg` files from
`../ERC_SE_CustomMessages/msg/avionics` — that relative path is its default.
Move either one and the pre-build step (§3) stops finding its input.

The split is deliberate: the `.msg` files are ROS interface definitions and the
ROS build owns them, so they stay where they are; everything the firmware and
the RPi both compile against lives in `Avionics_Common`. `Avionics_ROS` carries
the same submodule at `2026/docker/src/avionics_common`, pinned to the same
commit, so neither side can drift onto a private copy of the wire format.

**A submodule ref is a pointer.** Committing here records "Avionics_Common at
`<sha>`". Push the parent without pushing the submodule and a teammate gets a
commit pointing at something that exists only on your disk. Order: commit inside
the submodule → push it → then `git add` the path here → commit → push.

---

## 1. C++ standard: `gnu++17`

**Project Properties → C/C++ Build → Settings → Tool Settings → MCU G++ Compiler
→ General → Language standard → `gnu++17`**

Do it for **every build configuration** (Debug *and* Release), or the one you
did not touch will fail on the first `inline constexpr` it meets.

### Why

`System/Config/` relies on two C++17 features:

- **`inline constexpr`** for `PROFILES`, `CONN_PADS`, `PWM_PIN_CONFIG` and friends. At
  C++14 these are internal-linkage, so every translation unit that includes
  `BoardProfile.h` gets its own copy of the tables. Eight files do. Verified in
  the map: three objects emitted a 48-byte `PROFILES` and the linker kept one.
- **Designated initializers** (`.device = ...`, `.clk = pin::PC8`). These are a
  GNU extension before C++17, which is why the field order in an initializer
  must still match the declaration order — GCC rejects a reordering.

### Caveat — this one is NOT in git

Verified: `gnu++17` appears in **no** project file. Not `.cproject`, not
`.project`, not `.settings/`. The only place it exists is the generated
`Debug/**/subdir.mk`, which is tracked — so a command-line `make` works from a
fresh clone, and the moment the IDE regenerates those makefiles the flag reverts
to the toolchain default `gnu++14`.

So: set it in the UI on a new workspace, and re-check it after anything that
regenerates the build files. Editing `subdir.mk` by hand fixes today's build and
nothing after it.

### Checking it took

```bash
grep -rho "std=gnu++[0-9]*" Debug --include=subdir.mk | sort -u
# want: std=gnu++17
```

---

## 2. Include paths

**Project Properties → C/C++ General → Paths and Symbols → Includes → GNU C++**

**In git** (`.cproject`), so a clone has them. You need this page when you add a
source directory, or when a header stops resolving after a submodule moves.

```
${workspace_loc:/${ProjName}/System/Threads/Inc}
${workspace_loc:/${ProjName}/System/Core/Inc}
${workspace_loc:/${ProjName}/System/Config/Inc}
${workspace_loc:/${ProjName}/System/Comms/Inc}
${workspace_loc:/${ProjName}/Avionics_Common/include}
```

**GNU C++ only.** The generated C (`main.c`, `usbd_cdc_if.c`, …) reaches the C++
side through `Bridge.h`, which sits in `System/Core/Inc` and is the one header
both languages include.

`ERC_SE_CustomMessages/include` is deliberately **absent**. `device_ids.h` and
`packets.h` used to live there and now live in `Avionics_Common`; leaving the old
directory on the path would let a stale copy win the `#include` silently.

---

## 3. Pre-build step: `packets.h` is generated

**Project Properties → C/C++ Build → Settings → Build Steps → Pre-build steps**

**In git** (`.cproject`):

```
bash "${ProjDirPath}/Avionics_Common/gen_av_packets.sh"
```

It reads the `.msg` files from `ERC_SE_CustomMessages` and writes
`Avionics_Common/include/packets.h` before every compile, so the packed wire
structs cannot drift from the message definitions. **Do not edit `packets.h`** —
it is overwritten. Adding a wire packet means adding the `.msg` and an entry in
the script's `IDS` table; nothing else.

It rewrites the file only when the content changes, so a no-op run does not bump
the mtime and force a full rebuild.

Because it writes *into a submodule*, a `.msg` change leaves `Avionics_Common`
dirty. That regenerated `packets.h` has to be committed there, not here.

---

## 4. Editor folding

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
  `BoardUtils.h` is included from the *bottom* of `BoardProfile.h`, why
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

## 5. CubeMX regeneration — the `.ioc`, pins and peripherals

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
which is why `Pins.h` used to carry a hand-written `extern TIM_HandleTypeDef
htim1/htim2/htim15;` block.

**Already on** — `Pins.h` now just does `#include "tim.h"`. Leave it on: turning
it off brings that hand-written block back.

### Pads owned by the slot table, not by the `.ioc`

Every driver configures its own pads, so CubeMX's GPIO setup for them is
overwritten moments later. The `.ioc` should keep those pads **quiet** rather
than try to configure them — they were switched to open-drain for this reason.

The pads the firmware claims for itself are listed in `CONN_PADS` and `PWM_PADS`
in `System/Config/Inc/Pins.h` (they moved there with the rest of the pad
vocabulary; `BoardProfile.h` now lives in `Avionics_Common` and says only what is
*plugged into* each slot, never which pad it is). The `.ioc` still owns everything else:
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
grep -o 'prebuildStep="[^"]*"' 2026/Nova/.cproject               # want gen_av_packets.sh
git submodule status                                             # want both, no leading -
```

A leading `-` in `git submodule status` means the submodule is not initialised —
`packets.h` will not regenerate and half the headers will not resolve.
