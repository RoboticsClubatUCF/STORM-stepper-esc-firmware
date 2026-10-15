# My STORM Firmware Deliverables

Personal roadmap for contributing to the STORM stepper ESC firmware, from basic to advanced.

- **Core plan:** Sep 28 → Dec 1, 2026 (Levels 1–5)
- **Stretch plan:** Dec 2026 → late March 2027 (Level 6)

## Review before doing Level 1-5

- [x] Configure environment to run C-code in WSL2
- [x] Talk to the maintainer before week 2: share this plan, confirm priorities and PR size
- [ ] Only edit generated files inside `/* USER CODE BEGIN */ ... END` blocks -> Optional
- [x] Small, focused PRs, each one tested and green in CI
- [x] Keep an engineering log (what broke, what I learned, numbers I measured)

~~- [ ] One rest day per week; lighter weeks around exams~~

~~- [ ] Code freeze 2–3 weeks before competition (bug fixes only)~~

---

## Level 1: Setup and first PRs (Week 1 & 2: Sep 28 – Oct 1st & Oct 5th - Oct 9th)
### Src/main.c deliverables to do
 - [ ] TODO: Implement inverse kinematics
 - [ ] TODO: Implement mutexes
 - [ ] TODO: add semaphores
 - [ ] TODO: mess with timers (start & stop) for RTOS
 - [ ] TODO: Add queues
 - [ ] TODO: Explain what SPI1 & SPI3 are used for
 - [ ] TODO: Explain what the PTD is used for
 - [ ] TODO: Explain where is the FDCAN is used in the systme.
 - [ ] TODO: Establish message-comms protocol between the microcontroller and the other motors, steppers, encoders, and acutuators.
 - [ ] TODO: Draw a diagram of the main system architecture
 - [ ] TODO: Explain how each component of this code correlates to the data-schematic

### Other recommended todos to do in Src/main.c
- [ ] Install CMake, ARM GNU toolchain and STM32CubeMX; firmware builds locally
- [ ] Read `Core/Src/main.c` top to bottom; can explain the boot sequence from memory
- [ ] Open `esc-firmware.ioc` in CubeMX, regenerate, and see what changes
- [ ] Order or borrow hardware (see [Hardware](#hardware))
- [ ] **PR:** README typo fixes plus a "USER CODE blocks" contributor section
- [ ] **PR:** host unit-test target (Unity) wired into CMake so CI's `ctest` runs real tests
- [ ] Personal: register-only LED blink (Nucleo, or Renode)

**Done when:** CI runs at least 1 real test.

## Level 2: Fix the existing C (Week 2: Oct 5 – 11)

- [ ] **PR:** datagram pack/parse using shifts, not struct casts, with endianness tests
- [ ] **PR:** fix `spi_status_union` to 1-bit `uint8_t` fields; correct datagram length (5 bytes)
- [ ] **PR:** make task `params` static, check `osThreadNew` return values, fix `printf` formats
- [ ] **PR:** CI hygiene: `-Wextra`, `clang-format` check, RAM/flash size report, trimmed build matrix
- [ ] Firmware boots in Renode with GDB attached (`simulator.resc`, port 3333)
- [ ] Personal: register-level UART driver so `printf` output works

**Done when:** 10+ tests pass in CI; I can step through `main()` in GDB.

## Level 3: Drivers and bring-up (Weeks 3–5: Oct 12 – Nov 1)

- [ ] **PR:** `tmc5160.c/.h` module behind an `xfer` function-pointer interface, tested against a fake chip
- [ ] **PR:** SPI RX DMA + chip-select GPIOs in CubeMX; DMA completion signalled to the task (notification/semaphore)
- [ ] Hardware bring-up: read TMC5160 `GSTAT`/`IOIN` on the Nucleo
- [ ] Capture SPI on the logic analyzer; verify SPI mode and clock speed against datasheets
- [ ] **PR:** `as5047p.c/.h` encoder driver with parity check, read in its own task
- [ ] **PR:** HardFault handler that logs registers and reports the crash reason on next boot
- [ ] **PR:** UART debug console (`reg read`, `reg write`, `enc`, `motor en`)
- [ ] First open-loop motion using the TMC5160 ramp generator

**Done when:** a video of the motor moving to commanded positions, with the encoder angle printing.

## Level 4: Integration and safety (Weeks 6–7: Nov 2 – 15)

- [ ] **Doc:** CAN protocol spec v0.1 (IDs, command/telemetry frames, units, fault behavior), reviewed by firmware + ROS2 teams
- [ ] **PR:** DBC file for the protocol; generated C packing plus Python decoding (cantools)
- [ ] **PR:** FDCAN real bit timing, filters, and send/receive tasks
- [ ] Python tool (python-can) that controls the motor from my laptop
- [ ] **PR:** board state machine: INIT / IDLE / ENABLED / FAULT / ESTOP
- [ ] **PR:** safety: CAN command timeout triggers safe stop; speed, acceleration and position limits; driver fault handling
- [ ] **PR:** stall/step-loss detection (encoder vs. commanded) reported over CAN
- [ ] **PR:** timestamps in telemetry frames (for CV / sensor fusion)
- [ ] Stretch pulled in: **fuzz the CAN parser** (libFuzzer + AddressSanitizer)

**Done when:** unplugging CAN stops the motor within the spec'd timeout, and a forced stall is detected.

## Level 5: Closed-loop and proof (Weeks 8–9: Nov 16 – Dec 1)

- [ ] **PR:** closed-loop PID position control on one axis, mode selectable over CAN
- [ ] Python dashboard: live plot of commanded vs. actual position, tunable gains
- [ ] Measure loop rate, command latency, and position error (open-loop vs. closed-loop)
- [ ] Stretch: minimal ROS2 node publishing `/joint_states` from CAN telemetry
- [ ] **Doc:** architecture overview + how to contribute + 5–10 good-first-issues
- [ ] Portfolio write-up: diagram, logic analyzer captures, plots, video, lessons learned
- [ ] Resume bullets with real measured numbers; demo to the team

**Done when:** a new member can build, test and contribute using only my docs.

### Dec 1 scorecard

- [ ] 15–25 merged PRs
- [ ] Test suite in CI with known coverage
- [ ] Motor controlled over CAN, open- and closed-loop, with safety stop
- [ ] Protocol spec + DBC shared by firmware and ROS2
- [ ] Measured numbers + video + write-up
- [ ] Newcomer docs and issues

---

## Level 6: Advanced stretch goals (Dec → late March)

| When | Goal | Shows |
|---|---|---|
| Dec 2 – ~Dec 15 (finals, light) | [ ] SEGGER SystemView tracing | Real-time analysis, RTOS scheduling |
| Winter break | [ ] StallGuard2 sensorless homing, verified against the encoder | Calibration, datasheet depth |
| Winter break → Feb 8 | [ ] S-curve trajectories + coordinated 4-axis moves | Motion planning, kinematics |
| Jan – Feb 8 | [ ] Renode + Robot Framework tests in CI | Test infrastructure, emulation |
| Feb 9 – Mar 15 | [ ] CAN bootloader with CRC + A/B fallback | Boot process, flash layout, fail-safe design |
| Mar 16 – Mar 28 | [ ] Buffer: polish, measurements, write-ups, handoff | |

**Cut order if behind:**
1. Renode peripheral models
2. Bootloader A/B fallback
3. Coordinated multi-axis motion
4. The bootloader entirely

Always keep fuzzing, homing and SystemView.

**Checkpoints:** honest review on **Dec 1** and **Feb 8**. Update the resume at Dec 1 and mid-February.

---

## Hardware

Prices are rough estimates; check current listings. Ask the team first; they may lend or reimburse.

### Tier 0: $0 (Levels 1–2)
- [ ] Laptop + toolchain + Renode

### Tier 1: Starter, ~$75–110 (Level 3)
- [ ] NUCLEO-G474RE (~$20–25)
- [ ] TMC5160 SPI driver module (~$15–30)
- [ ] NEMA17 stepper (~$10–15)
- [ ] 24 V, 3–5 A enclosed power supply (~$15–25)
- [ ] 100 µF+ / 35 V+ electrolytic capacitor (~$1)
- [ ] Breadboard + jumper wires (~$10)
- [ ] 8-channel USB logic analyzer + PulseView (~$10–15)

### Tier 2: Full basic deliverables, ~$130–200 total (Levels 4–5)
- [ ] AS5047P adapter board (~$15–25)
- [ ] Diametric magnet, 6–8 mm (~$2–5)
- [ ] 3.3 V CAN transceiver module, e.g., SN65HVD230 (~$3–5); the Nucleo has none
- [ ] USB-CAN adapter, e.g., CANable 2.0 (~$15–40)

### Tier 3: Optional, later
- [ ] Multimeter (~$20–30)
- [ ] Bench power supply with current limit (~$50–80)
- [ ] J-Link EDU Mini for SystemView (~$60)

**Safety:**
- Never plug or unplug the motor while the driver is powered.
- Set motor current in software before enabling the driver.
- Use 3.3 V logic everywhere.

---

## Framework: breaking into a new codebase

1. **Map it:** purpose, directory layout, generated vs. vendored vs. hand-written code
2. **Build and run it:** locally, plus any simulator or tests
3. **Trace the entry point:** from boot / `main()` to the core loop
4. **Find the rules:** CI, code generators, conventions
5. **Read the recent history:** `git log`, what maintainers are working toward
6. **List bugs and gaps** found while reading → first-PR candidates
7. **Ladder contributions:** docs/tests → fixes → modules → architecture
8. **Ask the maintainer** before building anything big

## Skills checklist

- [ ] C for hardware: fixed-width types, bitwise ops, bitfields/unions, endianness, `volatile`, pointer lifetime
- [ ] STM32 HAL + CubeMX workflow; boot process, linker script, memory map
- [ ] SPI, DMA, interrupts, NVIC priorities
- [ ] FreeRTOS / CMSIS-RTOS v2: tasks, priorities, stacks, notifications, queues, ISR-safe calls
- [ ] CAN / FDCAN: bit timing, IDs, filters, SocketCAN, python-can
- [ ] Datasheet reading: TMC5160, AS5047P, STM32G4 reference manual (RM0440)
- [ ] Testing: Unity, fakes, fuzzing, coverage, Renode
- [ ] Debugging: GDB, logic analyzer, fault registers, tracing
- [ ] Control: PID, trajectories, calibration, system identification
- [ ] Robotics stack: ROS2, `ros2_control`, `/joint_states`, CV/ML interface needs (timestamps, velocity mode, limits)
