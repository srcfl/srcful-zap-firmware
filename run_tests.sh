#!/bin/bash
# Script to monitor test output

# Now run the tests
echo "Starting tests..."
pio test -e esp32-c3-test -vvv

# Keep the monitor running after tests complete
echo "Tests completed. Monitor is still running to capture output."
echo "Press Ctrl+C to exit when done."

# Wait for user to press Ctrl+C
wait $MONITOR_PID 