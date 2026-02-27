#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Configuration
#define DEFAULT_SSID "ThermalPrinter_GW"
#define DEFAULT_PASSWORD "Print$2026!"
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
void handlePrintImage();
void handleConfig();
void handleStatus();
void handleCORS();
void loadConfig();
void saveConfig();
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void printImageData(const uint8_t* imageData, int width, int height);
uint8_t* base64Decode(const char* input, size_t inputLen, size_t* outputLen);

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
  server.on("/printImage", HTTP_POST, handlePrintImage);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/config", HTTP_GET, []() {
    StaticJsonDocument<128> doc;
    doc["apMode"] = useAPMode;
    doc["ssid"] = wifiSSID;
    doc["btConnected"] = btConnected;
    
    String json;
    serializeJson(doc, json);
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
        .image-section {
            margin-top: 30px;
            padding-top: 20px;
            border-top: 2px solid #dee2e6;
        }
        .file-input-wrapper {
            position: relative;
            overflow: hidden;
            display: inline-block;
            margin: 10px 0;
        }
        .file-input-wrapper input[type=file] {
            position: absolute;
            left: -9999px;
        }
        .file-input-label {
            background-color: #28a745;
            color: white;
            padding: 12px 30px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            display: inline-block;
        }
        .file-input-label:hover {
            background-color: #218838;
        }
        #imagePreview {
            max-width: 100%;
            margin: 20px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
            display: none;
        }
        #imageCanvas {
            max-width: 100%;
            border: 1px solid #ddd;
            border-radius: 4px;
            display: none;
        }
        .button-secondary {
            background-color: #6c757d;
        }
        .button-secondary:hover {
            background-color: #5a6268;
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
        
        <div class="image-section">
            <h2>🖼️ Print Image</h2>
            <div class="file-input-wrapper">
                <label for="imageFile" class="file-input-label">Choose Image File</label>
                <input type="file" id="imageFile" accept="image/*" onchange="handleImageSelect(event)">
            </div>
            <p style="color: #666; font-size: 14px;">Supported formats: JPG, PNG, GIF (max width: 384px recommended)</p>
            <canvas id="imageCanvas"></canvas>
            <img id="imagePreview" alt="Preview">
            <div id="imageControls" style="display: none; margin: 10px 0;">
                <button onclick="sendImage()" id="printImageButton">Print Image</button>
                <button onclick="clearImage()" class="button-secondary">Clear</button>
            </div>
        </div>
        
        <div class="info">
            <h3>📡 API Endpoints</h3>
            <ul>
                <li><strong>GET /status</strong> - Get gateway and printer status</li>
                <li><strong>POST /print</strong> - Send print job (raw text in body)</li>
                <li><strong>POST /printImage</strong> - Print image (JSON with base64 data, width, height)</li>
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
        
        // Image handling variables
        let currentImageData = null;
        let currentImageWidth = 0;
        let currentImageHeight = 0;
        
        function handleImageSelect(event) {
            const file = event.target.files[0];
            if (!file) return;
            
            const reader = new FileReader();
            reader.onload = function(e) {
                const img = new Image();
                img.onload = function() {
                    processImage(img);
                };
                img.src = e.target.result;
            };
            reader.readAsDataURL(file);
        }
        
        function processImage(img) {
            // Limit width to 384 pixels (48 bytes) for thermal printer
            const maxWidth = 384;
            let width = img.width;
            let height = img.height;
            
            if (width > maxWidth) {
                height = Math.floor(height * (maxWidth / width));
                width = maxWidth;
            }
            
            // Create canvas and draw image
            const canvas = document.getElementById('imageCanvas');
            const ctx = canvas.getContext('2d');
            
            canvas.width = width;
            canvas.height = height;
            
            // Draw image
            ctx.drawImage(img, 0, 0, width, height);
            
            // Convert to black and white bitmap
            const imageData = ctx.getImageData(0, 0, width, height);
            const pixels = imageData.data;
            
            // Apply dithering and convert to 1-bit
            const threshold = 128;
            const widthBytes = Math.ceil(width / 8);
            const bitmapData = new Uint8Array(widthBytes * height);
            
            for (let y = 0; y < height; y++) {
                for (let x = 0; x < width; x++) {
                    const idx = (y * width + x) * 4;
                    const gray = (pixels[idx] + pixels[idx + 1] + pixels[idx + 2]) / 3;
                    const isBlack = gray < threshold;
                    
                    if (isBlack) {
                        const byteIdx = y * widthBytes + Math.floor(x / 8);
                        const bitIdx = 7 - (x % 8);
                        bitmapData[byteIdx] |= (1 << bitIdx);
                    }
                    
                    // Update canvas to show B&W version
                    const color = isBlack ? 0 : 255;
                    pixels[idx] = color;
                    pixels[idx + 1] = color;
                    pixels[idx + 2] = color;
                }
            }
            
            ctx.putImageData(imageData, 0, 0);
            
            // Show canvas and controls
            canvas.style.display = 'block';
            document.getElementById('imageControls').style.display = 'block';
            
            // Store bitmap data
            currentImageData = btoa(String.fromCharCode.apply(null, bitmapData));
            currentImageWidth = width;
            currentImageHeight = height;
            
            console.log('Image processed: ' + width + 'x' + height + ' pixels');
        }
        
        function sendImage() {
            if (!currentImageData) {
                alert('Please select an image first');
                return;
            }
            
            const button = document.getElementById('printImageButton');
            button.disabled = true;
            button.textContent = 'Sending...';
            
            const payload = {
                data: currentImageData,
                width: currentImageWidth,
                height: currentImageHeight
            };
            
            fetch('/printImage', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(payload)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    alert('Image sent to printer successfully!');
                } else {
                    alert('Error: ' + (data.error || 'Unknown error'));
                }
                button.disabled = false;
                button.textContent = 'Print Image';
            })
            .catch(err => {
                alert('Failed to send image: ' + err);
                button.disabled = false;
                button.textContent = 'Print Image';
            });
        }
        
        function clearImage() {
            currentImageData = null;
            currentImageWidth = 0;
            currentImageHeight = 0;
            document.getElementById('imageCanvas').style.display = 'none';
            document.getElementById('imageControls').style.display = 'none';
            document.getElementById('imageFile').value = '';
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
    StaticJsonDocument<64> doc;
    doc["error"] = "Printer not connected";
    String json;
    serializeJson(doc, json);
    server.send(503, "application/json", json);
    Serial.println("Print request received but printer not connected");
    return;
  }
  
  String data = server.arg("plain");
  
  if (data.length() == 0) {
    StaticJsonDocument<64> doc;
    doc["error"] = "No data to print";
    String json;
    serializeJson(doc, json);
    server.send(400, "application/json", json);
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
  
  StaticJsonDocument<128> doc;
  doc["success"] = true;
  doc["message"] = "Print job sent";
  doc["bytes"] = written;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleConfig() {
  String body = server.arg("plain");
  
  // Parse JSON using ArduinoJson library
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    StaticJsonDocument<128> errorDoc;
    String errorMsg = "Invalid JSON: ";
    errorMsg += error.c_str();
    errorDoc["error"] = errorMsg;
    
    String json;
    serializeJson(errorDoc, json);
    server.send(400, "application/json", json);
    return;
  }
  
  // Extract configuration values
  if (doc.containsKey("ssid")) {
    wifiSSID = doc["ssid"].as<String>();
  }
  
  if (doc.containsKey("password")) {
    wifiPassword = doc["password"].as<String>();
  }
  
  if (doc.containsKey("apMode")) {
    useAPMode = doc["apMode"].as<bool>();
  }
  
  saveConfig();
  
  StaticJsonDocument<128> responseDoc;
  responseDoc["success"] = true;
  responseDoc["message"] = "Configuration saved. Restart device to apply.";
  
  String json;
  serializeJson(responseDoc, json);
  server.send(200, "application/json", json);
  
  Serial.println("\n--- Configuration Updated ---");
  Serial.printf("AP Mode: %s\n", useAPMode ? "true" : "false");
  if (!useAPMode) {
    Serial.printf("SSID: %s\n", wifiSSID.c_str());
  }
}

void handleStatus() {
  StaticJsonDocument<256> doc;
  
  // WiFi status
  if (useAPMode) {
    doc["wifi"] = "AP Mode - " + WiFi.softAPIP().toString();
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      doc["wifi"] = "Connected - " + WiFi.localIP().toString();
    } else {
      doc["wifi"] = "Disconnected";
    }
  }
  
  // Bluetooth status
  doc["btConnected"] = SerialBT.connected();
  
  // System info
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Base64 decoding function
uint8_t* base64Decode(const char* input, size_t inputLen, size_t* outputLen) {
  const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
  size_t output_length = inputLen / 4 * 3;
  if (input[inputLen - 1] == '=') output_length--;
  if (input[inputLen - 2] == '=') output_length--;
  
  uint8_t* decoded = (uint8_t*)malloc(output_length);
  if (decoded == NULL) {
    *outputLen = 0;
    return NULL;
  }
  
  size_t j = 0;
  uint32_t buffer = 0;
  int bits = 0;
  
  for (size_t i = 0; i < inputLen; i++) {
    if (input[i] == '=') break;
    
    const char* p = strchr(base64_chars, input[i]);
    if (p == NULL) continue;
    
    buffer = (buffer << 6) | (p - base64_chars);
    bits += 6;
    
    if (bits >= 8) {
      bits -= 8;
      decoded[j++] = (buffer >> bits) & 0xFF;
    }
  }
  
  *outputLen = j;
  return decoded;
}

// Print bitmap image data to thermal printer using ESC/POS commands
void printImageData(const uint8_t* imageData, int width, int height) {
  // ESC/POS command for bitmap printing
  // GS v 0 - Print raster bitmap
  
  int widthBytes = (width + 7) / 8;  // Width in bytes (8 pixels per byte)
  
  Serial.printf("Printing image: %dx%d pixels (%d bytes wide)\n", width, height, widthBytes);
  
  // Send ESC * command for bit image
  // ESC * m nL nH d1...dk
  // m = mode (0 = 8-dot single-density)
  // nL nH = number of dots in horizontal direction (low byte, high byte)
  
  for (int y = 0; y < height; y += 24) {
    int linesThisPass = min(24, height - y);
    
    // ESC * 33 (ESC * ! for 24-dot double-density)
    SerialBT.write(0x1B);  // ESC
    SerialBT.write(0x2A);  // *
    SerialBT.write(33);    // 24-dot double-density mode
    
    // Width in low byte, high byte
    SerialBT.write(width & 0xFF);
    SerialBT.write((width >> 8) & 0xFF);
    
    // Send image data for this stripe
    for (int x = 0; x < width; x++) {
      for (int k = 0; k < 3; k++) {  // 3 bytes for 24 dots
        uint8_t byte = 0;
        for (int b = 0; b < 8; b++) {
          int py = y + (k * 8) + b;
          if (py < height) {
            int byteIndex = py * widthBytes + (x / 8);
            int bitIndex = 7 - (x % 8);
            if (imageData[byteIndex] & (1 << bitIndex)) {
              byte |= (1 << (7 - b));
            }
          }
        }
        SerialBT.write(byte);
      }
    }
    
    // Line feed after each stripe
    SerialBT.write(0x0A);
  }
  
  // Add some line feeds for paper advance
  SerialBT.print("\n\n\n");
}

// Handle image upload and print
void handlePrintImage() {
  if (!SerialBT.connected()) {
    StaticJsonDocument<64> doc;
    doc["error"] = "Printer not connected";
    String json;
    serializeJson(doc, json);
    server.send(503, "application/json", json);
    Serial.println("Image print request received but printer not connected");
    return;
  }
  
  String body = server.arg("plain");
  
  if (body.length() == 0) {
    StaticJsonDocument<64> doc;
    doc["error"] = "No data received";
    String json;
    serializeJson(doc, json);
    server.send(400, "application/json", json);
    return;
  }
  
  // Parse JSON
  DynamicJsonDocument doc(body.length() + 1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    StaticJsonDocument<128> errorDoc;
    String errorMsg = "Invalid JSON: ";
    errorMsg += error.c_str();
    errorDoc["error"] = errorMsg;
    
    String json;
    serializeJson(errorDoc, json);
    server.send(400, "application/json", json);
    return;
  }
  
  // Extract image data, width, and height
  if (!doc.containsKey("data") || !doc.containsKey("width") || !doc.containsKey("height")) {
    StaticJsonDocument<128> errorDoc;
    errorDoc["error"] = "Missing required fields: data, width, height";
    
    String json;
    serializeJson(errorDoc, json);
    server.send(400, "application/json", json);
    return;
  }
  
  String base64Data = doc["data"].as<String>();
  int width = doc["width"].as<int>();
  int height = doc["height"].as<int>();
  
  Serial.println("\n--- Image Print Job Received ---");
  Serial.printf("Image size: %dx%d pixels\n", width, height);
  Serial.printf("Base64 data length: %d bytes\n", base64Data.length());
  
  // Validate dimensions
  if (width <= 0 || width > 576 || height <= 0 || height > 2000) {
    StaticJsonDocument<128> errorDoc;
    errorDoc["error"] = "Invalid image dimensions (max 576x2000)";
    
    String json;
    serializeJson(errorDoc, json);
    server.send(400, "application/json", json);
    return;
  }
  
  // Decode base64 image data
  size_t decodedLen = 0;
  uint8_t* imageData = base64Decode(base64Data.c_str(), base64Data.length(), &decodedLen);
  
  if (imageData == NULL || decodedLen == 0) {
    StaticJsonDocument<128> errorDoc;
    errorDoc["error"] = "Failed to decode image data";
    
    String json;
    serializeJson(errorDoc, json);
    server.send(500, "application/json", json);
    return;
  }
  
  Serial.printf("Decoded image data: %d bytes\n", decodedLen);
  
  // Expected size: (width + 7) / 8 * height bytes
  int expectedSize = ((width + 7) / 8) * height;
  
  if (decodedLen < expectedSize) {
    Serial.printf("Warning: decoded data (%d bytes) smaller than expected (%d bytes)\n", 
                  decodedLen, expectedSize);
  }
  
  // Print the image
  printImageData(imageData, width, height);
  
  // Free allocated memory
  free(imageData);
  
  Serial.println("Image sent to printer successfully");
  
  StaticJsonDocument<128> responseDoc;
  responseDoc["success"] = true;
  responseDoc["message"] = "Image print job sent";
  responseDoc["width"] = width;
  responseDoc["height"] = height;
  
  String json;
  serializeJson(responseDoc, json);
  server.send(200, "application/json", json);
}
