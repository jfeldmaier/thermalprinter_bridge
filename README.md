# ESP32-S3 USB Thermodrucker Bridge (Version 1)

Lokale Web-basierte Druck-Bridge für einen 58mm USB-Thermodrucker (JK-5803P) auf ESP32-S3.

## Ziel
- Drucker per USB-Host direkt am ESP32-S3 ansteuern
- Weboberfläche für Text-, Bild- und Etikettendruck
- Keine Cloud-Abhängigkeit, alles lokal im WLAN

## Features (Stand V1)
- USB-Host Drucker-Anbindung (Bulk-OUT)
- Webinterface aus LittleFS (`data/index.html`)
- REST-API für Status, Textdruck, Bilddruck, Config-Update
- **TCP Raw Print Server auf Port 9100** (AppSocket / Raw Printing)
- **mDNS Auto-Discovery** (`thermodrucker.local`, Bonjour / ZeroConf)
- Etikettenmodus mit:
  - 3 Textzeilen
  - Fruchtsymbol-Auswahl
  - Anzahl + Vorschub/Gap
  - Schriftwahl
- Bilddruck mit Dithering + Reglern:
  - Helligkeit
  - Kontrast
  - Auflösung (Skalierung)
- Umlaute-Fix (UTF-8 → Windows-1252)
- Web-Debug-Log (`/log`)

## Hardware / Anschlüsse
- Board: ESP32-S3 DevKitC-1 (8MB Flash, 2MB PSRAM)
- Drucker: JK-5803P (USB)
- Wichtig: USB-OTG Port des ESP32-S3 für den Drucker nutzen

## Projektstruktur
- `src/main.cpp` Firmware (WiFi, USB Host, Webserver, REST)
- `src/secrets.h` WLAN-Zugangsdaten (lokal, nicht eingecheckt)
- `src/secrets.h.example` Vorlage für `secrets.h`
- `data/index.html` Frontend (UI + Canvas-Verarbeitung)
- `data/config.json` Laufzeit-Konfiguration fürs Etikettenlayout
- `data/himbeere.png`, `data/brombeere.png` Symbolgrafiken
- `platformio.ini` Build/Upload-Konfiguration

## Build & Upload
Voraussetzung: PlatformIO installiert.

Vor dem ersten Build WLAN-Zugangsdaten hinterlegen:
```bash
cp src/secrets.h.example src/secrets.h
# src/secrets.h editieren und WIFI_SSID / WIFI_PASS eintragen
```
`src/secrets.h` ist in `.gitignore` und wird nicht mit veröffentlicht.

```bash
cd thermal_printerbridge_usb
pio run -e esp32s3
pio run -t upload -e esp32s3
pio run -t uploadfs -e esp32s3
```

## Laufzeit / Zugriff
- Gerät verbindet sich mit dem in `src/main.cpp` hinterlegten WLAN
- Webserver läuft auf Port 80
- IP-Adresse im Log oder Router nachsehen

## REST API
- `GET /status` → JSON mit `printerReady`, `ip`, `rssi`, `uptime`, `freeHeap`, `psram`
- `GET /log` → Debug-Log (Text)
- `POST /printText` → Text drucken (`text/plain`)
- `POST /printImage` → ESC/POS-Bytestream (`application/octet-stream`)
- `POST /api/config` → `config.json` aktualisieren (JSON)

## TCP Print Server (Port 9100)
Der ESP32 lauscht auf Port 9100 als Raw/AppSocket-Druckserver.
Beliebige ESC/POS-Daten können direkt per TCP an den Drucker gesendet werden:

```bash
# Test mit netcat:
echo "Hello Printer" | nc thermodrucker.local 9100

# Oder per IP:
cat label.bin | nc 192.168.1.237 9100
```

## mDNS / Bonjour
Der Drucker meldet sich im Netzwerk als `thermodrucker.local` mit:
- `_http._tcp` (Port 80) → Webinterface
- `_pdl-datastream._tcp` (Port 9100) → Raw Print / AppSocket

macOS, Windows und Linux können den Drucker automatisch über „Drucker hinzufügen" finden.

## Hinweise
- Nach Frontend-Änderungen immer `uploadfs` ausführen
- Nach Firmware-Änderungen `upload` ausführen
- Browser bei UI-Problemen hart neu laden (Cache)

## Versionierung
Dieser Stand ist als **Version 1** lokal im Git gespeichert und getaggt.
