#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <Preferences.h>

// Configuration
#define DEFAULT_SSID "ThermalPrinter_GW"
#define DEFAULT_PASSWORD "printer123"
#define WEB_SERVER_PORT 80
#define PRINTER_BT_NAME "JK-5803P"

// Global objects
BluetoothSerial SerialBT;
WebServer server(WEB_SERVER_PORT);
Preferences preferences;

// State variables
bool btConnected = false;
String wifiSSID = "";
String wifiPassword = "";
bool useAPMode = true;

// Function declarations
void setupWiFi();
void setupBluetooth();
void setupWebServer();
void handleRoot();
void handlePrint();
void handleConfig();
void handleStatus();
void handleCORS();
void loadConfig();
void saveConfig();
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 Thermal Printer Gateway");
  Serial.println("=================================\n");
  
  // Load configuration
  loadConfig();
  
  // Setup WiFi
  setupWiFi();
  
  // Setup Bluetooth
  setupBluetooth();
  
  // Setup Web Server
  setupWebServer();
  
  Serial.println("\nSetup complete!");
  Serial.println("Gateway is ready to receive print jobs");
}

void loop() {
  server.handleClient();
  
  // Handle Bluetooth reconnection if disconnected
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    
    if (!SerialBT.connected() && btConnected) {
      btConnected = false;
      Serial.println("Bluetooth disconnected");
    } else if (SerialBT.connected() && !btConnected) {
      btConnected = true;
      Serial.println("Bluetooth connected!");
    }
  }
  
  delay(10);
}

void loadConfig() {
  preferences.begin("printer-gw", false);
  
  wifiSSID = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("password", "");
  useAPMode = preferences.getBool("apMode", true);
  
  Serial.println("Configuration loaded:");
  Serial.printf("  WiFi Mode: %s\n", useAPMode ? "Access Point" : "Station");
  if (!useAPMode && wifiSSID.length() > 0) {
    Serial.printf("  SSID: %s\n", wifiSSID.c_str());
  }
  
  preferences.end();
}

void saveConfig() {
  preferences.begin("printer-gw", false);
  
  preferences.putString("ssid", wifiSSID);
  preferences.putString("password", wifiPassword);
  preferences.putBool("apMode", useAPMode);
  
  preferences.end();
  Serial.println("Configuration saved");
}

void setupWiFi() {
  Serial.println("\n--- WiFi Setup ---");
  
  if (useAPMode || wifiSSID.length() == 0) {
    // Access Point mode
    Serial.println("Starting Access Point mode...");
    WiFi.mode(WIFI_AP);
    
    bool result = WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
    if (result) {
      Serial.println("Access Point started successfully");
      Serial.printf("  SSID: %s\n", DEFAULT_SSID);
      Serial.printf("  Password: %s\n", DEFAULT_PASSWORD);
      Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
      Serial.println("Failed to start Access Point!");
    }
  } else {
    // Station mode - connect to existing WiFi
    Serial.println("Connecting to WiFi...");
    Serial.printf("  SSID: %s\n", wifiSSID.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("\nFailed to connect to WiFi. Falling back to AP mode...");
      useAPMode = true;
      WiFi.mode(WIFI_AP);
      WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
      Serial.printf("  AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    }
  }
}

void setupBluetooth() {
  Serial.println("\n--- Bluetooth Setup ---");
  
  if (!SerialBT.begin("ESP32_Printer_Gateway")) {
    Serial.println("Bluetooth initialization failed!");
    return;
  }
  
  Serial.println("Bluetooth initialized successfully");
  Serial.printf("  Device name: ESP32_Printer_Gateway\n");
  Serial.printf("  Target printer: %s\n", PRINTER_BT_NAME);
  
  // Try to connect to the printer
  Serial.println("\nAttempting to connect to printer...");
  Serial.println("Please make sure the printer is powered on and in pairing mode");
  
  // Note: Auto-connection to a specific device requires the MAC address
  // For simplicity, we'll accept incoming connections or manual pairing
  Serial.println("Waiting for Bluetooth connection...");
}

void setupWebServer() {
  Serial.println("\n--- Web Server Setup ---");
  
  // CORS headers for all requests
  server.enableCORS(true);
  
  // Define routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/print", HTTP_POST, handlePrint);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/config", HTTP_GET, []() {
    String json = "{";
    json += "\"apMode\":" + String(useAPMode ? "true" : "false") + ",";
    json += "\"ssid\":\"" + wifiSSID + "\",";
    json += "\"btConnected\":" + String(btConnected ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });
  server.on("/status", HTTP_GET, handleStatus);
  
  // Handle OPTIONS for CORS preflight
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204);
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });
  
  server.begin();
  Serial.printf("Web server started on port %d\n", WEB_SERVER_PORT);
  
  if (useAPMode) {
    Serial.printf("  Access via: http://%s/\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("  Access via: http://%s/\n", WiFi.localIP().toString().c_str());
  }
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Thermal Printer Gateway</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            border-bottom: 2px solid #007bff;
            padding-bottom: 10px;
        }
        .status {
            padding: 15px;
            margin: 20px 0;
            border-radius: 4px;
            background-color: #e7f3ff;
            border-left: 4px solid #007bff;
        }
        .status.error {
            background-color: #ffe7e7;
            border-left-color: #dc3545;
        }
        .status.success {
            background-color: #e7ffe7;
            border-left-color: #28a745;
        }
        textarea {
            width: 100%;
            height: 200px;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-family: monospace;
            font-size: 14px;
            box-sizing: border-box;
        }
        button {
            background-color: #007bff;
            color: white;
            padding: 12px 30px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            margin-top: 10px;
        }
        button:hover {
            background-color: #0056b3;
        }
        button:disabled {
            background-color: #ccc;
            cursor: not-allowed;
        }
        .info {
            margin-top: 30px;
            padding: 15px;
            background-color: #fff3cd;
            border-radius: 4px;
        }
        .info h3 {
            margin-top: 0;
        }
        #statusIndicator {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🖨️ ESP32 Thermal Printer Gateway</h1>
        
        <div class="status" id="statusBox">
            <strong>Status:</strong>
            <div id="statusContent">Loading...</div>
        </div>
        
        <h2>Send Print Job</h2>
        <textarea id="printText" placeholder="Enter text to print...">Hello from WiFi!\nESP32 Thermal Printer Gateway\n\nThis is a test print.\n\n\n\n</textarea>
        <button onclick="sendPrint()" id="printButton">Send to Printer</button>
        
        <div class="info">
            <h3>📡 API Endpoints</h3>
            <ul>
                <li><strong>GET /status</strong> - Get gateway and printer status</li>
                <li><strong>POST /print</strong> - Send print job (raw text in body)</li>
                <li><strong>GET /config</strong> - Get current configuration</li>
                <li><strong>POST /config</strong> - Update WiFi configuration</li>
            </ul>
        </div>
    </div>
    
    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    const statusBox = document.getElementById('statusBox');
                    const statusContent = document.getElementById('statusContent');
                    
                    let html = '<div>';
                    html += '🌐 WiFi: ' + data.wifi + '<br>';
                    html += '📱 Bluetooth: ' + (data.btConnected ? '✅ Connected' : '❌ Disconnected') + '<br>';
                    html += '💾 Free Heap: ' + data.freeHeap + ' bytes';
                    html += '</div>';
                    
                    statusContent.innerHTML = html;
                    
                    if (data.btConnected) {
                        statusBox.className = 'status success';
                        document.getElementById('printButton').disabled = false;
                    } else {
                        statusBox.className = 'status error';
                        document.getElementById('printButton').disabled = true;
                    }
                })
                .catch(err => {
                    console.error('Status check failed:', err);
                });
        }
        
        function sendPrint() {
            const text = document.getElementById('printText').value;
            const button = document.getElementById('printButton');
            
            button.disabled = true;
            button.textContent = 'Sending...';
            
            fetch('/print', {
                method: 'POST',
                headers: {
                    'Content-Type': 'text/plain'
                },
                body: text
            })
            .then(response => response.json())
            .then(data => {
                alert(data.message || 'Print job sent successfully!');
                button.disabled = false;
                button.textContent = 'Send to Printer';
            })
            .catch(err => {
                alert('Failed to send print job: ' + err);
                button.disabled = false;
                button.textContent = 'Send to Printer';
            });
        }
        
        // Update status every 2 seconds
        updateStatus();
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handlePrint() {
  if (!SerialBT.connected()) {
    server.send(503, "application/json", "{\"error\":\"Printer not connected\"}");
    Serial.println("Print request received but printer not connected");
    return;
  }
  
  String data = server.arg("plain");
  
  if (data.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"No data to print\"}");
    return;
  }
  
  Serial.println("\n--- Print Job Received ---");
  Serial.printf("Data length: %d bytes\n", data.length());
  Serial.println("Data preview:");
  Serial.println(data.substring(0, min(100, (int)data.length())));
  if (data.length() > 100) {
    Serial.println("...");
  }
  
  // Send data to printer via Bluetooth
  size_t written = SerialBT.print(data);
  
  Serial.printf("Sent %d bytes to printer\n", written);
  
  String response = "{\"success\":true,\"message\":\"Print job sent\",\"bytes\":" + String(written) + "}";
  server.send(200, "application/json", response);
}

void handleConfig() {
  String body = server.arg("plain");
  
  // Simple JSON parsing (for production, use a proper JSON library)
  // Expected format: {"ssid":"network","password":"pass","apMode":false}
  
  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidStart);
  
  int passStart = body.indexOf("\"password\":\"") + 12;
  int passEnd = body.indexOf("\"", passStart);
  
  int apModePos = body.indexOf("\"apMode\":");
  bool newAPMode = true;
  
  if (ssidStart > 7 && ssidEnd > ssidStart) {
    wifiSSID = body.substring(ssidStart, ssidEnd);
  }
  
  if (passStart > 11 && passEnd > passStart) {
    wifiPassword = body.substring(passStart, passEnd);
  }
  
  if (apModePos > 0) {
    newAPMode = body.indexOf("true", apModePos) > 0;
  }
  
  useAPMode = newAPMode;
  
  saveConfig();
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuration saved. Restart device to apply.\"}");
  
  Serial.println("\n--- Configuration Updated ---");
  Serial.printf("AP Mode: %s\n", useAPMode ? "true" : "false");
  if (!useAPMode) {
    Serial.printf("SSID: %s\n", wifiSSID.c_str());
  }
}

void handleStatus() {
  String json = "{";
  
  // WiFi status
  if (useAPMode) {
    json += "\"wifi\":\"AP Mode - " + WiFi.softAPIP().toString() + "\",";
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      json += "\"wifi\":\"Connected - " + WiFi.localIP().toString() + "\",";
    } else {
      json += "\"wifi\":\"Disconnected\",";
    }
  }
  
  // Bluetooth status
  json += "\"btConnected\":" + String(SerialBT.connected() ? "true" : "false") + ",";
  
  // System info
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  
  json += "}";
  
  server.send(200, "application/json", json);
}
