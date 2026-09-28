# AGENT HANDOFF – Thermodrucker Bridge (V1)

Diese Datei ist die technische Übergabe für einen nachfolgenden KI-Agenten.

## 1) Aktueller Status
Projekt ist funktionsfähig mit Web-UI, USB-Druck, Etiketten und Bilddruck.

Bereits umgesetzt:
- ESP32-S3 USB-Host Druckpfad (Bulk-OUT Endpoint)
- REST API + LittleFS statische UI
- UTF-8 → WPC1252 für Umlaute in Textdruck
- Web-Konfigurationsspeicherung (`POST /api/config`)
- Etikettenlayout mit konfigurierbaren Parametern aus `data/config.json`
- Fruchtauswahl inkl. Biene sowie Bild-basierter Himbeere/Brombeere

## 2) Wichtige Dateien
- Firmware: `src/main.cpp`
- WLAN-Zugangsdaten: `src/secrets.h` (lokal, nicht eingecheckt, Vorlage: `src/secrets.h.example`)
- Frontend: `data/index.html`
- Persistente Label-Konfig: `data/config.json`
- Symbolbilder: `data/himbeere.png`, `data/brombeere.png`
- Build-Konfig: `platformio.ini`

## 3) Firmware-Architektur (`src/main.cpp`)
### Kernmodule
1. Web-Log Ringbuffer
2. USB-Host Setup + Client/Lib Tasks
3. Druckpfad `sendToPrinter(...)`
4. Textdruck `printText(...)` (ESC/POS + WPC1252)
5. AsyncWebServer Endpunkte
6. TCP Raw Print Server (Port 9100) – `setupTcpPrintServer()` / `handleTcpPrintServer()`
7. mDNS Auto-Discovery – `setupMDNS()`

### USB-Zustand (global)
- `printerReady` ist die zentrale Runtime-Variable für Druckbereitschaft
- `bulkOutEpAddr`, `bulkOutMps`, `printerIfaceNum` werden beim Descriptor-Parsing gesetzt
- Mutex/Semaphoren:
  - `printerMutex` für serialisierten Druck (Web-API UND TCP-Server nutzen denselben Mutex)
  - `xferDoneSem` für Transfer-Abschluss

### TCP Print Server (Port 9100)
- `tcpPrintServer` (WiFiServer) lauscht auf Port 9100
- `tcpPrintClient` hält genau einen verbundenen Client
- `handleTcpPrintServer()` wird aus `loop()` alle 2ms non-blocking aufgerufen
- Chunk-Puffer `tcpChunkBuf` (4 KB, PSRAM) für TCP→USB Durchleitung
- Bei Druckerfehler wird der Client sofort getrennt
- Zweite gleichzeitige Verbindung wird abgelehnt

### mDNS
- Hostname: `thermodrucker.local`
- Services: `_http._tcp:80`, `_pdl-datastream._tcp:9100`
- TXT-Records: ty, product, pdl, usb_MFG, usb_MDL

### Text-Encoding
- `utf8ToWPC1252(...)` wandelt UTF-8 um
- Nicht darstellbare Zeichen (z. B. Emoji) werden durch `?` ersetzt
- ESC/POS Codepage wird in `printText(...)` via `ESC t 16` gesetzt

## 4) Web-API Vertrag
- `GET /status`
  - liefert `printerReady` (bool)
- `GET /log`
  - liefert laufende Logs
- `POST /printText`
  - Body: text/plain
- `POST /printImage`
  - Body: roher ESC/POS Byte-Stream
- `POST /api/config`
  - Body: JSON, wird 1:1 in `/config.json` gespeichert

## 5) Frontend-Architektur (`data/index.html`)
### Bereiche
- Live-Status
- Etikettenkarten
- Textdruck
- Bilddruck (Dithering, Brightness/Contrast/Scale)
- Einstellungen (`config.json` lesen/schreiben)
- Debug-Log

### Label Rendering
- Canvas-Rendering in Schwarz/Weiß
- `buildEscPosImage(...)` erzeugt GS-v-0 Rasterdaten
- Fruchtgrafik:
  - Emoji oder Custom-Canvas-Formen
  - Für Himbeere/Brombeere: PNG aus LittleFS

### Kritische Konsistenz-Regel
- Frontend muss für Status `printerReady` auswerten (nicht `printer_connected`)

## 6) Build/Flash Workflow
```bash
cd thermal_printerbridge_usb
cp src/secrets.h.example src/secrets.h   # einmalig, WIFI_SSID/WIFI_PASS eintragen
pio run -e esp32s3
pio run -t upload -e esp32s3
pio run -t uploadfs -e esp32s3
```

## 7) Bekannte Risiken / offene Punkte
1. `POST /api/config` validiert JSON derzeit nicht
2. Kein Auth-Schutz auf Druck-API im LAN
3. Frontend ist monolithisch (eine Datei), mittelfristig modularisieren

## 8) Nächste sinnvolle Schritte (Priorität)
1. JSON-Validierung und Default-Fallback bei Config-Write
2. WiFi-Setup über Captive Portal / AP-Fallback
3. Optionale PIN/Token Auth für Web/API
4. Label-Template Presets + Export/Import
5. E2E-Tests für API (mind. lokal mit Skript)

## 9) Arbeitsregeln für nachfolgenden Agenten
- Bei Änderungen an `data/*` immer `uploadfs` ausführen
- Bei Änderungen an `src/*` immer `upload` ausführen
- USB-Probleme zuerst mit `/log` analysieren
- Keine Breaking-Changes in API ohne README + Handoff Update

## 10) Version-Marker
Dieser Stand wurde als lokaler Git-Tag `v1.0.0` markiert.
