# Problemlösung: Dateien nicht sichtbar & PIO funktioniert nicht

## Problem 1: Dateien sind nicht im main Branch sichtbar

### Ursache
Die Dateien befinden sich aktuell nur im Feature Branch `copilot/build-bluetooth-printer-gateway`. Der main/master Branch existiert noch nicht oder wurde noch nicht aktualisiert.

### Lösung

**Option A: Direkt aus dem Feature Branch arbeiten (Empfohlen für Start)**

Sie können direkt aus dem aktuellen Feature Branch arbeiten, ohne erst zu mergen:

```bash
# 1. Sicherstellen dass Sie im richtigen Branch sind
git branch
# Sollte zeigen: * copilot/build-bluetooth-printer-gateway

# 2. Überprüfen dass alle Dateien da sind
ls -la
# Sie sollten sehen: platformio.ini, src/, examples/, README.md, etc.

# 3. Mit PlatformIO fortfahren (siehe Problem 2)
```

**Option B: Main Branch erstellen und mergen**

Falls Sie einen main Branch erstellen und dorthin mergen möchten:

```bash
# 1. Main Branch erstellen (falls nicht existent)
git checkout -b main

# 2. Feature Branch mergen
git merge copilot/build-bluetooth-printer-gateway

# 3. Zum Remote pushen (optional)
git push -u origin main
```

**Option C: Über GitHub Pull Request mergen**

Falls ein GitHub Repository existiert:
1. Gehen Sie zu: https://github.com/jfeldmaier/thermalprinter_bridge
2. Erstellen Sie einen Pull Request von `copilot/build-bluetooth-printer-gateway` nach `main`
3. Mergen Sie den Pull Request
4. Lokal aktualisieren:
```bash
git checkout main
git pull origin main
```

---

## Problem 2: PlatformIO (pio) Kommando funktioniert nicht

### Ursache
PlatformIO ist nicht installiert oder nicht im PATH verfügbar.

### Lösung - Schritt für Schritt

#### Schritt 1: Python überprüfen

```bash
# Python Version prüfen (muss 3.7 oder höher sein)
python --version
# oder
python3 --version

# Falls Python fehlt:
# Windows: https://www.python.org/downloads/
# macOS: brew install python3
# Linux: sudo apt install python3 python3-pip
```

#### Schritt 2: PlatformIO installieren

```bash
# Mit pip installieren
pip install platformio

# ODER mit pip3 (falls pip nicht funktioniert)
pip3 install platformio

# ODER als User installieren (ohne sudo)
pip install --user platformio
```

#### Schritt 3: PATH überprüfen und ggf. aktualisieren

**Linux/macOS:**
```bash
# PlatformIO zum PATH hinzufügen
export PATH=$PATH:~/.platformio/penv/bin

# Dauerhaft in .bashrc oder .zshrc eintragen:
echo 'export PATH=$PATH:~/.platformio/penv/bin' >> ~/.bashrc
source ~/.bashrc
```

**Windows:**
```bash
# In CMD/PowerShell:
# PlatformIO ist meist automatisch im PATH
# Falls nicht, fügen Sie manuell hinzu:
# C:\Users\IhrUsername\.platformio\penv\Scripts
```

#### Schritt 4: Installation überprüfen

```bash
# PlatformIO Version prüfen
pio --version

# Sollte zeigen: PlatformIO Core, version X.X.X
```

#### Schritt 5: Neues Terminal öffnen

```bash
# Manchmal hilft es, ein neues Terminal-Fenster zu öffnen
# damit die PATH-Änderungen wirksam werden
```

---

## Komplette Schritt-für-Schritt Anleitung

### 1. Repository-Zustand prüfen

```bash
# Aktuelles Verzeichnis anzeigen
pwd

# Branch anzeigen
git branch

# Dateien auflisten
ls -la

# Erwartete Ausgabe:
# - platformio.ini
# - src/
# - examples/
# - README.md
# - etc.
```

**Falls Dateien fehlen:**
```bash
# Ins richtige Verzeichnis wechseln
cd /pfad/zum/thermalprinter_bridge

# Falls Repository noch nicht geklont:
git clone https://github.com/jfeldmaier/thermalprinter_bridge.git
cd thermalprinter_bridge

# Zum Feature Branch wechseln
git checkout copilot/build-bluetooth-printer-gateway
```

### 2. PlatformIO installieren

```bash
# Python überprüfen
python3 --version

# PlatformIO installieren
pip3 install platformio

# Installation überprüfen
pio --version
```

### 3. ESP32 Plattform installieren

```bash
# Im Projektverzeichnis (wo platformio.ini ist)
pio platform install espressif32
```

### 4. Projekt kompilieren (Test)

```bash
# Projekt bauen
pio run

# Erwartete Ausgabe:
# - Platform wird heruntergeladen (beim ersten Mal)
# - Bibliotheken werden installiert
# - Code wird kompiliert
# - "SUCCESS" am Ende
```

### 5. ESP32 DevKit V1 anschließen

```bash
# ESP32 per USB verbinden

# Verfügbare Ports anzeigen
pio device list

# Erwartete Ausgabe:
# Windows: COM3, COM4, etc.
# Linux: /dev/ttyUSB0, /dev/ttyUSB1
# macOS: /dev/cu.usbserial-*
```

### 6. Auf ESP32 flashen

```bash
# Automatischer Upload
pio run --target upload

# ODER mit spezifischem Port
pio run --target upload --upload-port COM3  # Windows
pio run --target upload --upload-port /dev/ttyUSB0  # Linux

# ODER mit BOOT-Button Methode (falls Upload hängt)
pio run --target upload --upload-resetmethod nodemcu
```

### 7. Serial Monitor öffnen

```bash
# Monitor starten
pio device monitor

# Erwartete Ausgabe:
# ESP32 Thermal Printer Gateway
# WiFi: ThermalPrinter_GW
# IP: 192.168.4.1
# Setup complete!

# Beenden mit: Ctrl+C
```

---

## Häufige Fehler und Lösungen

### Fehler: "command not found: pio"

**Lösung:**
```bash
# Python und pip neu installieren
sudo apt install python3-pip  # Linux
brew install python3          # macOS

# PlatformIO neu installieren
pip3 install --user platformio

# PATH aktualisieren
export PATH=$PATH:~/.local/bin
```

### Fehler: "Permission denied" beim Upload

**Lösung (Linux):**
```bash
# User zu dialout Gruppe hinzufügen
sudo usermod -a -G dialout $USER

# Neu anmelden (wichtig!)
# Oder:
sudo chmod 666 /dev/ttyUSB0
```

### Fehler: "Could not open port"

**Lösungen:**
1. Schließen Sie Arduino IDE oder andere Serial-Programme
2. Trennen und verbinden Sie USB erneut
3. Installieren Sie USB-Treiber:
   - CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
   - CH340: https://www.wch-ic.com/downloads/CH341SER_ZIP.html

### Fehler: "A fatal error occurred: Failed to connect"

**Lösungen:**
1. Drücken Sie BOOT-Taste am ESP32 während Upload
2. Verwenden Sie: `pio run --target upload --upload-resetmethod nodemcu`
3. Versuchen Sie niedrigere Upload-Geschwindigkeit:
   ```bash
   pio run --target upload --upload-speed 115200
   ```

### Fehler: Repository nicht auf main Branch

**Lösung:**
```bash
# Überprüfen Sie aktuelle Branch
git branch -a

# Falls nur Feature Branch existiert, arbeiten Sie direkt damit:
git checkout copilot/build-bluetooth-printer-gateway

# ODER erstellen Sie main Branch:
git checkout -b main
git merge copilot/build-bluetooth-printer-gateway
```

---

## Schnell-Diagnose Script

Führen Sie dieses Script aus um Ihr System zu überprüfen:

```bash
#!/bin/bash
echo "=== System-Diagnose ==="
echo ""

echo "1. Aktuelles Verzeichnis:"
pwd
echo ""

echo "2. Git Branch:"
git branch
echo ""

echo "3. Dateien im Verzeichnis:"
ls -la | head -20
echo ""

echo "4. Python Version:"
python3 --version
echo ""

echo "5. PlatformIO Installation:"
which pio
pio --version 2>/dev/null || echo "PlatformIO nicht gefunden!"
echo ""

echo "6. USB Geräte (Linux):"
ls -la /dev/ttyUSB* 2>/dev/null || echo "Keine USB-Geräte gefunden"
echo ""

echo "7. platformio.ini vorhanden?"
if [ -f "platformio.ini" ]; then
    echo "✅ platformio.ini gefunden"
    cat platformio.ini
else
    echo "❌ platformio.ini nicht gefunden!"
    echo "Hinweis: Sind Sie im richtigen Verzeichnis?"
fi
```

Speichern Sie dies als `diagnose.sh`, machen Sie es ausführbar und führen Sie es aus:

```bash
chmod +x diagnose.sh
./diagnose.sh
```

---

## Zusammenfassung: Was Sie jetzt tun sollten

1. **Überprüfen Sie Ihr aktuelles Verzeichnis:**
   ```bash
   pwd
   ls -la
   ```
   Sie müssen im `thermalprinter_bridge` Verzeichnis sein.

2. **Überprüfen Sie den Branch:**
   ```bash
   git branch
   ```
   Sie sollten auf `copilot/build-bluetooth-printer-gateway` sein.

3. **Installieren Sie PlatformIO:**
   ```bash
   pip3 install platformio
   pio --version
   ```

4. **ESP32 anschließen und flashen:**
   ```bash
   pio run --target upload
   ```

---

## Kontakt für weitere Hilfe

Falls weiterhin Probleme auftreten, bitte folgende Informationen bereitstellen:

1. Ausgabe von: `git branch -a`
2. Ausgabe von: `ls -la`
3. Ausgabe von: `pio --version` (oder Fehlermeldung)
4. Betriebssystem (Windows/macOS/Linux)
5. Genaue Fehlermeldung

**Viel Erfolg! 🚀**
