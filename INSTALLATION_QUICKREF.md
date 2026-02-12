# ESP32 Installation - Schnellreferenz / Quick Reference

## 🚀 Schnellstart (5 Minuten)

### 1. Software installieren
```bash
pip install platformio
```

### 2. Code herunterladen
```bash
git clone https://github.com/jfeldmaier/thermalprinter_bridge.git
cd thermalprinter_bridge
```

### 3. ESP32 per USB verbinden

### 4. Auf ESP32 flashen
```bash
pio run --target upload
```

### 5. Fertig! ✅

**WiFi:** ThermalPrinter_GW (Passwort: Print$2026!)  
**Web-Interface:** http://192.168.4.1

---

## ⚡ Häufige Befehle / Common Commands

```bash
# Kompilieren (ohne Upload)
pio run

# Upload auf ESP32
pio run --target upload

# Serial Monitor öffnen
pio device monitor

# Port angeben (bei Problemen)
pio run --target upload --upload-port COM3      # Windows
pio run --target upload --upload-port /dev/ttyUSB0  # Linux

# Verfügbare Ports anzeigen
pio device list

# Projekt neu kompilieren (Clean Build)
pio run --target clean
pio run
```

---

## 🔧 Problemlösungen / Quick Fixes

| Problem | Lösung |
|---------|--------|
| `pio: command not found` | `pip install platformio` neu ausführen |
| Upload schlägt fehl | BOOT-Taste drücken während Upload |
| Port nicht gefunden | `pio device list` → richtigen Port wählen |
| Treiber fehlt | CH340 oder CP2102 Treiber installieren |
| WiFi nicht sichtbar | 20 Sekunden warten, ESP32 RESET drücken |
| Bluetooth verbindet nicht | Drucker aus/an, alte Paarungen löschen |

---

## 📱 Nach der Installation

1. **Mit WiFi verbinden:**
   - SSID: `ThermalPrinter_GW`
   - Passwort: `Print$2026!`

2. **Web-Interface öffnen:**
   - URL: `http://192.168.4.1`

3. **Drucker paaren:**
   - JK-5803P einschalten
   - Automatische Verbindung abwarten
   - Status im Web-Interface prüfen

---

## 📚 Weitere Dokumentation

- `INSTALLATION_DE.md` - Ausführliche Installationsanleitung
- `README.md` - Vollständige Dokumentation
- `QUICKSTART.md` - Schnellstart für Nutzung
- `examples/` - Beispiel-Skripte

---

## 🆘 Hilfe

**GitHub Issues:** https://github.com/jfeldmaier/thermalprinter_bridge/issues

**Serial Monitor Ausgabe prüfen:**
```bash
pio device monitor
```

Sollte zeigen:
```
ESP32 Thermal Printer Gateway
Access Point started successfully
IP Address: 192.168.4.1
Setup complete!
```

---

**Viel Erfolg! 🎉**
