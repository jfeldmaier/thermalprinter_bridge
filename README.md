# ESP32 Thermal Printer WiFi Gateway / WiFi-Gateway für Thermaldrucker

Ein ESP32-basierter Gateway, der es ermöglicht, Druckaufträge per WiFi an einen Bluetooth-Thermaldrucker (JK-5803P) zu senden.

An ESP32-based gateway that enables sending print jobs via WiFi to a Bluetooth thermal printer (JK-5803P).

![ESP32 Gateway Architecture](https://img.shields.io/badge/ESP32-WiFi%20%E2%86%94%20Bluetooth-blue)
![Platform](https://img.shields.io/badge/platform-PlatformIO-orange)
![License](https://img.shields.io/badge/license-MIT-green)

## Features / Funktionen

- 🌐 **WiFi Access Point Mode** - Creates its own WiFi network / Erstellt eigenes WiFi-Netzwerk
- 📡 **WiFi Station Mode** - Connects to existing WiFi / Verbindet sich mit bestehendem WiFi
- 📱 **Bluetooth Classic SPP** - Communicates with JK-5803P printer / Kommuniziert mit JK-5803P Drucker
- 🖥️ **Web Interface** - Easy-to-use web UI for testing / Einfache Web-Oberfläche zum Testen
- 🔌 **REST API** - Send print jobs programmatically / Druckaufträge programmgesteuert senden
- 💾 **Persistent Configuration** - Saves WiFi settings / Speichert WiFi-Einstellungen
- 🔄 **Auto-Reconnection** - Handles connection drops / Behandelt Verbindungsabbrüche

## Hardware Requirements / Hardware-Anforderungen

- ESP32 Development Board (z.B. ESP32-DevKitC, NodeMCU-32S)
- JK-5803P Bluetooth Thermal Printer
- USB Cable for programming / USB-Kabel zum Programmieren
- Power supply (5V) / Stromversorgung (5V)

## Software Requirements / Software-Anforderungen

- [PlatformIO](https://platformio.org/) (recommended / empfohlen)
  - oder Arduino IDE with ESP32 support
- USB Driver for ESP32 (CH340/CP2102)

## Installation / Einrichtung

### Method 1: Using PlatformIO (Recommended)

1. **Install PlatformIO**
   ```bash
   # Install PlatformIO Core
   pip install platformio
   ```

2. **Clone Repository**
   ```bash
   git clone https://github.com/jfeldmaier/thermalprinter_bridge.git
   cd thermalprinter_bridge
   ```

3. **Build and Upload**
   ```bash
   # Build the project
   pio run
   
   # Upload to ESP32
   pio run --target upload
   
   # Monitor serial output
   pio device monitor
   ```

### Method 2: Using Arduino IDE

1. **Install Arduino IDE** and ESP32 board support
2. **Open** `src/main.cpp` in Arduino IDE (rename to `.ino` if needed)
3. **Select Board**: Tools → Board → ESP32 Dev Module
4. **Select Port**: Tools → Port → (your ESP32 port)
5. **Upload** the sketch

## Configuration / Konfiguration

### Initial Setup / Ersteinrichtung

1. **Power on the ESP32** / ESP32 einschalten
2. **Connect to WiFi Access Point**:
   - SSID: `ThermalPrinter_GW`
   - Password: `printer123`
3. **Open browser** and navigate to: `http://192.168.4.1`

### WiFi Modes / WiFi-Modi

#### Access Point Mode (Default)
- ESP32 creates its own WiFi network
- IP Address: `192.168.4.1`
- Ideal for standalone operation

#### Station Mode
- Connects to existing WiFi network
- Configure via web interface or API
- Allows integration into existing network

### Printer Connection / Drucker-Verbindung

1. **Power on the JK-5803P printer**
2. **Enable Bluetooth pairing mode** on the printer
3. **Pair the printer** with the ESP32:
   - Open Bluetooth settings on your phone/computer
   - Find "ESP32_Printer_Gateway"
   - Connect and enter PIN if required (usually 1234 or 0000)
   - The printer should now appear in the available devices
   - Select "JK-5803P" or your printer's Bluetooth name
4. **Check status** on the web interface - it should show "Bluetooth: Connected"

## Usage / Verwendung

### Web Interface

1. Navigate to the gateway's IP address in a web browser
2. Enter text in the text area
3. Click "Send to Printer" button
4. Text will be printed on the thermal printer

### REST API

#### Check Status
```bash
curl http://192.168.4.1/status
```

Response:
```json
{
  "wifi": "AP Mode - 192.168.4.1",
  "btConnected": true,
  "freeHeap": 234567,
  "uptime": 1234
}
```

#### Send Print Job
```bash
curl -X POST http://192.168.4.1/print \
  -H "Content-Type: text/plain" \
  -d "Hello from WiFi Gateway!\nThis is a test print.\n\n\n"
```

Response:
```json
{
  "success": true,
  "message": "Print job sent",
  "bytes": 45
}
```

#### Get Configuration
```bash
curl http://192.168.4.1/config
```

#### Update Configuration
```bash
curl -X POST http://192.168.4.1/config \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyWiFi","password":"MyPassword","apMode":false}'
```

## Examples / Beispiele

### Python Example
```python
import requests

# Print text
response = requests.post(
    'http://192.168.4.1/print',
    data='Hello from Python!\n\n\n',
    headers={'Content-Type': 'text/plain'}
)
print(response.json())
```

### Node.js Example
```javascript
const axios = require('axios');

async function print(text) {
  const response = await axios.post(
    'http://192.168.4.1/print',
    text,
    { headers: { 'Content-Type': 'text/plain' } }
  );
  console.log(response.data);
}

print('Hello from Node.js!\n\n\n');
```

### curl Example
```bash
# Simple text print
echo -e "Hello World!\nLine 2\nLine 3\n\n\n" | \
  curl -X POST http://192.168.4.1/print \
  -H "Content-Type: text/plain" \
  --data-binary @-
```

## Troubleshooting / Fehlerbehebung

### Bluetooth nicht verbunden / Bluetooth Not Connected

1. Ensure printer is powered on / Drucker eingeschaltet
2. Printer is in pairing mode / Drucker im Pairing-Modus
3. Try pairing manually from phone first / Zuerst manuell vom Handy pairen
4. Restart ESP32 / ESP32 neu starten
5. Check serial monitor for error messages / Serial Monitor für Fehlermeldungen prüfen

### WiFi Connection Issues

1. Check SSID and password / SSID und Passwort prüfen
2. Ensure WiFi signal is strong enough / WiFi-Signal stark genug
3. Try resetting to AP mode by clearing configuration / Auf AP-Modus zurücksetzen
4. Restart the ESP32 / ESP32 neu starten

### Serial Monitor Shows Errors

```bash
# View serial output
pio device monitor -b 115200
```

Common issues:
- `Bluetooth initialization failed!` - Restart ESP32
- `Failed to connect to WiFi` - Check credentials, will fall back to AP mode
- `Printer not connected` - Pair the printer first

## Advanced Configuration / Erweiterte Konfiguration

### Changing Default Settings

Edit `src/main.cpp`:

```cpp
// WiFi Settings
#define DEFAULT_SSID "ThermalPrinter_GW"  // Change AP SSID
#define DEFAULT_PASSWORD "printer123"      // Change AP password

// Printer Settings
#define PRINTER_BT_NAME "JK-5803P"         // Change printer Bluetooth name
```

### Adding Printer Commands

The JK-5803P supports ESC/POS commands. Example:

```cpp
// In handlePrint() function, add printer commands:

// Bold text
SerialBT.write(0x1B); // ESC
SerialBT.write(0x45); // E
SerialBT.write(0x01); // Enable

// Print text
SerialBT.print(data);

// Reset formatting
SerialBT.write(0x1B); // ESC
SerialBT.write(0x45); // E
SerialBT.write(0x00); // Disable
```

## Project Structure / Projektstruktur

```
thermalprinter_bridge/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp           # Main application code
├── README.md              # This file
└── LICENSE                # MIT License
```

## Technical Details / Technische Details

### System Architecture / Systemarchitektur

```
[Client Device]
     ↓ WiFi
[ESP32 Gateway]
     ↓ Bluetooth SPP
[JK-5803P Printer]
```

### Communication Flow / Kommunikationsfluss

1. Client sends HTTP POST to `/print` endpoint
2. ESP32 receives data via WiFi
3. ESP32 forwards data via Bluetooth Serial to printer
4. Printer receives and prints the data
5. ESP32 sends success response to client

### Memory Usage / Speicherverbrauch

- Flash: ~300 KB
- RAM: ~50 KB during operation
- Free heap typically: 200-250 KB

## Contributing / Beitragen

Contributions are welcome! / Beiträge sind willkommen!

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License / Lizenz

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support / Unterstützung

For issues and questions:
- Open an issue on GitHub
- Check the troubleshooting section
- Review the serial monitor output for detailed logs

## Acknowledgments / Danksagungen

- ESP32 Arduino Core Team
- PlatformIO Team
- BluetoothSerial Library Contributors

## Future Enhancements / Zukünftige Erweiterungen

- [ ] mDNS support for easy discovery
- [ ] HTTPS support for secure communication
- [ ] Image printing support
- [ ] QR code generation and printing
- [ ] Multiple printer support
- [ ] Web-based configuration interface
- [ ] OTA (Over-The-Air) updates
- [ ] ESC/POS command library integration

---

**Made with ❤️ for the maker community**