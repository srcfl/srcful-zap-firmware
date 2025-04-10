import requests
import json
import socket

# Get local IP address
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

# Replace with your ESP32's IP address
ESP32_IP = "192.168.1.66"  # Change this to your ESP32's IP

# Get local IP and construct firmware URL
LOCAL_IP = get_local_ip()
FIRMWARE_URL = f"https://{LOCAL_IP}:8001/firmware.bin"

print(f"Using local IP: {LOCAL_IP}")
print(f"Firmware URL: {FIRMWARE_URL}")

# Create the OTA update request
payload = {
    "url": FIRMWARE_URL,
    "version": "1.0.3"  # Increment this for each test
}

try:
    # Send the request
    response = requests.post(
        f"http://{ESP32_IP}/api/ota/update",
        json=payload,
        verify=False,  # Disable SSL verification for testing
        timeout=30     # 30 second timeout
    )

    # Print the response
    print(f"Status Code: {response.status_code}")
    print(f"Response: {response.text}")

except requests.exceptions.RequestException as e:
    print(f"Error: {e}") 