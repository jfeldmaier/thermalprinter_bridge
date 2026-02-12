#!/bin/bash
# Diagnose-Script für ESP32 Thermal Printer Gateway
# Diagnostic script for ESP32 Thermal Printer Gateway

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ESP32 Thermal Printer Gateway - System Diagnose           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Farben für bessere Lesbarkeit
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 1. Aktuelles Verzeichnis
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1️⃣  Aktuelles Verzeichnis / Current Directory"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
pwd
echo ""

# 2. Git Branch
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2️⃣  Git Branch"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
git branch 2>/dev/null || echo -e "${RED}❌ Git nicht verfügbar${NC}"
echo ""

# 3. Git Status
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3️⃣  Git Status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
git status -s 2>/dev/null || echo -e "${RED}❌ Git nicht verfügbar${NC}"
echo ""

# 4. Wichtige Dateien
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4️⃣  Wichtige Projekt-Dateien / Important Project Files"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✅${NC} $1"
    else
        echo -e "${RED}❌${NC} $1 ${RED}(FEHLT!)${NC}"
    fi
}

check_dir() {
    if [ -d "$1" ]; then
        echo -e "${GREEN}✅${NC} $1/"
    else
        echo -e "${RED}❌${NC} $1/ ${RED}(FEHLT!)${NC}"
    fi
}

check_file "platformio.ini"
check_file "README.md"
check_dir "src"
check_dir "examples"
check_file "src/main.cpp"
echo ""

# 5. Python Installation
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5️⃣  Python Installation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version 2>&1)
    echo -e "${GREEN}✅${NC} Python3: $PYTHON_VERSION"
else
    echo -e "${RED}❌ Python3 nicht gefunden!${NC}"
    echo -e "${YELLOW}   Installation: https://www.python.org/downloads/${NC}"
fi

if command -v pip3 &> /dev/null; then
    PIP_VERSION=$(pip3 --version 2>&1)
    echo -e "${GREEN}✅${NC} pip3: $PIP_VERSION"
else
    echo -e "${RED}❌ pip3 nicht gefunden!${NC}"
fi
echo ""

# 6. PlatformIO Installation
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6️⃣  PlatformIO Installation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if command -v pio &> /dev/null; then
    PIO_VERSION=$(pio --version 2>&1)
    echo -e "${GREEN}✅${NC} PlatformIO: $PIO_VERSION"
else
    echo -e "${RED}❌ PlatformIO (pio) nicht gefunden!${NC}"
    echo -e "${YELLOW}   Installation: pip3 install platformio${NC}"
    echo -e "${YELLOW}   Dann ggf. PATH aktualisieren${NC}"
fi
echo ""

# 7. USB Geräte / Serial Ports
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7️⃣  USB Geräte / Serial Ports"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if command -v pio &> /dev/null; then
    pio device list
else
    # Fallback für verschiedene Betriebssysteme
    if [ -d "/dev/" ]; then
        if ls /dev/ttyUSB* 2>/dev/null; then
            echo -e "${GREEN}✅${NC} USB Geräte gefunden:"
            ls -l /dev/ttyUSB* 2>/dev/null
        elif ls /dev/cu.usbserial* 2>/dev/null; then
            echo -e "${GREEN}✅${NC} USB Geräte gefunden:"
            ls -l /dev/cu.usbserial* 2>/dev/null
        else
            echo -e "${YELLOW}⚠️  Keine USB-Geräte gefunden${NC}"
            echo "   ESP32 angeschlossen?"
        fi
    fi
fi
echo ""

# 8. Zusammenfassung und Empfehlungen
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "8️⃣  Zusammenfassung & nächste Schritte"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

ERRORS=0

# Prüfe platformio.ini
if [ ! -f "platformio.ini" ]; then
    echo -e "${RED}❌ PROBLEM: platformio.ini fehlt!${NC}"
    echo "   Lösung: cd zum richtigen Verzeichnis"
    echo "   Oder: git checkout copilot/build-bluetooth-printer-gateway"
    ERRORS=$((ERRORS + 1))
fi

# Prüfe PlatformIO
if ! command -v pio &> /dev/null; then
    echo -e "${RED}❌ PROBLEM: PlatformIO nicht installiert!${NC}"
    echo "   Lösung: pip3 install platformio"
    echo "   Dann: export PATH=\$PATH:~/.platformio/penv/bin"
    ERRORS=$((ERRORS + 1))
fi

# Prüfe Python
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}❌ PROBLEM: Python3 nicht installiert!${NC}"
    echo "   Lösung: https://www.python.org/downloads/"
    ERRORS=$((ERRORS + 1))
fi

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✅ System bereit für ESP32 Flash!${NC}"
    echo ""
    echo "Nächste Schritte:"
    echo "  1. ESP32 DevKit V1 per USB verbinden"
    echo "  2. pio run --target upload"
    echo ""
else
    echo -e "${YELLOW}⚠️  $ERRORS Problem(e) gefunden - siehe oben${NC}"
    echo ""
    echo "Weitere Hilfe:"
    echo "  • TROUBLESHOOTING.md"
    echo "  • INSTALLATION_DE.md"
    echo "  • FLASH_ESP32_DEVKITV1.md"
    echo ""
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Diagnose abgeschlossen / Diagnostics complete"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
