#include "html.h"

// Define the HTML strings
const char WIFI_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            max-width: 500px;
            margin: 0 auto;
            padding: 20px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input, select {
            width: 100%;
            padding: 8px;
            box-sizing: border-box;
            margin-bottom: 10px;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 15px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background-color: #45a049;
        }
        .refresh-btn {
            background-color: #008CBA;
            margin-bottom: 10px;
        }
        #manual-ssid-group {
            display: none;
        }
        .qr-text {
            text-align: center;
            margin-top: 10px;
            font-size: 14px;
            color: #666;
        }
    </style>
</head>
<body>
    <h2>WiFi Setup</h2>
    <p>Configure your device to connect to your WiFi network.</p>
    
    <button class="refresh-btn" onclick="refreshNetworks()">Refresh Networks</button>
    
    <form id="wifi-form">
        <div class="form-group">
            <label for="network-select">Select Network:</label>
            <select id="network-select" onchange="handleNetworkSelect()">
                <option value="">-- Select a network --</option>
                <option value="manual">Enter manually...</option>
NETWORK_OPTIONS
            </select>
        </div>
        
        <div id="manual-ssid-group" class="form-group">
            <label for="manual-ssid">Network Name:</label>
            <input type="text" id="manual-ssid" placeholder="Enter network name">
        </div>
        
        <div class="form-group">
            <label for="password">Password:</label>
            <input type="password" id="password" placeholder="Enter network password">
        </div>
        
        <button type="button" onclick="submitForm()">Connect</button>
    </form>

    <div class="qr-text">Tap to login on the same device</div>

    <script>
        function handleNetworkSelect() {
            const select = document.getElementById('network-select');
            const manualGroup = document.getElementById('manual-ssid-group');
            manualGroup.style.display = select.value === 'manual' ? 'block' : 'none';
        }

        function refreshNetworks() {
            fetch('/api/wifi/scan')
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        setTimeout(() => {
                            window.location.reload();
                        }, 2000);
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        function submitForm() {
            const select = document.getElementById('network-select');
            const manualSsid = document.getElementById('manual-ssid');
            const password = document.getElementById('password');
            
            const ssid = select.value === 'manual' ? manualSsid.value : select.value;
            
            if (!ssid) {
                alert('Please select or enter a network');
                return;
            }
            
            const data = {
                ssid: ssid,
                password: password.value
            };
            
            fetch('/api/wifi', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(data)
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    alert('WiFi credentials updated. The device will now attempt to connect.');
                    setTimeout(() => {
                        window.location.href = 'http://MDNS_NAME.local/api/system';
                    }, 5000);
                } else {
                    alert('Error: ' + data.message);
                }
            })
            .catch(error => {
                console.error('Error:', error);
                alert('Failed to update WiFi credentials');
            });
        }
    </script>
</body>
</html>
)rawliteral";

const char JWT_CREATOR_HTML[] PROGMEM = R"==(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 JWT Creator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 800px; margin: 0 auto; }
    .jwt-form {
      margin: 20px 0;
      padding: 20px;
      border: 1px solid #ddd;
      border-radius: 8px;
    }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; }
    textarea {
      width: 100%;
      height: 120px;
      padding: 8px;
      margin-bottom: 10px;
      border: 1px solid #ddd;
      border-radius: 4px;
      font-family: monospace;
    }
    button {
      background-color: #4CAF50;
      color: white;
      padding: 10px 15px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
    #result {
      margin-top: 20px;
      padding: 10px;
      border: 1px solid #ddd;
      border-radius: 4px;
      display: none;
      word-break: break-all;
      font-family: monospace;
    }
    .error { background-color: #f8d7da; }
    .success { background-color: #d4edda; }
  </style>
</head>
<body>
  <div class="container">
    <h2>ESP32 JWT Creator</h2>
    <div class="jwt-form">
      <form id="jwtForm">
        <div class="form-group">
          <label for="header">Header JSON:</label>
          <textarea id="header" required>{
  "alg": "ES256K",
  "typ": "JWT"
}</textarea>
        </div>
        <div class="form-group">
          <label for="payload">Payload JSON:</label>
          <textarea id="payload" required>{
  "sub": "1234567890",
  "name": "John Doe",
  "iat": 1516239022
}</textarea>
        </div>
        <button type="submit">Create JWT</button>
      </form>
      <div id="result"></div>
    </div>
  </div>

  <script>
    function base64UrlEncode(str) {
      return btoa(str)
        .replace(/\+/g, '-')
        .replace(/\//g, '_')
        .replace(/=+$/, '');
    }

    document.getElementById('jwtForm').addEventListener('submit', async function(e) {
      e.preventDefault();
      const result = document.getElementById('result');
      
      try {
        // Validate and parse JSON
        const header = JSON.parse(document.getElementById('header').value);
        const payload = JSON.parse(document.getElementById('payload').value);

        // Create the JWT parts
        const headerBase64 = base64UrlEncode(JSON.stringify(header));
        const payloadBase64 = base64UrlEncode(JSON.stringify(payload));
        const message = headerBase64 + '.' + payloadBase64;

        // Get signature
        const response = await fetch('/api/sign', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({ message })
        });

        const data = await response.json();
        
        if (data.status === 'success') {
          // Create final JWT
          const jwt = message + '.' + data.signature;
          result.className = 'success';
          result.style.display = 'block';
          result.innerHTML = `<strong>JWT:</strong><br>${jwt}`;
        } else {
          throw new Error(data.message || 'Failed to sign JWT');
        }
      } catch (error) {
        result.className = 'error';
        result.style.display = 'block';
        result.innerHTML = 'Error: ' + error.message;
      }
    });
  </script>
</body>
</html>
)==";

const char SYSTEM_INFO_HTML[] PROGMEM = R"==(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 System Info</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 600px; margin: 0 auto; }
    .info-panel {
      margin: 20px 0;
      padding: 20px;
      border: 1px solid #ddd;
      border-radius: 8px;
    }
    .reset-button {
      background-color: #dc3545;
      color: white;
      padding: 10px 15px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
    #status {
      margin-top: 20px;
      padding: 10px;
      display: none;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>ESP32 System Information</h2>
    <div class="info-panel">
      <div id="systemInfo"></div>
      <hr style="margin: 20px 0;">
      <button class="reset-button" onclick="resetWiFi()">Reset WiFi</button>
      <div id="status"></div>
    </div>
  </div>

  <script>
    // Load system info
    fetch('/api/system/info')
      .then(response => response.json())
      .then(data => {
        const info = document.getElementById('systemInfo');
        info.innerHTML = `
          <p><strong>Heap:</strong> ${data.heap}</p>
          <p><strong>Device ID:</strong> ${data.deviceId}</p>
          <p><strong>CPU Frequency:</strong> ${data.cpuFreq} MHz</p>
          <p><strong>Flash Size:</strong> ${data.flashSize}</p>
          <p><strong>SDK Version:</strong> ${data.sdkVersion}</p>
          <p><strong>Public Key:</strong> ${data.publicKey}</p>
          <p><strong>WiFi Status:</strong> ${data.wifiStatus}</p>
          ${data.localIP ? `<p><strong>Local IP:</strong> ${data.localIP}</p>` : ''}
          ${data.ssid ? `<p><strong>Connected to:</strong> ${data.ssid}</p>` : ''}
          ${data.rssi ? `<p><strong>Signal Strength:</strong> ${data.rssi} dBm</p>` : ''}
        `;
      });

    function resetWiFi() {
      const status = document.getElementById('status');
      status.style.display = 'block';
      status.style.backgroundColor = '#fff3cd';
      status.innerHTML = 'Resetting WiFi...';

      fetch('/api/wifi/reset', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.status === 'success') {
            status.style.backgroundColor = '#d4edda';
            status.innerHTML = 'WiFi reset successful. Reloading...';
            setTimeout(() => {
              window.location.reload();
            }, 3000);
          } else {
            throw new Error(data.message || 'Failed to reset WiFi');
          }
        })
        .catch(error => {
          status.style.backgroundColor = '#f8d7da';
          status.innerHTML = 'Error: ' + error.message;
        });
    }
  </script>
</body>
</html>
)==";

const char INITIALIZE_FORM_HTML[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Gateway Initialize</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
    <h1>Initialize Gateway</h1>
    <form id="initForm">
        <label for="wallet">Wallet Address:</label><br>
        <input type="text" id="wallet" name="wallet" required><br>
        <label for="dryRun">Dry Run:</label>
        <input type="checkbox" id="dryRun" name="dryRun"><br><br>
        <button type="submit">Initialize</button>
    </form>
    <div id="result"></div>
</body>
</html>
)"; 