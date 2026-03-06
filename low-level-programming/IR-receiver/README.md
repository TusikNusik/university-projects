# STM32 SIRC Infrared Remote & RGB PWM Controller

This project provides bare-metal firmware for an STM32 microcontroller to receive and decode infrared remote signals using the **SIRC (Sony Infrared Remote Control)** protocol. 

The primary purpose of the application is to control lighting (an RGB LED and an additional status LED) — handling both simple on/off toggling and smooth brightness adjustments via hardware PWM. Additionally, the system reports its state over a UART interface using non-blocking DMA transfers and a custom-built circular buffer.



## Key Features

* **12-bit SIRC Decoding:** Hardware-assisted measurement of IR pulse durations using External Interrupts (EXTI) on both rising and falling edges, paired with a precision Timer.
* **Hardware PWM:** Smooth brightness control for RGB colors using Timer 3 output channels (resolution 0-1000).
* **Asynchronous UART (DMA):** Sends event logs (e.g., `RED ACTIVATED`, command/address data) without blocking the main program execution.
* **Software Debouncing:** Eliminates the multiple-click effect (e.g., when holding a button on the remote) using a dedicated hardware timer (TIM2).

## Hardware Connections (Pinout)

The pin configuration is set up for standard STM32 ports. Connect your components as follows:

| Component | STM32 Pin | Function / Mode |
| :--- | :--- | :--- |
| **IR Receiver** (e.g., TSOP) | `PA14` | Input (EXTI, Interrupt on Rising/Falling edge) |
| **Red LED (RGB)** | `PA6` | Output (TIM3 Channel 1, Alternate Function) |
| **Green LED (RGB)** | `PA7` | Output (TIM3 Channel 2, Alternate Function) |
| **Blue LED (RGB)** | `PB0` | Output (TIM3 Channel 3, Alternate Function) |
| **Extra Green LED** | `PA5` | Output (Standard GPIO Push-Pull) |
| **USART2 TX** | `PA2` | Log transmission (to USB-UART converter, 9600 baud) |
| **USART2 RX** | `PA3` | *Reserved (configured and ready for use)* |

## Remote Control Mapping

The remote control buttons are mapped to specific lighting control actions:

* **`1`** – Toggle Red LED.
* **`2`** – Toggle Blue LED.
* **`4`** – Toggle Green LED (RGB).
* **`3`** – Toggle the extra Green LED (PA5).
* **`VOL +` / `VOL -`** – Increase / Decrease Red color brightness.
* **`CH +` / `CH -`** – Increase / Decrease Green color brightness.
* **`UP` / `DOWN`** – Increase / Decrease Blue color brightness.

*(Each brightness adjustment modifies the PWM duty cycle by 1% within the 10-989 range).*

## Program Architecture

The code relies on an event-driven, bare-metal architecture (no RTOS):

1. **`TIM4` (IR Timer):** Ticks at 100kHz (1 tick = 10µs). Used within the `EXTI15_10_IRQHandler` to measure the duration of low and high states, distinguishing between a start bit (2400µs), logical one (1200µs), and logical zero (600µs).
2. **`TIM3` (PWM Timer):** Configured with an Auto-Reload Register (ARR) of 999, generating the PWM signal on 3 channels (`CCR1`, `CCR2`, `CCR3`) for the RGB LED.
3. **`TIM2` (Debounce Timer):** Ticks every 1ms. Prevents command spamming by ignoring repeated identical signals within a 250ms time window.
4. **Circular Buffer & DMA:** String messages are pushed into a ring buffer using the `push_back()` function. The `DMA1_Stream6` peripheral then automatically feeds this data into the USART data register, completely offloading the CPU.