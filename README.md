# Srcful ZAP Firmware

This repository contains the firmware for the Srcful ZAP, an ESP32-C3 based device designed to read data from smart meters via the P1 port and securely transmit it to the Srcful backend.

**NOTE this firmware is under heavy development and should be considered an Alpha vervison**

## Features

*   **P1 Smart Meter Reading:** Connects to and reads data telegrams from smart meters using the P1 port (DSMR standard).
*   **Data Decoding:** Supports decoding of both ASCII and DLMS (binary) P1 telegrams.
*   **Secure Data Transmission:** Formats meter data into JWTs, signs them using ECDSA (secp256k1), and transmits them securely over HTTPS to the Srcful data endpoint.
*   **WiFi Connectivity:** Connects to a local WiFi network (STA mode) to communicate with the backend.
*   **Configuration:**
    *   **BLE Setup:** Allows initial WiFi configuration via Bluetooth Low Energy (NimBLE).
    *   **Web Interface:** Provides a local web server for status information, diagnostics, and potentially configuration fallback.
*   **Backend Communication:**
    *   Sends device status and receives configuration updates via GraphQL over HTTPS.
    *   Utilizes WebSockets (GraphQL Subscriptions) for real-time communication with the backend (e.g., receiving commands or configuration changes).
*   **Over-the-Air (OTA) Updates:** Supports firmware updates over WiFi (under development).
*   **Hardware Interaction:** Manages LED status indication and button input (e.g., for WiFi reset or reboot).
*   **Robust Operation:** Uses FreeRTOS tasks for managing concurrent operations (WiFi, data reading, backend communication, local server).
*   **PlatformIO Build System:** Developed using the PlatformIO IDE framework for ESP32 development.

## Hardware

The firmware is designed for an ESP32-C3 based board equipped with:

*   A connection to the P1 port of a compatible smart meter.
*   WiFi and Bluetooth capabilities.
*   An LED for status indication.
*   A button for user interaction.
*   Based on the [P1 Dongle Pro+](https://smart-stuff.nl/product/p1-dongle-watermeter/)

## Building the Firmware

### Prerequisites

*   [PlatformIO Core](https://platformio.org/install/cli) installed.
*   Git command-line tools.

### Steps

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/srcful/srcful-zap-firmware.git # Replace if necessary
    cd srcful-zap-firmware
    ```

2.  **Build the firmware:**
    ```bash
    pio run -e esp32-c3
    ```
    This command compiles the code and builds the firmware binary for the ESP32-C3 environment.

3.  **Upload the firmware:**
    Connect the ESP32-C3 device via USB and run:
    ```bash
    pio run -e esp32-c3 -t upload
    ```
    PlatformIO will automatically detect the port and upload the firmware. Use `-t upload -t monitor` to also open the serial monitor.

## Configuration

*   **WiFi Credentials:** Configured primarily via the BLE setup process when the device first boots or after a WiFi reset (holding the button for >5 seconds).
*   **Backend Endpoints:** The URLs for the Srcful API ([API_URL](src/config.cpp)) and data ingestion ([DATA_URL](src/config.cpp)) are defined in `src/config.cpp`.
*   **Build-time Options:** Certain features can be controlled via build flags in `platformio.ini` (e.g., `-DUSE_BLE_SETUP`) or configuration options in `sdkconfig.defaults`.

## Usage

1.  **Flashing:** Build and upload the firmware to the ESP32-C3 device as described above.
2.  **Initial Setup:** On first boot or after a reset, the device enters BLE configuration mode. Use a compatible app (like the Srcful mobile app) to connect and provide WiFi credentials.
3.  **Operation:** Once connected to WiFi, the device will start reading P1 data, connecting to the backend, and transmitting data. The LED indicates the current status (e.g., WiFi connection state).
4.  **Button:**
    *   Hold > 2 seconds: Trigger a device reboot.
    *   Hold > 5 seconds: Clear WiFi credentials and trigger a reboot into BLE configuration mode.

## Testing

This project employs two primary methods for testing:

### 1. On-Device Integration Tests

*   **Framework:** Utilizes PlatformIO's built-in [Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html) engine for running tests directly on the ESP32-C3 hardware.
*   **Purpose:** These tests focus on integration aspects, verifying the interaction between different firmware modules and hardware peripherals (e.g., WiFi connection, basic P1 reading, LED control).
*   **Location:** Test cases are located within the `test` directory of the main project.
*   **Execution:**
    Connect the ESP32-C3 device via USB and run:
    ```bash
    # Run tests on the hardware target
    pio test -e esp32-c3
    ```
    PlatformIO compiles the tests, uploads them to the device, runs them, and reports the results via the serial connection.

### 2. Desktop Unit Tests (`test_desktop`)

*   **Framework:** A separate VSCode C++ project located in the `test_desktop` directory, configured to run tests on the host machine (native platform).
*   **Purpose:** These tests focus on unit testing individual components (classes, functions) in isolation, particularly logic that doesn't directly depend on ESP32 hardware specifics (e.g., data parsing, JWT formatting). This allows for faster development cycles and easier debugging.
*   **Location:** The test project and its source files are within the `test_desktop` directory. Shared code from the main firmware (`src`) can be included for testing.
*   See the corresponding README for further instructions

## Contributing

Contributions are welcome! Please refer to the contribution guidelines (not available yet) or open an issue to discuss proposed changes.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.