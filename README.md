# Srcful ZAP Firmware

This repository contains the firmware for the Srcful ZAP, an ESP32-C3 based device designed to read data from smart meters via the P1 port and securely transmit it to the Srcful backend.

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
*   **Over-the-Air (OTA) Updates:** Supports secure firmware updates over WiFi (under development).
*   **Hardware Interaction:** Manages LED status indication and button input (e.g., for WiFi reset or reboot).
*   **Robust Operation:** Uses FreeRTOS tasks for managing concurrent operations (WiFi, data reading, backend communication, server).
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
    git clone https://github.com/your-github-username/srcful-zap-firmware.git
    cd srcful-zap-firmware
    ```
    *(Replace `your-github-username` with the actual repository location)*

2.  **Build the firmware:**
    ```bash
    pio run
    ```
    This command compiles the code and builds the firmware binary.

3.  **Upload the firmware:**
    Connect the ESP32-C3 device via USB and run:
    ```bash
    pio run -t upload
    ```
    PlatformIO will automatically detect the port and upload the firmware.

## Configuration

*   **WiFi Credentials:** Configured primarily via the BLE setup process when the device first boots or after a WiFi reset.
*   **Backend Endpoints:** The URLs for the Srcful API ([API_URL](http://_vscodecontentref_/1)) and data ingestion ([DATA_URL](http://_vscodecontentref_/2)) are defined in [config.cpp](http://_vscodecontentref_/3).
*   **Build-time Options:** Certain features can be controlled via build flags in [platformio.ini](http://_vscodecontentref_/4) (e.g., `-DUSE_BLE_SETUP`).

## Usage

1.  **Flashing:** Build and upload the firmware to the ESP32-C3 device as described above.