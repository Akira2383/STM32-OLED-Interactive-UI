# STM32 Custom OLED UI & State Machine

This project is an interactive display system based on the **STM32F103C8T6** microcontroller, driving a 0.96-inch I2C OLED (SSD1306). 

Unlike basic text-only demos, this project implements a complete **Finite State Machine (FSM)** architecture to handle custom bitmap rendering, boot animations, an interactive terminal menu, a Snake game, and a real-time sensor visualization page.

##  UI Showcase

The system boots up with a custom 128x64 Rhodes Island logo, featuring a normal/inverted blinking animation. After the boot sequence, it transitions smoothly into an "Arknights" terminal-style menu with a moving cursor. From the menu you can launch the Snake game (Terminal), or open the live sensor page (Base).

![UI Showcase](https://cdn.jsdelivr.net/gh/Akira2383/STM32-OLED-Interactive-UI@main/images/ui_showcase.gif)

##  Key Features

*   **Custom Bitmap Rendering:** Implemented a low-level `ShowPicture` / `UI_DrawBMP_Fast` function to parse 1024-byte hex arrays into OLED GRAM pixels, displaying a full-screen 128x64 image.
*   **Finite State Machine (FSM):** Decoupled the UI logic into distinct states (`STATE_BOOT`, `STATE_MENU`, and `STATE_SENSOR_PAGE`), ensuring smooth transitions and preventing rendering conflicts.
*   **Interactive Terminal Menu:** Five menu entries with a looping cursor and intentional timing, simulating a real embedded product interface.
*   **Snake Game:** A self-contained game mode on a 32x16 grid (4x4 pixel cells, 150ms steps) with wall/self collision detection, a win condition, and a blinking end screen.
*   **Live Sensor Page:** The ADC on PA0 is sampled at 1kHz into a 128-sample ring buffer. The upper half renders an auto-gaining waveform (~30ms refresh), while the lower half shows the voltage, raw ADC value, and a progress bar (~100ms refresh).
*   **Spectrum Analysis:** `spectrum.c` runs a 128-point fixed-point FFT (mean removal, Hann window, Q15 twiddles, lookup-table square root) and reduces 64 bins into 16 bars. `UI_DrawSpectrum()` is ready to use.
*   **Hardware-to-Software I2C Fallback:** Attempted to use the STM32 hardware I2C peripheral (`I2C1`). Due to the infamous "F103 I2C Bus Deadlock" hardware bug causing the system to hang, I gracefully rolled back to a stable software-simulated I2C driver. The UI logic remains completely independent of the underlying driver.
*   **Event-Driven Key Driver:** Five keys (PA1/PA2/PA3/PA5/PA6) driven by a debounce state machine, with edge-triggered events consumed through `Key_GetEvent()`.
*   **Independent Watchdog:** LSI 40kHz with a /64 prescaler and reload value 1875, giving roughly a 3-second timeout.

##  Hardware Setup

The STM32 is connected to the OLED using flying wires (no breadboard for better stability).
*   **VCC**: 3.3V
*   **GND**: GND
*   **SCL**: PB6 (Software I2C)
*   **SDA**: PB7 (Software I2C)
*   **Keys**: PA1 (Next/Down; Breakout left), PA2 (Enter; Breakout right), PA3 (Up), PA5 (Left), PA6 (Right) — internal pull-up, active low
*   **Analog Input**: PA0 (ADC1 Channel 0, 0–3.3V)

Note: PA2 is defined as `OLED_RES` in `oled.h`, but that initialization is commented out and the pin is actually used as the Enter key.

![Hardware Setup](https://cdn.jsdelivr.net/gh/Akira2383/STM32-OLED-Interactive-UI@main/images/setup.jpg)

##  Tech Stack & Architecture

*   **MCU**: STM32F103C8T6 (ARM Cortex-M3)
*   **Toolchain**: Keil MDK-ARM V5.39 (ARMCC v5.06 update 6)
*   **Version Control**: Git / GitHub Desktop
*   **Image Processing**: PCtoLCD2002 (Column-by-column C51 format)

**Architecture Note:** 
The `main.c` handles only the application logic (FSM and UI rendering). The `oled.c` acts as the hardware abstraction layer (HAL), handling the I2C communication. This decoupling allows for easy future upgrades (e.g., switching to SPI or a different screen). All rendering is buffered in `OLED_GRAM[144][8]` and flushed to the panel with `OLED_Refresh()`.

## How to Build

1. Clone this repository.
2. Open `USER/OLED.uvprojx` in Keil MDK.
3. Build the project (`F7`).
4. Flash the code to the board using DAP-Link or ST-Link (`F8`).

The build produces `OBJ/OLED.axf` and `OBJ/OLED.hex`. There is no CMake/Make/CI setup; verification is a successful uVision build followed by hardware execution.
