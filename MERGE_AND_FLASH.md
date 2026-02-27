# Merge zu Master - Checkliste und Anleitung

## Vorbereitung für Merge

### 1. Überprüfung vor dem Merge

**Branch Status:**
```bash
# Aktueller Branch
git status

# Alle Commits anzeigen
git log --oneline -10

# Unterschiede zu main/master
git diff main..HEAD  # oder master..HEAD
```

**Erwartete Commits:**
- Image printing feature implementation
- Documentation updates
- Installation guides
- ESP32 DevKit V1 specific docs

### 2. Merge zu Master durchführen

Der Merge wird typischerweise über GitHub Pull Request gemacht, aber hier die Git-Befehle:

**Option A: Via GitHub (Empfohlen)**

1. Pull Request ist bereits erstellt auf GitHub
2. Gehen Sie zu: https://github.com/jfeldmaier/thermalprinter_bridge/pulls
3. Öffnen Sie den PR "Add image printing support via ESC/POS bitmap commands"
4. Klicken Sie auf "Merge pull request"
5. Bestätigen Sie den Merge
6. Optional: Branch löschen nach dem Merge

**Option B: Lokal mit Git (Manuell)**

```bash
# Sicherstellen dass alles committed ist
git status

# Zum Master-Branch wechseln
git checkout main  # oder master

# Aktualisieren
git pull origin main

# Merge durchführen
git merge copilot/build-bluetooth-printer-gateway

# Zum Remote pushen
git push origin main
```

### 3. Nach dem Merge: Projekt auf ESP32 DevKit V1 flashen

**Schritt 1: Neuen Master-Branch auschecken**
```bash
# Zum Master wechseln
git checkout main  # oder master

# Aktualisieren
git pull origin main

# Überprüfen dass alle Änderungen da sind
ls -la
cat platformio.ini
```

**Schritt 2: ESP32 DevKit V1 vorbereiten**
```bash
# CP2102 Treiber überprüfen (falls nicht installiert)
# Windows: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

# ESP32 per USB verbinden
# Port überprüfen
pio device list
```

**Schritt 3: Kompilieren und Flashen**
```bash
# Projekt bauen
pio run

# Auf ESP32 DevKit V1 flashen
pio run --target upload

# Bei Problemen (BOOT-Button drücken):
pio run --target upload --upload-resetmethod nodemcu
```

**Schritt 4: Serial Monitor öffnen**
```bash
pio device monitor
```

### 4. Erste Tests nach dem Flash

**Test 1: WiFi Access Point**
```
Erwartete Ausgabe im Serial Monitor:
- "Access Point started successfully"
- "SSID: ThermalPrinter_GW"
- "IP Address: 192.168.4.1"
```

Aktion:
- Smartphone WiFi öffnen
- "ThermalPrinter_GW" suchen
- Mit Passwort "Print$2026!" verbinden

**Test 2: Web-Interface**
```
Im Browser öffnen: http://192.168.4.1
```

Erwartung:
- Gateway-Oberfläche erscheint
- Status-Anzeige sichtbar
- Text-Eingabefeld sichtbar
- Bild-Upload-Bereich sichtbar

**Test 3: Bluetooth-Verbindung**
```
JK-5803P Drucker einschalten
```

Erwartung im Serial Monitor:
- "Bluetooth connected!"

Erwartung im Web-Interface:
- Status: "Bluetooth: ✅ Connected"

**Test 4: Text drucken**
```
Im Web-Interface:
1. Text eingeben: "Test vom ESP32 DevKit V1"
2. "Send to Printer" klicken
```

Erwartung:
- Text wird auf Drucker ausgegeben
- Success-Meldung im Browser

**Test 5: Bild drucken**
```
Im Web-Interface:
1. Zum "Print Image" Bereich scrollen
2. Bild auswählen (z.B. Logo, QR-Code)
3. Preview ansehen
4. "Print Image" klicken
```

Erwartung:
- Bild wird als schwarz-weiß Preview angezeigt
- Bild wird auf Drucker ausgegeben

### 5. Produktiv-Konfiguration (Optional)

**In bestehendes WiFi einbinden:**

Web-Interface öffnen → Configuration:
```json
{
  "ssid": "IhrWiFiName",
  "password": "IhrWiFiPasswort",
  "apMode": false
}
```

Speichern und ESP32 neu starten.

Neue IP-Adresse im Serial Monitor ablesen:
```
Connected to WiFi!
IP Address: 192.168.1.XXX
```

### 6. Erfolgs-Bestätigung

✅ **Checkliste:**
- [ ] Code auf Master gemerged
- [ ] ESP32 DevKit V1 erfolgreich geflashed
- [ ] WiFi Access Point funktioniert
- [ ] Web-Interface erreichbar
- [ ] Bluetooth zu JK-5803P verbunden
- [ ] Text-Druck funktioniert
- [ ] Bild-Druck funktioniert
- [ ] Serial Monitor zeigt keine Fehler

### 7. Dokumentation nach Merge

**Aktualisierte Dateien auf Master:**
```
thermalprinter_bridge/
├── src/main.cpp                    # Hauptfirmware mit Image-Support
├── platformio.ini                  # ESP32 DevKit V1 optimiert
├── README.md                       # Vollständige Dokumentation
├── INSTALLATION_DE.md              # Deutsche Installation
├── INSTALLATION_QUICKREF.md        # Schnellreferenz
├── FLASH_ESP32_DEVKITV1.md        # DevKit V1 spezifisch ⭐ NEU
├── IMPLEMENTATION.md               # Technische Details
├── QUICKSTART.md                   # Schnellstart Nutzung
├── PROJECT_SUMMARY.md              # Projekt-Übersicht
└── examples/
    ├── print_image.py              # Python Bild-Druck
    ├── print_client.py             # Python Text-Druck
    ├── test_image_printing.html    # HTML Test-Tool
    └── README.md                   # Beispiele-Dokumentation
```

### 8. Nächste Schritte nach erfolgreichem Flash

1. **Produktiv nehmen:**
   - ESP32 mit stabilem Netzteil versorgen
   - Optional: WiFi-Konfiguration anpassen
   - Optional: Gehäuse verwenden

2. **Weitere Features nutzen:**
   - Python-Skripte für automatische Drucke
   - REST API für Integration in andere Systeme
   - Batch-Druck von mehreren Bildern

3. **Bei Problemen:**
   - Serial Monitor Ausgabe prüfen
   - Dokumentation konsultieren
   - GitHub Issue erstellen

### Schnellbefehle Zusammenfassung

```bash
# 1. Nach Merge: Code aktualisieren
git checkout main && git pull

# 2. ESP32 DevKit V1 flashen
pio run --target upload

# 3. Monitor öffnen
pio device monitor

# 4. Test: WiFi verbinden
# SSID: ThermalPrinter_GW
# Pass: Print$2026!

# 5. Test: Browser
# http://192.168.4.1
```

---

## Problemlösungen beim Merge/Flash

| Problem | Lösung |
|---------|--------|
| Merge-Konflikt | Lokal auf main branch: `git merge --abort`, dann via GitHub PR mergen |
| Upload schlägt fehl | BOOT-Taste drücken, oder: `--upload-resetmethod nodemcu` |
| Port nicht gefunden | `pio device list`, Treiber prüfen |
| WiFi nicht sichtbar | 20 Sek. warten, RESET drücken |
| Web-Interface 404 | IP überprüfen (192.168.4.1), WiFi-Verbindung prüfen |
| Bluetooth verbindet nicht | Drucker aus/an, alte Paarungen löschen |

---

**Viel Erfolg beim Merge und Flash! 🚀**

Bei Fragen: GitHub Issues oder die erstellten Dokumentationen konsultieren.
