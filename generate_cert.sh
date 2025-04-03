#!/bin/bash

# Generate self-signed certificate in the current directory
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"

echo "Generated key.pem and cert.pem in current directory" 