# Srcful ZAP Firmware - Desktop Testing Environment

This directory contains a desktop testing environment for the Srcful ZAP firmware, enabling development and testing without physical hardware.

## Overview

The desktop testing environment allows you to run and test core ZAP firmware functionality on a standard computer. This significantly speeds up development by eliminating hardware flashing cycles and providing better debugging capabilities.

Key benefits:
- Test code changes instantly without hardware
- Use powerful desktop debugging tools
- Develop and contribute without physical devices
- Run automated tests through VS Code
- Validate protocol parsers and data processing

## Project Structure

```
test_desktop/
├── mock/               # Hardware interface mocks
│   ├── Arduino.h       # Arduino API compatibility layer
│   ├── HTTPClient.h    # Mock HTTP client
│   └── ...             # Other mock components
├── data/               # Tests for data processing components
├── json_light/         # Tests for JSON functionality
├── backend/            # Tests for backend services
├── ams/                # AMS/HAN protocol parsers and tests
├── frames.h            # Test meter data frames
├── main.cpp            # Test runner
└── ...                 # Other test files
```

## Building and Running Tests

### Using VS Code Tasks

The easiest way to build and run tests is using the provided VS Code tasks:

1. Open the project in Visual Studio Code
2. Press `Ctrl+Shift+B` or run the "Build Simple Test" task from the Terminal → Run Task... menu
3. The test binary will be built in the `build` directory
4. Run the tests with `./build/simple_test`

The VS Code build task is configured as follows:

```json
{
    "label": "Build Simple Test",
    "type": "shell",
    "command": "g++",
    "args": [
        "-std=c++11",
        "-g",
        "-I${workspaceFolder}/../src",
        "-I${workspaceFolder}/../include",
        "-I${workspaceFolder}/mock",
        "${workspaceFolder}/mock/Arduino.cpp",
        "${workspaceFolder}/mock/HTTPClient.cpp",
        "${workspaceFolder}/mock/crypto.cpp",
        "${workspaceFolder}/main.cpp",
        "${workspaceFolder}/ams/HdlcParser.cpp",
        "-o",
        "${workspaceFolder}/build/simple_test"
    ],
    "group": {
        "kind": "build",
        "isDefault": true
    },
    "presentation": {
        "reveal": "always"
    },
    "problemMatcher": "$gcc"
}
```

### Building Manually

You can also build the tests manually:

```bash
cd test_desktop
g++ -std=c++11 -g -I../src -I../include -I./mock \
  mock/Arduino.cpp mock/HTTPClient.cpp mock/crypto.cpp \
  main.cpp ams/HdlcParser.cpp \
  -o build/simple_test
```

## Mock Implementation

The desktop testing environment uses several mock components to simulate Arduino and ESP32 hardware:

- **Arduino.h**: Provides basic Arduino functions and types
- **String**: Implementation of Arduino's String class
- **WiFiClient**: Simulated network connectivity
- **HTTPClient**: Mock HTTP client for testing API requests
- **ESP32**: Mock ESP32-specific functions

## Test Components

The desktop environment includes several test components:

- **circular_buffer_test**: Tests for the circular buffer implementation
- **frame_detector_test**: Validation of smart meter frame detection
- **ascii_decoder_test**: Tests for ASCII protocol parser
- **HdlcParser**: Tests for HDLC/DLMS protocol parsing
- **json_light_test**: Validation of JSON generation and parsing
- **graphql_test**: Tests for GraphQL API integration
- **zap_str_test**: String handling utilities tests
- **debug_test**: Debugging utilities tests

## Development Workflow

1. **Develop core functionality** in the main `src` directory
2. **Write tests** in the appropriate test file
3. **Build and run tests** using VS Code tasks or command line
4. **Debug and fix** any issues found
5. **Deploy to hardware** once tests pass

## Adding New Tests

To add new tests:

1. Create a test file in the appropriate subdirectory
2. Write test functions and a `run()` function to execute them
3. Include your test file in `main.cpp`
4. Add your test suite's `run()` function to `main()`

Example:

```cpp
namespace my_test_suite {
    int test_specific_feature() {
        // Test implementation
        assert(expected_result == actual_result);
        return 0;
    }

    void run() {
        std::cout << "=== Running My Test Suite ===" << std::endl;
        test_specific_feature();
        std::cout << "My Test Suite passed!" << std::endl;
    }
}
```

## Debugging

To debug tests in VS Code:

1. Set breakpoints in your code
2. Build the debug version using the "Build Simple Test" task
3. Launch the debugger using an appropriate launch configuration
4. Step through code and inspect variables as needed