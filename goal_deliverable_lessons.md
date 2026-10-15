# Goal Deliverable Lessons: Simulator vs. Real Hardware

Companion to `DELIVERABLES.md`. That file is the *timeline*; this one is the *skills inventory*: every way the Renode sandbox (`simulator.resc` + `stm32g474.repl` + the peripheral models you write) and the real bench (NUCLEO-G474RE + TMC5160 + NEMA17 + AS5047P + CAN) can teach you the STORM firmware. The lists are deliberately exhaustive. Skip freely and come back; the point is to know what exists.

## How to use this list

- Every checked item should leave an artifact: a commit, an engineering-log entry, a capture (log, CSV, screenshot, trace) or a measured number. If it left nothing, it isn't done.
- **Part 1** is simulator only. **Part 2** is real hardware only. **Part 3** is the same exercise in both, with the difference written down. **Part 4** is the variation matrix. **Part 5** is the "I understand this firmware" outcomes.
- Tags: `(reg)` do it at register level with no HAL · `(HAL)` through the ST HAL · `(model)` needs a Renode model written or changed · `(bug)` exercises a real defect in today's `main.c`.
- Each section ends with an **Outcome** line. That is the thing you should be able to do or explain afterwards; check it only when you can do it without notes.

### Facts about the firmware as it stands (the items below refer to these)

- Clock tree: HSI 16 MHz, no PLL, AHB = APB1 = APB2 = 16 MHz (`SystemClock_Config`).
- SPI1 (TMC5160s): 8-bit, mode 0 (CPOL low, 1st edge), prescaler 2 → 8 Mbit/s, software NSS. SPI3 (AS5047P): the same but 16-bit. No chip-select pins are defined anywhere.
- DMA: only `SPI1_TX` and `SPI3_TX` channels exist in the `.ioc`; the code calls `HAL_SPI_Receive_DMA`.
- FDCAN1: classic frames, placeholder bit timing (prescaler 16, TSEG1 1, TSEG2 1 → 333 kbit/s, 3 time quanta per bit).
- FreeRTOS: 1 kHz tick, two tasks with 512 B stacks each, 3072 B heap, no stack-overflow or malloc-failed hooks, `configMAX_SYSCALL_INTERRUPT_PRIORITY` = 5, DMA IRQs at priority 5.
- `printf` goes to `__io_putchar`, which nothing implements, so no output anywhere yet.
- Debug pins are `JTAG (4 pins)`: PA13, PA14, PA15, PB3.
- Renode platform: no DMA, FDCAN or SPI-slave models (those regions are `Tag`s), the RCC is a Python "flipflop", and the SysTick (170 MHz), DWT (72 MHz) and TIM (10 MHz) frequencies in the `.repl` do not match the 16 MHz firmware clock tree.

---

## Part 1: Simulator sandbox (Renode)

### 1.0 Sandbox baseline (do these first)

- [ ] Install Renode; run `renode simulator.resc`; confirm the machine starts and the GDB server listens on port 3333
- [ ] Attach `arm-none-eabi-gdb build/Debug/esc-firmware.elf -ex "target remote :3333"`; break on `main`, step to `osKernelStart`
- [ ] Explain every line of `simulator.resc`: `logFile`, `mach create`, `LoadPlatformDescription`, `EnableProfiler`, `showAnalyzer`, `LoadELF`, `StartGdbServer`
- [ ] Explain every block of `stm32g474.repl`: what a `Tag` is, why the `flash` and `USB:RESET` tags are needed for CubeMX projects, what `ApplySVD` buys you (register names in the logs)
- [ ] Read `scripts/pydev/flipflop.py` in your Renode install and explain how the fake RCC makes `HAL_RCC_OscConfig` return `HAL_OK`
- [ ] Find out why the firmware hangs before the RTOS starts today: `HAL_FDCAN_Init` times out on the tagged FDCAN region → `Error_Handler` → `__disable_irq(); while (1)`. Prove it with `cpu0 PC` and `bt` (bug)
- [ ] Put a Renode hook on `Error_Handler` (`sysbus GetSymbolAddress "Error_Handler"` + `cpu0 AddHook`) so the sim logs loudly instead of spinning silently
- [ ] Make `printf` work: implement `__io_putchar` over LPUART1 (the same UART the Nucleo's ST-LINK virtual COM port uses) and see text in the `lpuart1` analyzer
- [ ] Discover newlib buffering: `printf("Hello")` with no `\n` never appears; fix with `\n` or `setvbuf(stdout, NULL, _IONBF, 0)` (bug)
- [ ] Write `simulator/README.md`: how to build, run, attach GDB, where logs and metrics land
- [ ] **Outcome:** a cold clone plus three commands gives a running sim with UART output and GDB attached

### 1.1 Boot, memory map, linker script and startup

- [ ] Step from the reset vector: `Reset_Handler` → `SystemInit` → `.data` copy loop → `.bss` zero loop → `__libc_init_array` → `main` (`startup_stm32g474xx.s`)
- [ ] Dump the vector table at `0x08000000` (`x/16xw`) and match each entry to a handler in `stm32g4xx_it.c`
- [ ] Confirm the initial MSP equals `_estack` from `STM32G474xx_FLASH.ld`; watch SP move during the first calls
- [ ] Read the linker script: `.isr_vector`, `.text`, `.rodata`, `.data`, `.bss`, `._user_heap_stack`; explain `_sidata`, `_sdata`, `_edata`, `_sbss`, `_ebss`
- [ ] Generate and read the `.map` file (`-Wl,-Map=`); find where `trinamicTask_attributes`, `hspi1` and `ucHeap` live
- [ ] Run `arm-none-eabi-size`, `nm --size-sort`, `objdump -h`; record flash and RAM use as a baseline number
- [ ] Watch `.data` initialisation happen: break before and after the copy loop and examine a known initialised global
- [ ] Explain the `0x00000000` boot alias (flash remapped to address 0) and why a NULL pointer *read* may not fault on an STM32
- [ ] Shrink and grow `_Min_Heap_Size` and `_Min_Stack_Size`; see the link fail or succeed
- [ ] (reg) Replace the FPU enable and VTOR setup from `SystemInit` with your own and prove it still boots
- [ ] Move the vector table (VTOR) and run a second image at an offset: the first step toward a bootloader
- [ ] Place a buffer in CCM SRAM with a linker section attribute; explain why the `.repl` only models `sram0`
- [ ] Compare `-O0`, `-Og`, `-O2`, `-Os` builds: size, stepping behaviour, inlined HAL calls
- [ ] Compare GCC and clang (`cmake/starm-clang.cmake`) binaries: size and warnings
- [ ] **Outcome:** I can draw this firmware's memory map from memory and explain every step before `main`

### 1.2 Clocks, time and simulator fidelity

- [ ] Read `SystemClock_Config` and compute every peripheral clock from HSI 16 MHz with all prescalers at /1
- [ ] Configure the PLL for 170 MHz in CubeMX, regenerate, and see which HAL calls change (flash latency, voltage scaling, APB dividers)
- [ ] Measure one `osDelay(1000)` in Renode virtual time; find that the `.repl` SysTick (170 MHz) and TIM1 (10 MHz) frequencies do not match the firmware's 16 MHz; fix the `.repl` so one virtual second is one firmware second
- [ ] Explain the HAL tick (TIM1 via `TIM1_UP_TIM16_IRQn`) versus the FreeRTOS tick (SysTick), and why CubeMX separates them when an RTOS is present
- [ ] Use `emulation RunFor "0.5"`, `pause` and `start` to run exactly N ms of firmware; use it to test timeouts deterministically
- [ ] Change `emulation SetGlobalQuantum` and `SetAdvanceImmediately`; explain virtual time versus host time
- [ ] Write a better RCC model (Python peripheral): ready flags that set after a write, a SW/SWS field that reflects the selected clock, instead of the flipflop
- [ ] Enable DWT `CYCCNT` (`DEMCR.TRCENA`, `DWT_CTRL`) and time a code section in cycles; compare with the `.repl`'s `dwt` frequency
- [ ] Count HAL timebase interrupts per virtual second with a counter and the Monitor's `watch`
- [ ] **Outcome:** I can state every clock in the system, what the sim fakes, and what one virtual second means

### 1.3 GPIO

- [ ] (HAL) Blink PA5 (`greenled` in the `.repl`) and watch the LED state change in the Monitor
- [ ] (reg) The same blink with `GPIOA->MODER`, `ODR` and `BSRR` only; diff the register writes using `sysbus LogPeripheralAccess gpioa`
- [ ] Add the five missing chip-select outputs (4 × TMC5160, 1 × AS5047P) in CubeMX and drive them idle-high before SPI init
- [ ] Switch debug from `JTAG (4 pins)` to `Serial Wire` to free PA15 and PB3; see what changes in the generated GPIO init
- [ ] Note that PA5 is also LD2 on a Nucleo-64; decide whether SPI1_SCK on PA5 is acceptable or remap it
- [ ] Add a `Miscellaneous.Button` to a pin in the `.repl`, press it from the Monitor, handle it with EXTI
- [ ] Drive a pin from the Monitor (`gpioa OnGPIO <n> true`) to emulate TMC5160 DIAG0/DIAG1 inputs
- [ ] Debounce logic unit-tested on the host, then run in the sim with scripted presses
- [ ] Toggle a "scope pin" at ISR entry and exit and log the toggle timestamps from a Renode hook (the sim version of the oscilloscope trick)
- [ ] **Outcome:** I can configure, read and write any pin without CubeMX and explain MODER, OTYPER, OSPEEDR, PUPDR and AFR

### 1.4 UART, printf, logging and the debug console

- [ ] (HAL) Blocking `HAL_UART_Transmit` printf path; (reg) the same with `USART_TDR` and `ISR.TXE`
- [ ] Interrupt-driven TX ring buffer so `printf` never blocks a task
- [ ] DMA TX for UART once a DMA model exists (see 1.6)
- [ ] RX interrupt plus a line parser; build the console from DELIVERABLES Level 3 (`reg read`, `reg write`, `enc`, `motor en`, plus `stack` and `heap`)
- [ ] Expose the UART over TCP (`emulation CreateServerSocketTerminal 3456 "term"` + `connector Connect sysbus.lpuart1 term`) and drive the console from a Python script
- [ ] Expose it as a PTY (`CreateUartPtyTerminal`) and use `minicom` or `pyserial` exactly as you would on hardware
- [ ] Log levels and module tags; a compile-time switch that strips logging from Release builds
- [ ] Prove why `printf` from an ISR is dangerous (reentrancy, stack) by doing it and watching it break
- [ ] Measure how many bytes `printf("%d", x)` pushes on the stack (`-fstack-usage`, high-water mark before and after)
- [ ] Second UART (`usart1` analyzer) as a binary telemetry stream; decode it with Python
- [ ] **Outcome:** I have a reusable non-blocking logging layer and a console I can script against

### 1.5 SPI (the heart of this firmware)

Register level and HAL:

- [ ] Run `sysbus LogPeripheralAccess spi1` while `HAL_SPI_Transmit` executes; map every write and read to CR1, CR2, SR, DR and the RM0440 SPI chapter
- [ ] (reg) Send one byte with `SPI1->CR1`, `CR2`, `DR` and poll `SR.TXE`, `RXNE`, `BSY`
- [ ] Explain mode 0 versus mode 3 (CPOL, CPHA), MSB-first, 8-bit versus 16-bit data size, `FRXTH` and the G4 SPI FIFO
- [ ] List the differences between Renode's `STM32SPI` model (F4-series flavour) and the G4 SPI register map; note what the model ignores
- [ ] Compare blocking, interrupt and DMA transfers of the same 5-byte datagram in GDB

TMC5160 protocol (write the model, then the driver):

- [ ] (model) C# `ISPIPeripheral` model `Tmc5160.cs`: 40-bit datagrams, address byte bit 7 = write, SPI_STATUS byte, 32-bit payload, MSB first
- [ ] (model) Implement the "a read's data comes back in the *next* datagram" rule and prove the driver handles it
- [ ] (model) Register map with datasheet defaults: GCONF, GSTAT (clear on write 1), IFCNT (increments on every write), IOIN (VERSION byte), CHOPCONF, IHOLD_IRUN, DRV_STATUS…; cite the datasheet section for each
- [ ] (model) Log every datagram the model receives so a Robot test can assert on it
- [ ] Driver `tmc5160.c/.h` behind an `xfer(tx, rx, len)` function pointer; unit-test it on the host with a fake, run it in the sim against the model
- [ ] Pack and unpack datagrams with shifts, not `__attribute__((packed))` bitfield casts; test endianness on the host and in the sim (bug)
- [ ] Fix the 2-byte receive (a datagram is 5 bytes) and the `%c` address print (bug)
- [ ] Read `IOIN` → VERSION, read `GSTAT` → clear it → read again, read `IFCNT` before and after a write: your three "driver is alive" checks
- [ ] Write `IHOLD_IRUN`, `CHOPCONF`, `GLOBAL_SCALER`; read back and compare (the model enforces read-only and write-only registers)
- [ ] (model) Ramp generator: XTARGET, XACTUAL, VACTUAL integrated over virtual time using VMAX and AMAX; `position_reached` flag; RAMP_STAT bits
- [ ] (model) StallGuard: `SG_RESULT` as a function of a modelled load; `sg_stop`; a DIAG pin output wired to a GPIO/EXTI line
- [ ] (model) Faults on demand: `ot`/`otpw`, `s2ga`/`s2gb`, `ola`/`olb`, `uv_cp`; the driver reacts (disable, report over CAN)
- [ ] Command an open-loop move in the sim and plot XACTUAL over time from the model's log

AS5047P protocol (SPI3, 16-bit):

- [ ] (model) `As5047p.cs`: 16-bit frames, bit 15 even parity, bit 14 R/W in commands and EF (error flag) in responses, 14-bit address or data
- [ ] (model) The "response arrives in the next frame" rule; NOP (`0x0000`) to clock out the last result
- [ ] (model) Registers: ERRFL, PROG, DIAAGC (MAGL, MAGH, COF, LF, AGC), MAG, ANGLEUNC, ANGLECOM, ZPOSM/ZPOSL, SETTINGS1/2
- [ ] Driver `as5047p.c/.h` with parity generation and checking; reject frames with EF set and read ERRFL
- [ ] Angle math: 14 bits → degrees and radians, wrap-around, delta-angle velocity, zero offset
- [ ] (model) Angle sources: constant → ramp → sine → coupled to the TMC ramp model (gear ratio, direction sign, backlash, noise)
- [ ] (model) Fault injection: flipped parity bit, EF set, MAGL/MAGH on a "lost" magnet; the driver reports each one distinctly
- [ ] Read the encoder in its own task at a fixed period; measure the achieved period in virtual time

Chip selects and bus sharing:

- [ ] Software NSS: assert and release CS around each datagram; verify the order in the access log (CS low → bytes → CS high)
- [ ] (model) Select among four TMC5160 models by GPIO chip select (Renode's `SPIMultiplexer` or a small custom one)
- [ ] Wrong CS pin → garbage or zeros; CS left low → the next transfer is corrupt; forgotten idle-high default. See each failure in the sim
- [ ] SPI1 shared by two tasks → mutex; first prove the race without it (interleaved datagrams in the model log), then fix it
- [ ] One bus-owner task with a queue of requests from other tasks; compare with the mutex design
- [ ] Decide between NSSP pulse mode and software CS and write down why
- [ ] **Outcome:** I can write and debug an SPI driver for a new chip from its datasheet, without the HAL, in the sim, before hardware exists

### 1.6 DMA

- [ ] Trace `HAL_SPI_Receive_DMA` in GDB: with `SPI_DIRECTION_2LINES` it calls `HAL_SPI_TransmitReceive_DMA`, which needs `hspi1.hdmarx`; CubeMX only generated `SPI1_TX` and `SPI3_TX`, so the handle is NULL (bug)
- [ ] Add `SPI1_RX` and `SPI3_RX` DMA channels in CubeMX; see `__HAL_LINKDMA` and `HAL_DMA_Init` appear in `stm32g4xx_hal_msp.c`
- [ ] Note that `dma1`, `dma2` and `dmamux1` are only `Tag`s in the `.repl`: DMA transfers never complete, silently
- [ ] (model) Add a channel-based STM32 DMA model plus DMAMUX (start from the closest existing Renode platform file) and get one SPI RX DMA completion interrupt
- [ ] Follow a completion end to end: DMA IRQ → `HAL_DMA_IRQHandler` → `HAL_SPI_RxCpltCallback` → task notification → task wakes
- [ ] Measure CPU time for the same transfer with polling, interrupt and DMA (DWT cycles or the profiler)
- [ ] Circular DMA for continuous encoder sampling; half- and full-transfer callbacks
- [ ] DMA priorities, byte versus half-word alignment (SPI3 is 16-bit), memory-increment settings; break each one deliberately
- [ ] Explain why `volatile` and barriers matter for DMA buffers even on a Cortex-M4 with no data cache (compiler reordering)
- [ ] **Outcome:** I can set up any DMA request end to end and explain every interrupt along the way

### 1.7 Interrupts, NVIC and faults

- [ ] Read NVIC `ISER`, `ISPR`, `IPR` in the Monitor (SVD names) and match them to the `HAL_NVIC_SetPriority` calls
- [ ] Explain `NVIC_PRIORITYGROUP_4`, four priority bits, and why `configMAX_SYSCALL_INTERRUPT_PRIORITY = 5` means "ISRs at priority 0–4 may not call FreeRTOS"
- [ ] Set a DMA IRQ to priority 4, call a `FromISR` API from it, and watch `configASSERT` fire (or chaos without it)
- [ ] Trace interrupt entry and exit in GDB: the stacked frame (xPSR, PC, LR, R12, R3–R0) and `EXC_RETURN`
- [ ] `cpu0 LogFunctionNames true` to watch handler names scroll by; spot PendSV, SVC and SysTick
- [ ] Deliberate HardFaults: NULL function pointer, unaligned access with `UNALIGN_TRP`, divide by zero with `DIV_0_TRP`, execute from a no-execute region
- [ ] Implement the HardFault handler from DELIVERABLES Level 3: dump CFSR, HFSR, MMFAR, BFAR and the stacked PC/LR, store them in no-init RAM, print on the next boot
- [ ] Enable UsageFault, BusFault and MemManage separately (`SHCSR`); show each escalating to HardFault when disabled
- [ ] EXTI from a simulated DIAG pin; measure pin edge → handler latency with DWT
- [ ] Nested interrupts and tail-chaining: two IRQs at different priorities, observe the order
- [ ] Vector table relocation to RAM and runtime handler patching
- [ ] **Outcome:** given a fault dump I can name the faulting instruction and the reason in under five minutes

### 1.8 FreeRTOS / CMSIS-RTOS v2

- [ ] Explain `.stack_size = 128 * 4` (bytes, carved from the 3072 B heap) versus `configMINIMAL_STACK_SIZE` (words)
- [ ] Enable `configCHECK_FOR_STACK_OVERFLOW = 2` and implement the hook; overflow a 512 B stack with `printf` and catch it (bug)
- [ ] Record `osThreadGetStackSpace` / `uxTaskGetStackHighWaterMark` for both tasks; size stacks from data, not guesses
- [ ] Use `-fstack-usage` plus the call graph to compute the worst-case stack of `Trinamic_SPI_Task` statically
- [ ] Enable `configUSE_MALLOC_FAILED_HOOK`; shrink `configTOTAL_HEAP_SIZE` until `osThreadNew` returns NULL; check return values (bug)
- [ ] Demonstrate the `params` lifetime bug: `main()`'s stack is reused after `osKernelStart`; watch the struct get clobbered in GDB; fix it with `static` (bug)
- [ ] Static allocation (`osThreadAttr_t.stack_mem` and `cb_mem`) versus heap_4; compare fragmentation and determinism
- [ ] Priorities: the `AboveNormal` busy loop starves `defaultTask`; prove it (no "Hello"), then fix it with blocking I/O
- [ ] Idle task, idle hook and `WFI`; see CPU idle time in the profiler
- [ ] Task notifications, binary semaphore, counting semaphore, queue and event flags: implement ISR → task signalling with each and compare latency and footprint
- [ ] Mutex with priority inheritance on the SPI bus; show priority inversion without it
- [ ] Queues: encoder task → control task → CAN task pipeline; back-pressure when full; `osMessageQueueGetSpace`
- [ ] Software timers (`osTimerNew`) for periodic telemetry and a CAN-command watchdog
- [ ] `osDelayUntil` / `vTaskDelayUntil` for a drift-free control loop; measure the drift against plain `osDelay`
- [ ] Run-time stats (`configGENERATE_RUN_TIME_STATS` with DWT) → `vTaskGetRunTimeStats` over the console
- [ ] Critical sections (`taskENTER_CRITICAL`), `__disable_irq` and mutexes: when each is right
- [ ] Deadlock on purpose (two mutexes taken in opposite order); diagnose it from GDB (`pxCurrentTCB`, task states)
- [ ] FreeRTOS-aware debugging: inspect `pxReadyTasksLists`, `pxCurrentTCB` and each TCB's `pxTopOfStack` in GDB
- [ ] Trace facility (`configUSE_TRACE_FACILITY` is already 1): `vTaskList` over UART; hand-draw the schedule from it
- [ ] Explain the CMSIS-RTOS2 → FreeRTOS mapping (`cmsis_os2.c`): what `osThreadNew` really calls
- [ ] Tickless idle in the sim: enable it, count tick interrupts per second
- [ ] **Outcome:** I can design the task/ISR/primitive layout for a new feature and justify every priority and stack size with numbers

### 1.9 CAN / FDCAN

- [ ] Understand why `HAL_FDCAN_Init` fails in the sim (tagged region); add a minimal stub so boot proceeds, and write down what the stub lies about
- [ ] Bit-timing math: tq = prescaler / f_kernel; bit time = 1 + TSEG1 + TSEG2 quanta; sample point around 87.5 %. Compute real 500 kbit/s and 1 Mbit/s settings for both the 16 MHz and 170 MHz clock trees (today's 16/1/1 is a placeholder)
- [ ] G4 FDCAN message RAM: fixed layout, filter elements, RX FIFO0/1, TX FIFO/queue, TX event FIFO; read the RM0440 chapter and label the `0x4000A400` region in the `.repl`
- [ ] (model) Check whether Renode's Bosch M_CAN model fits the G4 FDCAN (same IP core); otherwise write a stub with TX FIFO, RX FIFO0 and interrupts
- [ ] Two machines in one emulation on a `CANHub`: the ESC node plus a "controller" node (a second firmware or a scripted sender)
- [ ] Standard versus extended IDs, filters (range, dual, mask), FIFO0 versus FIFO1 routing; prove wrong IDs are rejected
- [ ] Classic versus FD frames (DLC > 8, BRS): the DELIVERABLES comparison experiment, in the sim
- [ ] Protocol v0.1 from DELIVERABLES Level 4 encoded as a DBC; generate C with cantools; round-trip test on the host and in the sim
- [ ] Command timeout → safe stop, tested deterministically with `RunFor` (no frames for N ms → state machine enters FAULT)
- [ ] Telemetry timestamps from `osKernelGetTickCount` or DWT; verify monotonicity across tick wrap-around
- [ ] If your Renode version has a SocketCAN bridge: connect the emulated bus to `vcan0` and run `candump` and python-can against the sim
- [ ] Bus-off, error counters, arbitration loss: list which of these a model can represent and which need the bench
- [ ] **Outcome:** I can configure FDCAN from the reference manual, pick timing for any clock, and test the protocol without hardware

### 1.10 Timers, PWM and the other modelled peripherals

- [ ] Use TIM2 or TIM3 (modelled) for a periodic interrupt; compare with the RTOS tick as the control-loop clock
- [ ] PWM output (STEP pulses) plus a DIR GPIO as the alternative to the TMC ramp generator; count pulses in a model
- [ ] Timer encoder-interface mode fed by a Python quadrature generator (the AS5047P's ABI output, simulated)
- [ ] Input capture to timestamp DIAG edges
- [ ] One-pulse mode and output compare for chip-select timing experiments
- [ ] ADC1 (modelled): read a "motor temperature" or "supply voltage" channel whose value you set from the Monitor; add it to telemetry
- [ ] I2C1 (modelled): talk to a simple sensor model to learn a second bus protocol and compare it with SPI
- [ ] RTC and RNG models: timestamps and a random back-off for CAN retries
- [ ] **Outcome:** I can choose between timer, RTOS and DMA for any periodic job and explain the trade-off

### 1.11 Watchdogs, resets and robustness

- [ ] Enable IWDG (modelled); stop kicking it; observe the reset and read the `RCC_CSR` reset flags (note what the fake RCC cannot tell you)
- [ ] Kick the watchdog from a health task that checks every other task checked in (task-level watchdog pattern)
- [ ] Unexpected-reset handling: no-init RAM crash record, boot counter, "last fault" reported over CAN
- [ ] Redesign `Error_Handler`: record HAL file and line (`USE_FULL_ASSERT`), log, then reset instead of spinning
- [ ] Reset in the sim with `machine Reset` and verify the crash record survives
- [ ] **Outcome:** the firmware never hangs silently; every failure leaves a readable trace

### 1.12 Toolchain, inspection and debugging technique

- [ ] GDB: hardware watchpoint on `RX_Buffer`, `finish`, `bt` from inside an ISR, `info registers`, `p/x *hspi1.Instance`, `x/5xb`, `display`
- [ ] `monitor` passthrough: run Renode Monitor commands from inside GDB
- [ ] Renode Monitor: `peripherals`, `sysbus WhatIsAt 0x40013000`, `sysbus ReadDoubleWord`, `WriteDoubleWord` to poke a register while the firmware runs
- [ ] `Save` and `Load` emulation state to reproduce a bug instantly
- [ ] `cpu0 EnableExecutionTracing` → PC trace → which functions ran, in what order; derive crude coverage from it
- [ ] `machine EnableProfiler` metrics plus Renode's `metrics_analyzer` tool: executed instructions, memory and peripheral accesses, exceptions over time
- [ ] `objdump -d` the bitfield pack/unpack code and compare it with the shift version
- [ ] Prove a missing `volatile`: an optimised polling loop that never exits at `-O2`, then fix it
- [ ] Turn on `-Wall -Wextra -Wconversion -Wshadow -Werror` for `Core/` (not `Drivers/`) and fix everything
- [ ] `clang-tidy`, `cppcheck` and GCC `-fanalyzer` over `Core/`; triage the findings
- [ ] `clang-format` config plus a CI check
- [ ] `compile_commands.json` plus clangd navigation that resolves the HAL headers
- [ ] **Outcome:** I debug from evidence (traces, registers, numbers), not from guesses

### 1.13 Build system and CI hygiene

- [ ] Add a new module (`tmc5160.c`) to `target_sources` in `CMakeLists.txt` and understand the `cmake/stm32cubemx` split between generated and user code
- [ ] Explain why the CI matrix is redundant today: `CMAKE_C_COMPILER=gcc|clang` is overridden by the toolchain file, so all six jobs build the same thing; trim it (DELIVERABLES Level 2)
- [ ] Add a host test target (no toolchain file) so `ctest` runs real tests; `Core/Tests/datagram_verification.c` is empty today
- [ ] Add a flash/RAM size report step to CI (`--print-memory-usage`) and fail on regressions past a threshold
- [ ] Regenerate from `esc-firmware.ioc` in CubeMX and diff; confirm only `USER CODE` blocks survive
- [ ] Use `CMakePresets.json` (the `starm-clang` presets) and understand how presets differ from the README's command line
- [ ] **Outcome:** CI builds, tests and measures every PR, and I can explain every line of the workflow

### 1.14 Host-side simulation (no Renode)

- [ ] Unity tests for datagram pack/unpack, parity, angle math, the state machine and the CAN parser
- [ ] A fake `xfer`/HAL layer so `tmc5160.c` and `as5047p.c` compile and run natively
- [ ] CMock/Ceedling versus hand-written fakes: decide and document
- [ ] Coverage with gcov/lcov; publish the number
- [ ] libFuzzer plus AddressSanitizer on the CAN parser and the console parser (DELIVERABLES Level 4 stretch)
- [ ] A Python plant model (stepper, load, encoder) for the PID loop; tune gains before touching the sim or hardware
- [ ] **Outcome:** every pure-logic module has host tests; firmware bugs are caught before the sim runs

### 1.15 Test automation with the simulator

- [ ] First Robot Framework test: boot, `Wait For Line On Uart "Hello"`
- [ ] Console test: `Write Line To Uart "reg read 0x04"` → expect the VERSION byte
- [ ] Model-assertion test: after `motor en`, the TMC model saw `IHOLD_IRUN` and then `CHOPCONF` writes
- [ ] Fault-injection test: the model raises `ot` → the firmware reports FAULT within N ms (`RunFor`)
- [ ] Timing test: the CAN command-timeout path
- [ ] Run Renode headless in GitHub Actions (Antmicro's Docker image); upload logs and metrics as artifacts
- [ ] Nightly job: the full Robot suite plus a profiler metrics trend
- [ ] Flaky-test discipline: deterministic tests on virtual time, never `sleep`
- [ ] **Outcome:** a PR cannot merge if the firmware stops booting or the SPI protocol regresses

### 1.16 Scaling the hardware in simulation (the "various amounts" axis)

- [ ] 1 MCU + 1 TMC5160 (baseline)
- [ ] 1 MCU + 2 TMC5160: shared SCK/MISO/MOSI, two CS lines, interleaved reads
- [ ] 1 MCU + 4 TMC5160 (the real board): per-axis driver instances, one bus task or one mutex; worst-case bus time per control period
- [ ] One of four drivers missing or unpowered: reads return all zeros or all ones; detect and report it per axis
- [ ] 1 MCU + 4 TMC + 1 AS5047P on SPI3: two buses, two tasks, real concurrency
- [ ] N encoders (future boards): what changes in the driver API when there are several
- [ ] 2 ESC boards on one CAN hub with colliding IDs → see the mess → design a per-board ID scheme
- [ ] 4 ESC boards + 1 controller node: bus-load calculation versus measured frames per second in the sim
- [ ] 2 machines with different clocks (16 MHz and 170 MHz) on the same bus
- [ ] Parameter sweep: the same Robot test at ten encoder-noise levels; plot position error against noise
- [ ] **Outcome:** I can predict what breaks when a device is added, removed or fails, because I have done it

### 1.17 Modelling as a skill

- [ ] Python memory-mapped stub: register array with read/write handlers and logging (like the RCC flipflop, but honest)
- [ ] C# peripheral: `DoubleWordRegisterCollection`, `IKnownSize`, `IGPIOReceiver`, `ISPIPeripheral`, IRQ outputs; loaded at runtime with `include @simulator/models/….cs`
- [ ] Time-aware model: a counter advancing on the machine's clock source (ramp generator, encoder motion)
- [ ] Composite model: TMC ramp → shaft angle → encoder reading, with configurable gear ratio and noise
- [ ] Model parameters settable from the Monitor and Robot (`tmc0 InjectFault "ot"`, `enc0 Angle 1234`)
- [ ] Publish the models under `simulator/models/` with a README and datasheet citations
- [ ] Decide the fidelity level of each model and write it down, especially what it does *not* check (SPI mode, timing, electrical behaviour)
- [ ] Encode hardware lessons back into the models after Part 2: for example a hook that warns if `SPI1_CR1` isn't mode 3 before a TMC datagram
- [ ] **Outcome:** given a new chip's datasheet I can have a usable model in a day

### 1.18 Reading skills the sim lets you verify immediately

- [ ] RM0440 (STM32G4 reference manual): SPI, DMA/DMAMUX, FDCAN, RCC, GPIO, NVIC/EXTI chapters, each read with the Renode access log open
- [ ] TMC5160A datasheet: SPI interface, register map, ramp generator, StallGuard2, coolStep, DIAG pins
- [ ] AS5047P datasheet: SPI interface, register map, parity, DIAAGC, zero-position programming
- [ ] Cortex-M4 generic user guide: exception model, fault registers, NVIC, SysTick, DWT
- [ ] FreeRTOS reference manual and the Cortex-M port (`port.c`): scheduler start, context switch, `FromISR` rules
- [ ] For each document, keep a one-page "what I verified in the sim" note
- [ ] **Outcome:** I read a register description and immediately know how to test my understanding

### 1.19 What the simulator cannot teach (hand-off to Part 2)

- [ ] SPI mode and edge mismatches (models consume bytes, not edges) → 2.3
- [ ] Clock-speed limits, signal integrity, ringing, crosstalk → 2.2 and 2.3
- [ ] Power sequencing, decoupling, brown-outs, current limits → 2.4
- [ ] Motor current, heat, resonance, StallGuard reality → 2.4
- [ ] Magnet placement, AGC, real angle noise → 2.5
- [ ] CAN transceivers, termination, bus-off → 2.6
- [ ] Real interrupt latency, flash wait states, actual CPU load → 2.7
- [ ] Human factors: wiring errors, ESD, hot-plugging → 2.9

---

## Part 2: Real hardware bench

### 2.0 Bench baseline

- [ ] Flash the Nucleo with STM32CubeProgrammer, then with OpenOCD (`board/st_nucleo_g4.cfg` or `target/stm32g4x.cfg`) or pyOCD; `monitor reset halt`; attach GDB exactly as in the sim
- [ ] Confirm the ST-LINK virtual COM port is wired to LPUART1 (UM2505 solder bridges); the `__io_putchar` from 1.0 prints on the real board unchanged
- [ ] Measure the 3V3 and 5V rails with a multimeter; record the Nucleo's idle current
- [ ] Learn the Nucleo-64 pinout: LD2 on PA5 (shared with SPI1_SCK here), B1 on PC13, Arduino versus Morpho headers
- [ ] Read the option bytes; understand RDP levels (level 2 is permanent; never set it on a dev board)
- [ ] **Outcome:** build → flash → UART output → GDB attached in under two minutes, and I know what every LED on the board means

### 2.1 Debug and trace on real silicon

- [ ] SWD versus JTAG pins; switch the `.ioc` to Serial Wire and confirm the debugger still connects
- [ ] SWO/ITM `printf` on PB3 (Trace Asynchronous Sw) with CubeIDE's SWV viewer or `orbuculum`; compare throughput with UART
- [ ] Hardware breakpoints are limited (Cortex-M4 FPB: 6 instruction + 2 literal comparators; DWT: 4 watchpoints); see what happens when you run out
- [ ] Live variable watch with STM32CubeMonitor
- [ ] HardFault on hardware: provoke one, read CFSR and BFAR in the debugger, compare with the sim dump
- [ ] Semihosting: try it, watch it halt when no debugger is attached, remove it
- [ ] SEGGER SystemView or Percepio Tracealyzer via J-Link RTT (DELIVERABLES Level 6); read a real schedule trace
- [ ] Debugger side effects: halting stops SysTick, a breakpoint inside an SPI transaction corrupts the frame, the watchdog resets while halted (`DBGMCU` freeze bits)
- [ ] **Outcome:** I can debug a timing-sensitive driver on hardware without the debugger changing its behaviour

### 2.2 Electrical fundamentals

- [ ] Breadboard capacitance and wire length: scope SCK at 8 MHz and at 1 MHz; see ringing and overshoot; choose a bring-up prescaler
- [ ] 3.3 V versus 5 V logic: confirm every module's IO voltage before wiring it
- [ ] Pull-ups and pull-downs: MISO idle state, CS idle-high, DIAG open-drain versus push-pull
- [ ] Decoupling: 100 µF electrolytic plus 100 nF ceramic at the TMC5160 VS pin; scope the rail during a step burst with and without them
- [ ] Grounding: one ground reference, star versus daisy-chain; motor return current kept away from logic ground
- [ ] Identify stepper coil pairs with the multimeter (a few ohms between the two wires of a coil)
- [ ] Power supply: 24 V at 3–5 A, inrush, and why a bench supply with a current limit saves chips
- [ ] Measure the actual motor current on the supply readout against the IRUN setting
- [ ] **Outcome:** I wire a new module correctly the first time and know what the scope should show

### 2.3 SPI on the wire

- [ ] Logic analyzer (PulseView) decode of one TMC datagram: CS, SCK, MOSI, MISO; annotate the bytes
- [ ] Sample-rate limits: a 24 MS/s analyzer cannot resolve 8 MHz SPI; lower the prescaler for bring-up or use a scope
- [ ] SPI mode mismatch for real: the TMC5160 expects mode 3 (CPOL = 1, CPHA = 1) and the AS5047P mode 1 (CPOL = 0, CPHA = 1); the firmware uses mode 0 for both. Observe the garbage, fix it, re-capture (bug)
- [ ] Check the maximum SCK against each datasheet's SPI timing table (AS5047P: 10 MHz; TMC5160: read its table) and the clock you measured
- [ ] CS setup/hold and the inter-frame gap (the AS5047P needs CS high for roughly 350 ns between frames); see errors when violated
- [ ] 16-bit frames on SPI3: byte order on the wire versus in memory
- [ ] Shared bus with four slaves: MISO contention when one slave fails to tri-state; probe MISO with every CS high
- [ ] Long wires near the motor driver: crosstalk from STEP and coil currents onto MISO; shorter runs and twisted pairs fix it
- [ ] Measure a full read-modify-write round trip; compare with sim virtual time
- [ ] **Outcome:** given a capture I can tell which side is wrong and why

### 2.4 TMC5160 on the bench

- [ ] Power-up order: motor supply (VS) present before expecting SPI answers (on many modules reads are zeros without it); VCC_IO at 3.3 V; never hot-plug the motor
- [ ] First reads: IOIN VERSION = 0x30, GSTAT reset flag, IFCNT increments: the same sequence as in the sim
- [ ] Set GLOBAL_SCALER, IRUN and IHOLD from the motor's rated current and the module's sense resistors; measure coil current
- [ ] Enable the driver (DRV_ENN pin, TOFF) and feel the holding torque; disable and feel it release
- [ ] spreadCycle versus stealthChop: hear and feel the difference; read TSTEP and TPWMTHRS
- [ ] Microstep resolution (MRES) and interpolation; count steps per revolution against the encoder
- [ ] Ramp generator in the real world: VSTART, A1, V1, AMAX, VMAX, DMAX, D1, VSTOP; find where the motor stalls or resonates
- [ ] Position mode to XTARGET, velocity mode, hold mode; watch RAMP_STAT and `position_reached`
- [ ] StallGuard2: sweep SGT, load the shaft by hand, plot SG_RESULT; set TCOOLTHRS; sensorless homing with `sg_stop` (DELIVERABLES Level 6)
- [ ] coolStep current scaling under a varying load
- [ ] DIAG0/DIAG1 as interrupts: stall → EXTI → safe stop
- [ ] Faults for real: over-temperature (otpw, ot) after sustained current, open load (a coil disconnected while disabled), short detection
- [ ] Thermal: touch or IR-read the driver and motor after ten minutes at IRUN; relate to the datasheet limits
- [ ] The TMC5160's own encoder interface (ENCMODE, X_ENC, ENC_DEVIATION) fed by the AS5047P's ABI output: hardware step-loss detection
- [ ] **Outcome:** I can bring up a new TMC board and tune it to move a given motor quietly and reliably

### 2.5 AS5047P on the bench

- [ ] Mount a diametric magnet; measure air gap and centring; read DIAAGC (AGC, MAGL, MAGH) and find the sweet spot
- [ ] Read ANGLECOM at standstill for a minute; compute the noise (counts RMS): your real noise model for the sim
- [ ] Rotate by hand; confirm the direction sign and 16384 counts per revolution; handle wrap-around in velocity estimation
- [ ] ANGLEUNC versus ANGLECOM (dynamic angle error compensation) at speed
- [ ] Program the zero position (ZPOSM/ZPOSL) in volatile memory; document the OTP permanent-programming trap and don't do it on a dev part
- [ ] Parity and EF errors from a loose wire; read ERRFL; harden the driver
- [ ] ABI, UVW and PWM outputs on the scope; resolution settings in SETTINGS2
- [ ] Sample-rate study: how fast SPI3 can read it before timing violations, and what rate the control loop actually needs
- [ ] Couple it to the motor: encoder angle versus TMC XACTUAL → gear ratio, offset, backlash (the real version of the sim model)
- [ ] **Outcome:** I can install, calibrate and validate an absolute encoder and quantify its error

### 2.6 CAN on the bench

- [ ] Transceiver (SN65HVD230) wiring at 3.3 V logic, two 120 Ω terminators; measure 60 Ω across CANH–CANL
- [ ] USB-CAN adapter → SocketCAN (candleLight or slcan); `candump`, `cansend`, `cangen`, `canbusload`
- [ ] Real bit timing at 500 kbit/s and 1 Mbit/s as computed in 1.9; scope one frame; check the sample point and stuff bits
- [ ] Oscillator tolerance: HSI versus an HSE crystal for CAN; see error frames when the timing is marginal
- [ ] Filters and FIFOs with real traffic; drop frames on purpose with a flooded bus
- [ ] Bus-off and recovery: short CANH to CANL, watch the error counters, implement recovery
- [ ] Missing terminator, one terminator, stub-length effects
- [ ] CAN FD with BRS on real transceivers (needs FD-capable parts): the DELIVERABLES comparison on the bench
- [ ] python-can control tool; DBC decoding with cantools; command-to-motion latency measured with a scope pin
- [ ] Unplug CAN mid-motion: safe stop within the spec'd timeout (DELIVERABLES Level 4 "done when")
- [ ] **Outcome:** I can bring up a CAN link from the transceiver to python-can and diagnose it with a scope

### 2.7 Timing, performance and measurement

- [ ] GPIO toggle plus scope: ISR latency, task wake latency, control-loop period and jitter; compare with the sim numbers
- [ ] DWT cycle counts of each driver call on hardware versus in the sim
- [ ] Flash wait states and prefetch/cache (`FLASH_ACR`) at 170 MHz; measure the difference
- [ ] CPU load from run-time stats on hardware; headroom before adding closed-loop control
- [ ] MCU power draw in run, sleep and tickless idle
- [ ] **Outcome:** I have measured, not estimated, every timing number the design depends on

### 2.8 Control and motion on hardware

- [ ] Open-loop move with the ramp generator; video plus encoder log (DELIVERABLES Level 3 "done when")
- [ ] Closed-loop PID position control on one axis; tune on the bench with the Python dashboard
- [ ] System identification: step response of motor plus load; compare with the sim plant model and update it
- [ ] Step-loss detection: encoder versus commanded; force a stall and see it reported over CAN
- [ ] S-curve trajectories and coordinated multi-axis moves (DELIVERABLES Level 6)
- [ ] Limits and safety: speed, acceleration and position limits, E-stop input, fault handling under real faults
- [ ] **Outcome:** a motor follows commanded positions with a measured error, and misbehaves safely

### 2.9 Bench practice and safety

- [ ] ESD basics; handling boards; checking polarity before power
- [ ] A current-limited bench supply as the first line of defence; fuse the 24 V rail
- [ ] A bring-up checklist you actually follow: power, IO voltage, CS idle, SPI mode, clock, first reads
- [ ] Mechanical safety: couplings, fingers, loose magnets at speed
- [ ] Label every wire; photograph every working setup
- [ ] **Outcome:** nothing on my bench has died from a wiring mistake since I started the checklist

### 2.10 System integration

- [ ] A ROS2 node publishing `/joint_states` from CAN telemetry (`ros2_socketcan`); a `ros2_control` hardware-interface sketch
- [ ] Several boards on one bus with an ID scheme; a coordinated four-axis move
- [ ] Firmware update over CAN with real flash erase/write timing and an A/B fallback (DELIVERABLES Level 6)
- [ ] EMC reality check: motors running while CAN and SPI stay error-free; log error counters for an hour
- [ ] **Outcome:** the ESC works as part of the robot, not just on the bench

---

## Part 3: Sim ↔ real crossover (do both, record the delta)

- [ ] The same ELF boots in both; compare the time to the first UART line
- [ ] `osDelay(1000)` measured in both (after fixing the `.repl` clocks)
- [ ] SPI access log (sim) versus logic-analyzer capture (hardware) of the same datagram: a byte-for-byte diff
- [ ] Datagram parsing: works in the sim, garbage on hardware → mode 3 fix → add the sim hook that would have caught it
- [ ] Encoder noise measured on hardware → injected into the sim model → PID retuned in the sim → verified on hardware
- [ ] Plant model (sim) versus step response (hardware): fit inertia and friction; keep a "model error" number
- [ ] Stack high-water marks in both (they should match); heap use in both
- [ ] HardFault dump in both for the same bug (it should match)
- [ ] CAN timeout → safe stop in both; latency numbers side by side
- [ ] ISR latency: sim (idealised) versus hardware; write down the gap
- [ ] Fault-injection parity: every fault the models can raise is reproduced once on hardware, or marked "hardware only"
- [ ] Keep `simulator/SIM_TO_REAL.md`: every surprise the bench taught that the sim didn't, and what you changed in the models afterwards
- [ ] **Outcome:** I know exactly which parts of the firmware the sim validates and which only hardware can

---

## Part 4: Variation matrix

Take any skill above and run it across these axes. Every new cell is a checkbox-worthy exercise.

| Axis | Variations |
|---|---|
| Device count | 1 TMC → 2 → 4; 0 encoders → 1 → N; 1 CAN node → 2 → 5 |
| Bus | SPI1 (8-bit, TMC) vs SPI3 (16-bit, AS5047P); shared vs dedicated |
| Transfer mode | blocking → interrupt → DMA → circular DMA |
| RTOS primitive | notification, binary/counting semaphore, queue, event flags, mutex, stream buffer |
| Clock tree | HSI 16 MHz (today) vs PLL 170 MHz; HSI vs HSE for CAN |
| Fault | none; device missing; stuck MISO; CS stuck low; parity/EF error; wrong SPI mode; over-temperature; bus-off; command timeout |
| Timing | nominal; slow bus (prescaler 256); jittery task; starved task |
| Build | -O0 / -O2 / -Os; GCC vs clang; Debug vs Release; asserts on/off |
| Where | host unit test → Renode → Nucleo on a breadboard → the STORM board |

- [ ] Fill in a copy of this table for the TMC driver and check off every cell you have actually run
- [ ] The same for the encoder driver
- [ ] The same for the CAN layer

---

## Part 5: "I understand this firmware" outcomes

- [ ] I can explain every line of `main.c` and `main.h`, including what CubeMX generated and why
- [ ] I can redraw the bus/architecture diagram (`docs/bus_overview2.png`) from memory, with pins and SPI modes
- [ ] Before calling a HAL function I can predict the register writes it will make, and verify them in the Renode log
- [ ] I can write the TMC5160 and AS5047P drivers without the HAL and without looking at my old code
- [ ] I can list the bugs the original `Trinamic_SPI_Task` had, what each would have done on hardware, and which ones the sim could catch
- [ ] I can size a task stack, a queue and the heap from measurements
- [ ] I can say which bugs only the bench will reveal, and which instrument finds each one
- [ ] I can add a new device model to the sim and a new driver to the firmware in one PR, with tests
- [ ] A new team member can run the sim, the tests and the bench from my docs alone
