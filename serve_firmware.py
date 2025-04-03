from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl
import os

# Get the current directory
current_dir = os.path.dirname(os.path.abspath(__file__))

# Get paths for certificates and firmware
cert_path = os.path.join(current_dir, 'cert.pem')
key_path = os.path.join(current_dir, 'key.pem')
firmware_dir = os.path.join(current_dir, '.pio', 'build', 'esp32-c3')

# Change to firmware directory
os.chdir(firmware_dir)

# Create HTTPS server
httpd = HTTPServer(('0.0.0.0', 8001), SimpleHTTPRequestHandler)

# Create SSL context
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(cert_path, key_path)

# Wrap the socket
httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print("Serving firmware at https://localhost:8001")
print("Firmware file should be in: " + firmware_dir)
print("Using certificates from: " + current_dir)
httpd.serve_forever() 