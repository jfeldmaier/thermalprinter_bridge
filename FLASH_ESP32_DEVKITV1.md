# ESP32 DevKit V1 Installation - Spezifische Anleitung

## Für ESP32 DevKit V1 Board

### Hardware Identifikation

Das **ESP32 DevKit V1** ist ein 30-Pin Development Board mit:
- ESP32-WROOM-32 Modul
- CP2102 USB-zu-Serial Chip
- Micro-USB Anschluss
- 2x15 Pin Header

### Schritt-für-Schritt Installation

#### 1. Treiber Installation (falls erforderlich)

**Windows:**
1. CP2102 Treiber herunterladen: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
2. Installieren Sie den Treiber
3. ESP32 per USB verbinden
4. Im Geräte-Manager sollte ein COM-Port erscheinen (z.B. COM3)

**macOS:**
```bash
# CP2102 Treiber ist meist vorinstalliert
# Falls nicht, von Silicon Labs Website herunterladen
```

**Linux:**
```bash
# Treiber ist vorinstalliert
# Benutzer zur dialout Gruppe hinzufügen:
sudo usermod -a -G dialout $USER
# Neu anmelden nach diesem Befehl
```

#### 2. PlatformIO Installation

```bash
pip install platformio
```

#### 3. Projekt vorbereiten

```bash
# Repository klonen (falls noch nicht geschehen)
git clone https://github.com/jfeldmaier/thermalprinter_bridge.git
cd thermalprinter_bridge

# Zum Master-Branch wechseln nach dem Merge
git checkout main  # oder master, je nach Repository
git pull
```

#### 4. ESP32 DevKit V1 anschließen

1. Verbinden Sie das ESP32 DevKit V1 per Micro-USB
2. Warten Sie bis der Port erkannt wird
3. Port überprüfen:

```bash
# Verfügbare Ports anzeigen:
pio device list
```

**Typische Port-Namen:**
- Windows: `COM3`, `COM4`, etc.
- Linux: `/dev/ttyUSB0`, `/dev/ttyUSB1`
- macOS: `/dev/cu.usbserial-*`

#### 5. Firmware kompilieren und hochladen

**Automatischer Upload (empfohlen):**
```bash
# PlatformIO erkennt automatisch den richtigen Port
pio run --target upload
```

**Upload mit spezifischem Port:**
```bash
# Windows:
pio run --target upload --upload-port COM3

# Linux:
pio run --target upload --upload-port /dev/ttyUSB0

# macOS:
pio run --target upload --upload-port /dev/cu.usbserial-0001
```

**Bei Upload-Problemen:**

Wenn "Connecting..." erscheint und hängt:
1. Halten Sie die **BOOT** Taste am ESP32 gedrückt
2. Starten Sie den Upload-Befehl erneut
3. Warten Sie bis "Writing..." erscheint
4. Lassen Sie die BOOT Taste los

Oder verwenden Sie den automatischen Boot-Modus:
```bash
pio run --target upload --upload-resetmethod nodemcu
```

#### 6. Serial Monitor öffnen

```bash
# Serial Monitor starten:
pio device monitor

# Mit spezifischem Port:
pio device monitor --port COM3
```

**Erwartete Ausgabe:**
```
=================================
ESP32 Thermal Printer Gateway
=================================

Configuration loaded:
  WiFi Mode: Access Point

--- WiFi Setup ---
Starting Access Point mode...
Access Point started successfully
  SSID: ThermalPrinter_GW
  Password: Print$2026!
  IP Address: 192.168.4.1

--- Bluetooth Setup ---
Bluetooth initialized successfully
  Device name: ESP32_Printer_Gateway
  Target printer: JK-5803P

--- Web Server Setup ---
Web server started on port 80
  Access via: http://192.168.4.1/

Setup complete!
Gateway is ready to receive print jobs
```

#### 7. Testen

1. **WiFi Verbindung:**
   - Smartphone/Laptop WiFi öffnen
   - Netzwerk "ThermalPrinter_GW" suchen
   - Mit Passwort "Print$2026!" verbinden

2. **Web-Interface:**
   - Browser öffnen: http://192.168.4.1
   - Sie sollten die Gateway-Oberfläche sehen

3. **Drucker verbinden:**
   - JK-5803P einschalten
   - Bluetooth sollte sich automatisch verbinden
   - Im Web-Interface: Status = "Bluetooth: ✅ Connected"

### Technische Spezifikationen für DevKit V1

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    bblanchon/ArduinoJson@^6.21.3
upload_speed = 921600

# Optional: Erweiterte Einstellungen für DevKit V1
board_build.flash_mode = dio
board_build.f_cpu = 240000000L
board_build.f_flash = 40000000L
```

### Fehlerbehebung ESP32 DevKit V1

| Problem | Lösung |
|---------|--------|
| Port nicht erkannt | CP2102 Treiber installieren |
| Upload bleibt bei "Connecting..." | BOOT Taste drücken während Upload |
| "Brownout detector" Fehler | Besseres USB-Kabel verwenden, externes Netzteil |
| Serial Monitor zeigt Müll | Baudrate auf 115200 setzen |
| Flash schlägt fehl | `--upload-resetmethod nodemcu` verwenden |

### Spezielle Upload-Optionen für DevKit V1

```bash
# Upload mit automatischem Reset:
pio run --target upload --upload-resetmethod nodemcu

# Upload mit niedrigerer Geschwindigkeit (bei Problemen):
pio run --target upload --upload-speed 115200

# Kompletter Flash-Vorgang:
pio run --target erase
pio run --target upload
```

### Board-Spezifische Pins (Referenz)

Das Gateway nutzt keine GPIO-Pins (nur Bluetooth), aber für Referenz:

```
ESP32 DevKit V1 Pin-Layout:
- USB: CP2102 auf GPIO1 (TX) und GPIO3 (RX)
- Boot Button: GPIO0
- Enable Button: EN/RST Pin
- LED: GPIO2 (meist onboard)
```

### Nach erfolgreichem Flash

**Erfolgsanzeige:**
```
Writing at 0x00010000... (100%)
Wrote 847872 bytes (534321 compressed) at 0x00010000 in 12.3 seconds
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
```

**Nächste Schritte:**
1. ✅ ESP32 trennen und neu verbinden (oder RESET drücken)
2. ✅ Mit WiFi "ThermalPrinter_GW" verbinden
3. ✅ Browser öffnen: http://192.168.4.1
4. ✅ JK-5803P Drucker einschalten
5. ✅ Ersten Druckauftrag senden!

### Permanente Installation

Für den Dauerbetrieb:
1. ESP32 mit 5V Netzteil versorgen (min. 500mA)
2. Optional: Gehäuse verwenden
3. WiFi-Konfiguration über Web-Interface anpassen

### Automatischer Start nach Stromausfall

Der ESP32 startet automatisch nach Stromausfall und:
- Stellt WiFi Access Point wieder her
- Wartet auf Bluetooth-Verbindung
- Startet Web-Server neu

Keine manuelle Konfiguration erforderlich!

---

## Zusammenfassung Schnellbefehle

```bash
# Installation
pip install platformio

# Projekt klonen
git clone https://github.com/jfeldmaier/thermalprinter_bridge.git
cd thermalprinter_bridge

# ESP32 DevKit V1 flashen
pio run --target upload

# Serial Monitor
pio device monitor

# Bei Problemen
pio run --target upload --upload-resetmethod nodemcu --upload-speed 115200
```

**Fertig! Das Gateway ist einsatzbereit! 🚀**
