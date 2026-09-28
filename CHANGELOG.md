# Changelog

Alle relevanten Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format orientiert sich an Keep a Changelog und Semantic Versioning.

## [Unreleased]

### Added
- Platzhalter für neue Features

### Changed
- Platzhalter für Änderungen an vorhandenem Verhalten

### Fixed
- Platzhalter für Bugfixes

### Removed
- Platzhalter für entfernte Funktionen

---

## [1.1.0] - 2026-03-08

### Added
- TCP Raw Print Server auf Port 9100 (AppSocket / Raw Printing)
- mDNS Auto-Discovery mit Hostname `thermodrucker.local`
- mDNS Service `_pdl-datastream._tcp:9100` für OS-Druckererkennung
- mDNS Service `_http._tcp:80` für Webinterface-Discovery
- TXT-Records mit Drucker-Metadaten (Typ, Hersteller, Modell)
- Non-blocking TCP-Client-Handling in `loop()` (2ms Poll-Intervall)
- PSRAM-basierter 4 KB Chunk-Puffer für TCP→USB Durchleitung

### Changed
- `loop()` von idle-sleep auf aktives TCP-Polling umgestellt
- Datei-Header in main.cpp um Punkte 5+6 erweitert
- README, AGENT_HANDOFF und CHANGELOG aktualisiert

---

## [1.0.0] - 2026-03-08

### Added
- Erstes lauffähiges Release der ESP32-S3 USB-Thermodrucker-Bridge
- USB-Host-Anbindung für 58mm Thermodrucker (JK-5803P, Bulk-OUT)
- Webinterface aus LittleFS mit Status-, Label-, Text- und Bilddruckbereich
- Label-Workflow mit 3 Textzeilen, Symbolauswahl, Mengensteuerung und Vorschau
- Bilddruck mit Dithering (Floyd-Steinberg, Atkinson, Stucki, Bayer, Threshold)
- Regler für Helligkeit, Kontrast und Auflösung im Bilddruck
- Persistente Konfiguration über `data/config.json` + `POST /api/config`
- Symbolbilder für Himbeere und Brombeere aus LittleFS (`data/himbeere.png`, `data/brombeere.png`)
- Biene als zusätzliches auswählbares Symbol
- Web-Debug-Log über `GET /log`
- Projektdokumentation in `README.md` und `AGENT_HANDOFF.md`

### Changed
- UTF-8 Textpfad auf WPC1252-Ausgabe umgestellt für Umlaute
- Druckerstatus-Anzeige im Frontend auf `printerReady` angepasst
- Layout auf feste, zentrierte Maximalbreite für bessere Desktop-Darstellung angepasst

### Fixed
- Fehlerhafte Umlautausgabe beim Textdruck
- Frontend-Statusproblem (falsches JSON-Property ausgewertet)
- Mehrere UI-/Layout-Unstimmigkeiten aus der ersten Integrationsrunde

---

## Hinweise für künftige Releases

- Neue Einträge immer zuerst unter `## [Unreleased]` ergänzen.
- Beim Release den Block nach unten kopieren und mit Version + Datum versehen.
- Versionen im Format `MAJOR.MINOR.PATCH` führen (z. B. `1.1.0`, `1.1.1`, `2.0.0`).
