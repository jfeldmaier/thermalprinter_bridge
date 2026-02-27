# ESP32 Installation Anleitung / Installation Guide

Dieses Dokument beschreibt Schritt für Schritt, wie Sie die Thermal Printer Gateway Firmware auf Ihrem ESP32 installieren.

This document describes step-by-step how to install the Thermal Printer Gateway firmware on your ESP32.

---

## Voraussetzungen / Prerequisites

### Hardware
- ✅ ESP32 Development Board (z.B. ESP32-DevKitC, NodeMCU-32S, WROOM-32)
- ✅ USB-Kabel (Micro-USB oder USB-C, je nach Board)
- ✅ Computer (Windows, macOS oder Linux)
- ✅ JK-5803P Bluetooth Thermaldrucker

### Software
Sie benötigen Python 3.7 oder höher auf Ihrem Computer installiert.

You need Python 3.7 or higher installed on your computer.

---

## Methode 1: Installation mit PlatformIO (Empfohlen / Recommended)

### Schritt 1: PlatformIO installieren

**Windows / macOS / Linux:**
```bash
# Öffnen Sie Terminal/Command Prompt und führen Sie aus:
pip install platformio

# Alternativ mit pip3:
pip3 install platformio

# Überprüfen Sie die Installation:
pio --version
```

**Bei Problemen:**
- Stellen Sie sicher, dass Python und pip installiert sind: `python --version` und `pip --version`
- Unter Windows evtl. als Administrator ausführen
- Unter Linux evtl. `sudo` verwenden: `sudo pip install platformio`

### Schritt 2: Repository herunterladen

**Option A: Mit Git (empfohlen)**
```bash
git clone https://github.com/jfeldmaier/thermalprinter_bridge.git
cd thermalprinter_bridge
```

**Option B: Als ZIP herunterladen**
1. Besuchen Sie: https://github.com/jfeldmaier/thermalprinter_bridge
2. Klicken Sie auf "Code" → "Download ZIP"
3. Entpacken Sie die ZIP-Datei
4. Öffnen Sie Terminal im entpackten Ordner

### Schritt 3: ESP32 anschließen

1. Verbinden Sie den ESP32 über USB mit Ihrem Computer
2. Der ESP32 sollte als COM-Port (Windows) oder /dev/ttyUSB0 (Linux) erkannt werden

**Treiber installieren (falls nötig):**
- **CH340 Treiber**: Für viele günstige ESP32 Boards
  - Windows: https://www.wch-ic.com/downloads/CH341SER_ZIP.html
  - macOS: https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
- **CP2102 Treiber**: Für einige DevKit-C Boards
  - https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

### Schritt 4: Firmware kompilieren

```bash
# Im Projektverzeichnis thermalprinter_bridge:
pio run
```

**Was passiert:**
- PlatformIO lädt die ESP32-Plattform herunter (~1-2 GB beim ersten Mal)
- Die erforderlichen Bibliotheken werden installiert
- Der Code wird kompiliert

**Dauer:** 5-10 Minuten beim ersten Mal, danach schneller.

### Schritt 5: Auf ESP32 hochladen

```bash
# Firmware auf ESP32 hochladen:
pio run --target upload
```

**Wenn der Upload fehlschlägt:**
1. Überprüfen Sie die USB-Verbindung
2. Drücken Sie die BOOT-Taste am ESP32 während des Uploads
3. Spezifizieren Sie den Port manuell:
   ```bash
   # Windows:
   pio run --target upload --upload-port COM3
   
   # Linux/macOS:
   pio run --target upload --upload-port /dev/ttyUSB0
   ```

4. Port herausfinden:
   ```bash
   # Alle verfügbaren Ports anzeigen:
   pio device list
   ```

### Schritt 6: Serielle Ausgabe überwachen (Optional)

```bash
# Serielle Konsole öffnen:
pio device monitor
```

**Sie sollten sehen:**
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

--- Web Server Setup ---
Web server started on port 80
  Access via: http://192.168.4.1/

Setup complete!
Gateway is ready to receive print jobs
```

**Zum Beenden:** Drücken Sie `Ctrl+C`

---

## Methode 2: Installation mit Arduino IDE

### Schritt 1: Arduino IDE installieren

1. Laden Sie Arduino IDE herunter: https://www.arduino.cc/en/software
2. Installieren Sie die IDE

### Schritt 2: ESP32 Board Support hinzufügen

1. Öffnen Sie Arduino IDE
2. Gehen Sie zu: **Datei** → **Voreinstellungen** (File → Preferences)
3. Fügen Sie diese URL in "Zusätzliche Boardverwalter-URLs" ein:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Klicken Sie OK
5. Gehen Sie zu: **Werkzeuge** → **Board** → **Boardverwalter**
6. Suchen Sie nach "esp32"
7. Installieren Sie "esp32 by Espressif Systems"

### Schritt 3: Projekt öffnen

1. Laden Sie das Repository herunter (siehe Methode 1, Schritt 2)
2. Öffnen Sie die Datei: `src/main.cpp`
3. Speichern Sie die Datei als `main.ino` (Arduino erfordert .ino Dateien)

### Schritt 4: Bibliotheken installieren

Gehen Sie zu: **Sketch** → **Bibliothek einbinden** → **Bibliotheken verwalten**

Installieren Sie:
- **ArduinoJson** (Version 6.21.3 oder höher)

### Schritt 5: Board-Einstellungen

**Werkzeuge → Board:** ESP32 Dev Module (oder Ihr spezifisches Board)  
**Werkzeuge → Upload Speed:** 921600  
**Werkzeuge → Port:** Wählen Sie Ihren ESP32 Port  

### Schritt 6: Upload

Klicken Sie auf den **Upload-Button** (Pfeil nach rechts) ➡️

---

## Nach der Installation / After Installation

### 1. ESP32 neu starten
Trennen Sie das USB-Kabel und schließen Sie es wieder an, oder drücken Sie den RESET-Knopf am ESP32.

### 2. Mit WiFi verbinden

**Auf Ihrem Smartphone/Computer:**
1. Öffnen Sie WiFi-Einstellungen
2. Suchen Sie nach dem Netzwerk: **ThermalPrinter_GW**
3. Verbinden Sie sich mit dem Passwort: **Print$2026!**

### 3. Web-Interface öffnen

Öffnen Sie einen Webbrowser und gehen Sie zu: **http://192.168.4.1**

Sie sollten das Thermal Printer Gateway Interface sehen!

### 4. Drucker verbinden (Bluetooth Pairing)

**Wichtig:** Der JK-5803P muss erst mit dem ESP32 gepaart werden.

**Option A: Über Android/iPhone:**
1. Einschalten des JK-5803P Druckers
2. Bluetooth auf Smartphone aktivieren
3. Nach "ESP32_Printer_Gateway" suchen
4. Verbinden (PIN: meist 1234 oder 0000)
5. Dann nach "JK-5803P" suchen und verbinden

**Option B: Direkte Paarung:**
1. Der ESP32 wartet auf Verbindungen
2. Schalten Sie den JK-5803P ein
3. Der Drucker sollte sich automatisch verbinden

**Status prüfen:**
Im Web-Interface sollte "Bluetooth: ✅ Connected" erscheinen.

---

## Fehlerbehebung / Troubleshooting

### Problem: "pip: command not found"
**Lösung:** Python ist nicht installiert oder nicht im PATH
```bash
# Python installieren:
# Windows: https://www.python.org/downloads/
# macOS: brew install python3
# Linux: sudo apt install python3 python3-pip
```

### Problem: "pio: command not found"
**Lösung:** PlatformIO wurde nicht korrekt installiert
```bash
# Neuinstallation:
pip install --upgrade platformio

# PATH aktualisieren (Linux/macOS):
export PATH=$PATH:~/.platformio/penv/bin

# Windows: Neustart des Terminals
```

### Problem: "Could not open port"
**Lösung:** 
1. Schließen Sie Arduino Serial Monitor falls offen
2. Schließen Sie PlatformIO Monitor: `Ctrl+C`
3. Überprüfen Sie USB-Verbindung
4. Installieren Sie Treiber (siehe Schritt 3)

### Problem: ESP32 startet nicht / keine Ausgabe
**Lösung:**
1. Drücken Sie RESET-Knopf am ESP32
2. Überprüfen Sie Stromversorgung (gutes USB-Kabel verwenden)
3. Prüfen Sie mit Serial Monitor (115200 Baud)

### Problem: Kann nicht mit WiFi verbinden
**Lösung:**
1. Warten Sie 10-20 Sekunden nach dem Start
2. Suchen Sie erneut nach WiFi-Netzwerken
3. Prüfen Sie Serial Monitor für Fehlermeldungen
4. Drücken Sie RESET am ESP32

### Problem: Drucker verbindet nicht über Bluetooth
**Lösung:**
1. Schalten Sie den Drucker aus und wieder an
2. Löschen Sie alte Bluetooth-Verbindungen zum Drucker
3. Paaren Sie Drucker erst mit Smartphone, dann lassen Sie ESP32 verbinden
4. Prüfen Sie Serial Monitor für Bluetooth-Status

---

## Firmware Update / Updating Firmware

Um die Firmware zu aktualisieren:

```bash
# Repository aktualisieren:
cd thermalprinter_bridge
git pull

# Neu kompilieren und hochladen:
pio run --target upload
```

---

## Konfiguration ändern / Changing Configuration

### WiFi-Passwort ändern

Bearbeiten Sie `src/main.cpp` Zeile 10:
```cpp
#define DEFAULT_PASSWORD "IhrNeuesPasswort123"
```

Dann neu kompilieren und hochladen.

### WiFi-SSID ändern

Bearbeiten Sie `src/main.cpp` Zeile 9:
```cpp
#define DEFAULT_SSID "MeinDrucker"
```

### In bestehendes WiFi einbinden

1. Öffnen Sie: http://192.168.4.1
2. Scrollen Sie zu "Configuration"
3. Geben Sie Ihr WiFi-SSID und Passwort ein
4. Setzen Sie "AP Mode" auf `false`
5. Speichern und ESP32 neu starten
6. Der ESP32 verbindet sich nun mit Ihrem WiFi
7. Neue IP-Adresse finden Sie im Serial Monitor

---

## Nächste Schritte / Next Steps

✅ **Installation abgeschlossen!**

**Jetzt können Sie:**
- Text drucken über Web-Interface
- Bilder drucken über Web-Interface
- Python-Skripte verwenden (siehe `examples/`)
- REST API nutzen

**Dokumentation:**
- `README.md` - Vollständige Anleitung
- `QUICKSTART.md` - Schnellstart
- `examples/README.md` - Beispiele und Skripte

**Support:**
Bei Fragen öffnen Sie ein Issue auf GitHub:
https://github.com/jfeldmaier/thermalprinter_bridge/issues

---

## Hardware-Verbindung

**ESP32 Pin-Belegung:** (für Referenz, nicht erforderlich bei Bluetooth)

Der Gateway nutzt **nur Bluetooth** - keine physischen Verbindungen zum Drucker nötig!

**Stromversorgung:**
- ESP32: 5V über USB (während Entwicklung)
- ESP32: 5V externes Netzteil (für Dauerbetrieb empfohlen)
- JK-5803P: Eingebauter Akku oder externes Netzteil

---

**Viel Erfolg! / Good Luck! 🚀**

Bei Problemen: https://github.com/jfeldmaier/thermalprinter_bridge/issues
