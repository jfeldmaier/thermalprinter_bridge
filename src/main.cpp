// =============================================================================
//  ESP32-S3  –  USB-Host Thermodrucker-Bridge  (JK-5803P / 58 mm POS)
//  Datei: src/main.cpp
// =============================================================================
//
//  Aufbau:
//    1.  WLAN-Verbindung (versteckte SSID)
//    2.  LittleFS  →  statische Web-Dateien
//    3.  ESPAsyncWebServer  →  REST-API + Webinterface
//    4.  USB-Host  →  Bulk-Transfer an den Thermodrucker
//    5.  TCP Raw Print Server (Port 9100)  →  AppSocket / Raw Printing
//    6.  mDNS Auto-Discovery  →  Bonjour / ZeroConf
//
//  Der USB-OTG-Port (GPIO 19/20) wird im Host-Modus betrieben.
//  Der Serial-Monitor läuft über die UART-Bridge (zweite USB-Buchse
//  des DevKitC-1 bzw. externe USB-UART-Bridge an UART0).
//
//  Sperrmechanismus:
//    Sowohl Web-API als auch TCP-Server greifen auf den USB-Drucker zu.
//    Der bestehende printerMutex serialisiert alle Druckaufträge.
//    sendToPrinter() holt den Mutex, der TCP-Handler ebenso.
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

// ESP-IDF USB-Host-Treiber  –  kommt mit dem Arduino-ESP32-Core
#include "usb/usb_host.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Web-Debug-Log  (Ringpuffer)
// ─────────────────────────────────────────────────────────────────────────────
//  Ersetzt Serial-Output für den Fall, dass kein UART-Port verfügbar ist.
//  Alle Log-Nachrichten werden in einem 8 KB Ringpuffer gesammelt und
//  können über  GET /log  im Browser abgerufen werden.
// ─────────────────────────────────────────────────────────────────────────────

#define WEBLOG_SIZE  8192
static char   webLogBuf[WEBLOG_SIZE];
static size_t webLogPos = 0;
static SemaphoreHandle_t webLogMutex = NULL;

// Schreibt formatierte Nachricht in den Ringpuffer UND auf Serial.
static void webLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void webLog(const char *fmt, ...) {
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    if (len <= 0) return;
    if ((size_t)len >= sizeof(tmp)) len = sizeof(tmp) - 1;

    // Auch auf Serial ausgeben (falls UART angeschlossen)
    Serial.write((const uint8_t *)tmp, len);

    // In den Ringpuffer schreiben (thread-safe)
    if (webLogMutex && xSemaphoreTake(webLogMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (int i = 0; i < len; i++) {
            webLogBuf[webLogPos % WEBLOG_SIZE] = tmp[i];
            webLogPos++;
        }
        xSemaphoreGive(webLogMutex);
    }
}

// Gibt den Ringpuffer als String zurück (älteste Einträge zuerst).
static String webLogRead() {
    String out;
    if (!webLogMutex) return out;
    if (xSemaphoreTake(webLogMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (webLogPos <= WEBLOG_SIZE) {
            out = String(webLogBuf, webLogPos);
        } else {
            // Ringpuffer hat sich gewrappt
            size_t start = webLogPos % WEBLOG_SIZE;
            out.reserve(WEBLOG_SIZE);
            out += String(webLogBuf + start, WEBLOG_SIZE - start);
            out += String(webLogBuf, start);
        }
        xSemaphoreGive(webLogMutex);
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Konstanten
// ─────────────────────────────────────────────────────────────────────────────

// WLAN-Zugangsdaten (versteckte SSID) – siehe src/secrets.h (nicht eingecheckt).
// Vorlage: src/secrets.h.example nach src/secrets.h kopieren und anpassen.
#include "secrets.h"

// Maximale Größe des USB-Bulk-Transfer-Puffers (Bytes)
// Muss ein Vielfaches der Bulk-MaxPacketSize (64 B bei Full-Speed) sein.
#define USB_XFER_BUF_SIZE  4096

// FreeRTOS Task-Prioritäten
#define USB_LIB_TASK_PRIO   5
#define USB_CLIENT_TASK_PRIO 5

// ─────────────────────────────────────────────────────────────────────────────
//  Globale Objekte
// ─────────────────────────────────────────────────────────────────────────────

// Asynchroner Webserver auf Port 80
AsyncWebServer webServer(80);

// TCP Raw Print Server auf Port 9100 (AppSocket / Raw Printing)
static WiFiServer tcpPrintServer(9100);
static WiFiClient tcpPrintClient;

// mDNS Hostname
static const char *MDNS_HOSTNAME = "thermodrucker";

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  globaler Zustand
// ─────────────────────────────────────────────────────────────────────────────

static usb_host_client_handle_t usbClientHdl  = NULL;   // registrierter Client
static usb_device_handle_t      printerDevHdl = NULL;   // geöffnetes Gerät
static usb_transfer_t          *xferOut       = NULL;   // wiederverwendbarer Transfer

static uint8_t  bulkOutEpAddr   = 0;      // Adresse d. Bulk-OUT-Endpoints
static uint16_t bulkOutMps      = 0;      // MaxPacketSize des Endpoints
static uint8_t  printerIfaceNum = 0;      // Interface-Nummer des Druckers
static volatile bool printerReady = false; // true sobald der Drucker nutzbar ist

// Semaphore: wird im Transfer-Callback freigegeben
static SemaphoreHandle_t xferDoneSem  = NULL;
// Mutex: serialisiert gleichzeitige Druckaufträge
static SemaphoreHandle_t printerMutex = NULL;

// Puffer für den /printImage Body (wird in PSRAM alloziert)
static uint8_t *imgBodyBuf    = nullptr;
static size_t   imgBodyLen    = 0;

// Puffer für den /printText Body
static String   textBodyBuf;

// Puffer für den POST /api/config Body
static String   configBodyBuf;

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Transfer-Callback
// ─────────────────────────────────────────────────────────────────────────────
//  Wird vom USB-Host-Stack aufgerufen, sobald ein Bulk-Transfer abgeschlossen
//  (oder fehlgeschlagen) ist.  Gibt das Semaphore frei, damit sendToPrinter()
//  fortfahren kann.
// ─────────────────────────────────────────────────────────────────────────────

static void IRAM_ATTR xferCallback(usb_transfer_t *transfer)
{
    // Wir geben das Semaphore frei – der wartende Task prüft transfer->status.
    xSemaphoreGive(xferDoneSem);
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Daten an den Drucker senden
// ─────────────────────────────────────────────────────────────────────────────
//  Teilt die Daten in Chunks von max. USB_XFER_BUF_SIZE auf und sendet
//  sie blockierend über den USB Bulk-OUT-Endpoint.
//
//  Ablauf pro Chunk:
//    1.  Nutzdaten in den Transfer-Puffer kopieren
//    2.  Transfer-Felder setzen  (Gerät, EP-Adresse, Länge, Callback)
//    3.  Transfer an den USB-Stack übergeben  (usb_host_transfer_submit)
//    4.  Auf Semaphore warten  (= Transfer abgeschlossen)
//    5.  Status prüfen
// ─────────────────────────────────────────────────────────────────────────────

static bool sendToPrinter(const uint8_t *data, size_t len)
{
    if (!printerReady || !xferOut || !printerDevHdl) return false;

    // Exklusiver Zugriff auf den Drucker
    if (xSemaphoreTake(printerMutex, pdMS_TO_TICKS(10000)) != pdTRUE) {
        webLog("[USB] Timeout beim Warten auf Drucker-Mutex\n");
        return false;
    }

    bool ok = true;
    size_t offset = 0;

    while (offset < len) {
        // Chunk-Größe bestimmen
        size_t chunk = len - offset;
        if (chunk > USB_XFER_BUF_SIZE) chunk = USB_XFER_BUF_SIZE;

        // ── Transfer vorbereiten ──
        memcpy(xferOut->data_buffer, data + offset, chunk);
        xferOut->num_bytes          = chunk;
        xferOut->device_handle      = printerDevHdl;
        xferOut->bEndpointAddress   = bulkOutEpAddr;
        xferOut->callback           = xferCallback;
        xferOut->context            = NULL;

        // ── Transfer absenden ──
        //    usb_host_transfer_submit() reiht den Transfer in die USB-HW-Queue
        //    ein.  Bei Erfolg wird der Callback aufgerufen.
        esp_err_t err = usb_host_transfer_submit(xferOut);
        if (err != ESP_OK) {
            webLog("[USB] transfer_submit Fehler: %s\n", esp_err_to_name(err));
            ok = false;
            break;
        }

        // ── Auf Abschluss warten (max. 5 s) ──
        if (xSemaphoreTake(xferDoneSem, pdMS_TO_TICKS(5000)) != pdTRUE) {
            webLog("[USB] Transfer-Timeout!\n");
            ok = false;
            break;
        }

        // ── Status prüfen ──
        if (xferOut->status != USB_TRANSFER_STATUS_COMPLETED) {
            webLog("[USB] Transfer-Status != COMPLETED  (%d)\n",
                          xferOut->status);
            ok = false;
            break;
        }

        offset += chunk;
    }

    xSemaphoreGive(printerMutex);
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  einfachen Text drucken  (ESC/POS)
// ─────────────────────────────────────────────────────────────────────────────
//  Baut folgende Kommando-Sequenz:
//      ESC @          →  Drucker initialisieren
//      ESC t 0        →  Code-Page 0 (PC437) setzen
//      <Text-Bytes>   →  Nutzdaten
//      LF             →  Zeilenumbruch
//      ESC d 3        →  3 Leerzeilen Vorschub
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
//  UTF-8 → WPC1252 Konvertierung
// ─────────────────────────────────────────────────────────────────────────────
//  WPC1252 (Windows-1252) enthält alle westeuropäischen Zeichen inkl.
//  ä ö ü Ä Ö Ü ß é è ê usw.  Für Codepoints U+0080..U+00FF stimmen
//  die Bytewerte mit dem Unicode-Codepoint überein.
//  Codepoints > U+00FF (z. B. Emoji) werden durch '?' ersetzt.
// ─────────────────────────────────────────────────────────────────────────────

static size_t utf8ToWPC1252(const uint8_t *utf8, size_t utf8Len,
                             uint8_t *out, size_t outMax)
{
    size_t i = 0, o = 0;
    while (i < utf8Len && o < outMax) {
        uint8_t b = utf8[i];
        if (b < 0x80) {
            // ASCII – 1:1 durchreichen
            out[o++] = b;
            i++;
        } else if ((b & 0xE0) == 0xC0 && i + 1 < utf8Len) {
            // 2-Byte UTF-8  (U+0080 .. U+07FF)
            uint16_t cp = ((uint16_t)(b & 0x1F) << 6)
                        | (utf8[i + 1] & 0x3F);
            out[o++] = (cp <= 0xFF) ? (uint8_t)cp : '?';
            i += 2;
        } else if ((b & 0xF0) == 0xE0 && i + 2 < utf8Len) {
            // 3-Byte UTF-8  (U+0800 .. U+FFFF) – nicht in WPC1252, Euro (€=0x80) separat
            uint16_t cp = ((uint16_t)(b & 0x0F) << 12)
                        | ((uint16_t)(utf8[i+1] & 0x3F) << 6)
                        | (utf8[i+2] & 0x3F);
            if (cp == 0x20AC) out[o++] = 0x80;     // € → 0x80 in WPC1252
            else              out[o++] = '?';
            i += 3;
        } else if ((b & 0xF8) == 0xF0 && i + 3 < utf8Len) {
            // 4-Byte UTF-8  (Emoji etc.) – nicht darstellbar
            out[o++] = '?';
            i += 4;
        } else {
            // Ungültiges Byte – überspringen
            out[o++] = '?';
            i++;
        }
    }
    return o;
}

static bool printText(const String &text)
{
    // UTF-8 → WPC1252 konvertieren
    size_t utf8Len = text.length();
    uint8_t *conv = (uint8_t *)malloc(utf8Len);
    if (!conv) return false;
    size_t convLen = utf8ToWPC1252((const uint8_t *)text.c_str(),
                                   utf8Len, conv, utf8Len);

    // 2 (ESC@) + 3 (ESC t 16) + convLen + 1 (LF) + 3 (ESC d n)
    size_t cmdLen = 2 + 3 + convLen + 1 + 3;
    uint8_t *buf = (uint8_t *)malloc(cmdLen);
    if (!buf) { free(conv); return false; }

    size_t p = 0;
    buf[p++] = 0x1B; buf[p++] = 0x40;       // ESC @  – Initialisieren
    buf[p++] = 0x1B; buf[p++] = 0x74;       // ESC t  – Code Page wählen
    buf[p++] = 16;                            //          Page 16 = WPC1252

    memcpy(buf + p, conv, convLen);
    p += convLen;
    free(conv);

    buf[p++] = 0x0A;                          // LF
    buf[p++] = 0x1B; buf[p++] = 0x64;       // ESC d  – Papiervorschub
    buf[p++] = 0x03;                          //          3 Zeilen

    bool ok = sendToPrinter(buf, p);
    free(buf);
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Gerät öffnen, Interface claimen, Endpoints ermitteln
// ─────────────────────────────────────────────────────────────────────────────
//  Wird aufgerufen, sobald der USB-Stack ein neues Gerät meldet.
//
//  Ablauf:
//    1.  Gerät öffnen  (usb_host_device_open)
//    2.  Device-Descriptor lesen  →  VID/PID loggen
//    3.  Konfigurations-Descriptor lesen
//    4.  Byte-weise durch die Deskriptoren iterieren und das erste
//        Interface mit einem Bulk-OUT-Endpoint suchen
//    5.  Interface claimen  (exklusiver Zugriff)
//    6.  Transfer-Objekt allozieren
// ─────────────────────────────────────────────────────────────────────────────

static void openPrinterDevice(uint8_t devAddr)
{
    esp_err_t err;

    // ── 1. Gerät öffnen ──
    err = usb_host_device_open(usbClientHdl, devAddr, &printerDevHdl);
    if (err != ESP_OK) {
        webLog("[USB] device_open Fehler: %s\n", esp_err_to_name(err));
        return;
    }

    // ── 2. Device-Descriptor ──
    const usb_device_desc_t *devDesc = NULL;
    usb_host_get_device_descriptor(printerDevHdl, &devDesc);
    if (devDesc) {
        webLog("[USB] Gerät  VID=0x%04X  PID=0x%04X  Class=0x%02X\n",
                      devDesc->idVendor, devDesc->idProduct, devDesc->bDeviceClass);
    }

    // ── 3. Aktive Konfiguration holen ──
    const usb_config_desc_t *cfgDesc = NULL;
    err = usb_host_get_active_config_descriptor(printerDevHdl, &cfgDesc);
    if (err != ESP_OK || !cfgDesc) {
        webLog("[USB] Konfiguration nicht lesbar\n");
        usb_host_device_close(usbClientHdl, printerDevHdl);
        printerDevHdl = NULL;
        return;
    }

    // ── 4. Deskriptoren parsen  →  Bulk-OUT suchen ──
    //
    //  Der Konfigurations-Descriptor ist ein zusammenhängender Byte-Block
    //  der Länge  cfgDesc->wTotalLength.  Darin befinden sich Interface-
    //  und Endpoint-Deskriptoren hintereinander.
    //
    //  Jeder Deskriptor beginnt mit:
    //      Byte 0 = bLength    (Länge in Bytes)
    //      Byte 1 = bDescriptorType
    //
    //  Wir merken uns die Interface-Nummer des zuletzt gesehenen
    //  Interface-Deskriptors, damit wir wissen, welches Interface wir
    //  claimen müssen, wenn wir den passenden Endpoint finden.

    const uint8_t *raw = (const uint8_t *)cfgDesc;
    uint16_t totalLen   = cfgDesc->wTotalLength;
    int      pos        = 0;
    uint8_t  curIface   = 0;
    bool     found      = false;

    while (pos < totalLen) {
        uint8_t dLen  = raw[pos];
        uint8_t dType = raw[pos + 1];

        if (dLen == 0) break;   // Schutz vor Endlosschleife

        if (dType == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
            // ── Interface-Deskriptor ──
            const usb_intf_desc_t *intf = (const usb_intf_desc_t *)(raw + pos);
            curIface = intf->bInterfaceNumber;
            webLog("[USB]  Interface %d  Class=0x%02X  Sub=0x%02X  Proto=0x%02X  EPs=%d\n",
                          intf->bInterfaceNumber,
                          intf->bInterfaceClass,
                          intf->bInterfaceSubClass,
                          intf->bInterfaceProtocol,
                          intf->bNumEndpoints);
        }
        else if (dType == USB_B_DESCRIPTOR_TYPE_ENDPOINT) {
            // ── Endpoint-Deskriptor ──
            const usb_ep_desc_t *ep = (const usb_ep_desc_t *)(raw + pos);

            // Transfertyp  (Bits 1:0 von bmAttributes)
            uint8_t xferType = ep->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK;
            // Richtung  (Bit 7 von bEndpointAddress: 0 = OUT, 1 = IN)
            bool isOut = (ep->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) == 0;

            webLog("[USB]    EP 0x%02X  Type=%d  Dir=%s  MPS=%d\n",
                          ep->bEndpointAddress, xferType,
                          isOut ? "OUT" : "IN",
                          ep->wMaxPacketSize);

            if (xferType == USB_BM_ATTRIBUTES_XFER_BULK && isOut && !found) {
                bulkOutEpAddr   = ep->bEndpointAddress;
                bulkOutMps      = ep->wMaxPacketSize;
                printerIfaceNum = curIface;
                found = true;
                webLog("[USB] >> Bulk-OUT EP 0x%02X  MPS=%d  auf Interface %d\n",
                              bulkOutEpAddr, bulkOutMps, printerIfaceNum);
            }
        }

        pos += dLen;
    }

    if (!found) {
        webLog("[USB] Kein Bulk-OUT-Endpoint gefunden\n");
        usb_host_device_close(usbClientHdl, printerDevHdl);
        printerDevHdl = NULL;
        return;
    }

    // ── 5. Interface claimen ──
    //  Damit „gehört" der USB-Endpunkt exklusiv diesem Client.
    err = usb_host_interface_claim(usbClientHdl, printerDevHdl,
                                   printerIfaceNum, 0 /* alt-setting */);
    if (err != ESP_OK) {
        webLog("[USB] interface_claim Fehler: %s\n", esp_err_to_name(err));
        usb_host_device_close(usbClientHdl, printerDevHdl);
        printerDevHdl = NULL;
        return;
    }

    // ── 6. Transfer-Objekt allozieren ──
    //  Wird für alle Bulk-Transfers wiederverwendet.
    //  Der zweite Parameter (0) steht für isochronous packet descriptors
    //  – bei Bulk nicht benötigt.
    err = usb_host_transfer_alloc(USB_XFER_BUF_SIZE, 0, &xferOut);
    if (err != ESP_OK) {
        webLog("[USB] transfer_alloc Fehler: %s\n", esp_err_to_name(err));
        usb_host_interface_release(usbClientHdl, printerDevHdl, printerIfaceNum);
        usb_host_device_close(usbClientHdl, printerDevHdl);
        printerDevHdl = NULL;
        return;
    }

    printerReady = true;
    webLog("[USB] === Drucker bereit! ===\n");
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Client-Event-Callback
// ─────────────────────────────────────────────────────────────────────────────
//  Wird vom USB-Host-Stack aufgerufen, wenn:
//    • ein neues Gerät angesteckt wird   (NEW_DEV)
//    • ein Gerät abgezogen wird          (DEV_GONE)
// ─────────────────────────────────────────────────────────────────────────────

static void clientEventCb(const usb_host_client_event_msg_t *msg, void *arg)
{
    switch (msg->event) {

    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        webLog("[USB] Neues Geraet  (Adresse %d)\n", msg->new_dev.address);
        openPrinterDevice(msg->new_dev.address);
        break;

    case USB_HOST_CLIENT_EVENT_DEV_GONE:
        webLog("[USB] Geraet entfernt\n");
        printerReady = false;

        // Ressourcen freigeben
        if (xferOut) {
            usb_host_transfer_free(xferOut);
            xferOut = NULL;
        }
        if (printerDevHdl) {
            usb_host_interface_release(usbClientHdl, printerDevHdl, printerIfaceNum);
            usb_host_device_close(usbClientHdl, printerDevHdl);
            printerDevHdl = NULL;
        }
        break;

    default:
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Library-Daemon-Task
// ─────────────────────────────────────────────────────────────────────────────
//  usb_host_lib_handle_events() muss regelmäßig aufgerufen werden, damit
//  der interne Zustandsautomat des USB-Host-Stacks (Root-Hub, Enumeration,
//  Pipe-Management) funktioniert.
// ─────────────────────────────────────────────────────────────────────────────

static void usbLibTask(void * /*arg*/)
{
    for (;;) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        // Flags ignorieren – wir benutzen immer genau einen Client.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Client-Task
// ─────────────────────────────────────────────────────────────────────────────
//  Registriert den Client und ruft in einer Endlosschleife
//  usb_host_client_handle_events() auf.  Diese Funktion blockiert, bis
//  ein Event vorliegt (z. B. neues Gerät), und ruft dann den oben
//  registrierten Callback (clientEventCb) auf.
// ─────────────────────────────────────────────────────────────────────────────

static void usbClientTask(void * /*arg*/)
{
    // ── Client beim Host-Stack registrieren ──
    usb_host_client_config_t clientCfg = {};
    clientCfg.is_synchronous           = false;
    clientCfg.max_num_event_msg        = 5;
    clientCfg.async.client_event_callback = clientEventCb;
    clientCfg.async.callback_arg          = NULL;

    esp_err_t err = usb_host_client_register(&clientCfg, &usbClientHdl);
    if (err != ESP_OK) {
        webLog("[USB] client_register Fehler: %s\n", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    webLog("[USB] Client registriert - warte auf Drucker...\n");

    for (;;) {
        usb_host_client_handle_events(usbClientHdl, portMAX_DELAY);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  USB-Host  –  Setup
// ─────────────────────────────────────────────────────────────────────────────

static void setupUSBHost()
{
    // Semaphore  &  Mutex anlegen
    xferDoneSem  = xSemaphoreCreateBinary();
    printerMutex = xSemaphoreCreateMutex();

    // ── USB-Host-Library installieren ──
    //  skip_phy_setup = false  →  der Treiber konfiguriert den internen
    //  USB-PHY selbst (Pin-MUX, VBUS, Pull-Ups).
    usb_host_config_t hostCfg = {};
    hostCfg.skip_phy_setup = false;
    hostCfg.intr_flags     = ESP_INTR_FLAG_LEVEL1;

    esp_err_t err = usb_host_install(&hostCfg);
    if (err != ESP_OK) {
        webLog("[USB] usb_host_install Fehler: %s\n", esp_err_to_name(err));
        return;
    }
    webLog("[USB] Host-Library installiert\n");

    // ── Tasks starten  (Core 0, damit Core 1 für WiFi/Webserver frei bleibt) ──
    xTaskCreatePinnedToCore(usbLibTask,    "usb_lib",    4096, NULL, USB_LIB_TASK_PRIO,    NULL, 0);
    xTaskCreatePinnedToCore(usbClientTask, "usb_client", 4096, NULL, USB_CLIENT_TASK_PRIO, NULL, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  WLAN  –  Verbindung aufbauen (versteckte SSID)
// ─────────────────────────────────────────────────────────────────────────────

static void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    // Alle Kanäle durchscannen – nötig für versteckte Netzwerke
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);

    webLog("[WiFi] Verbinde mit \"%s\" ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
        delay(500);
        webLog(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        webLog("\n[WiFi] Verbunden!  IP = %s\n", WiFi.localIP().toString().c_str());
    } else {
        webLog("\n[WiFi] Verbindung fehlgeschlagen - Neustart in 5 s\n");
        delay(5000);
        ESP.restart();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Webserver  –  REST-API & statische Dateien
// ─────────────────────────────────────────────────────────────────────────────

static void setupWebServer()
{
    // ── Statische Dateien aus LittleFS ──
    webServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // ── GET /status  →  Druckerstatus als JSON ──
    webServer.on("/status", HTTP_GET, [](AsyncWebServerRequest *req) {
        String json = "{\"printerReady\":" + String(printerReady ? "true" : "false")
                    + ",\"ip\":\"" + WiFi.localIP().toString() + "\""
                    + ",\"rssi\":" + String(WiFi.RSSI())
                    + ",\"uptime\":" + String(millis() / 1000)
                    + ",\"freeHeap\":" + String(ESP.getFreeHeap())
                    + ",\"psram\":" + String(ESP.getFreePsram())
                    + "}";
        req->send(200, "application/json", json);
    });

    // ── GET /log  →  Debug-Log als Klartext ──
    webServer.on("/log", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/plain; charset=utf-8", webLogRead());
    });

    // ── POST /printText  →  Text drucken ──
    //
    //  Body = reiner Text  (Content-Type beliebig).
    //  Der Code akkumuliert alle Chunks im body-Callback
    //  und druckt im response-Callback.

    webServer.on(
        "/printText", HTTP_POST,
        // --- Response-Handler  (alle Body-Chunks empfangen) ---
        [](AsyncWebServerRequest *req) {
            if (textBodyBuf.isEmpty()) {
                req->send(400, "application/json", "{\"error\":\"Kein Text\"}");
                return;
            }
            if (!printerReady) {
                textBodyBuf.clear();
                req->send(503, "application/json", "{\"error\":\"Drucker nicht verbunden\"}");
                return;
            }
            bool ok = printText(textBodyBuf);
            textBodyBuf.clear();
            if (ok)
                req->send(200, "application/json", "{\"ok\":true}");
            else
                req->send(500, "application/json", "{\"error\":\"Druckfehler\"}");
        },
        NULL, // Upload-Handler nicht benötigt
        // --- Body-Handler  (wird pro Chunk aufgerufen) ---
        [](AsyncWebServerRequest * /*req*/, uint8_t *data, size_t len,
           size_t index, size_t /*total*/) {
            if (index == 0) textBodyBuf.clear();
            textBodyBuf += String((const char *)data, len);
        });

    // ── POST /printImage  →  rohes ESC/POS-Byte-Array drucken ──
    //
    //  Body = application/octet-stream  mit dem fertigen ESC/POS-Datenpaket
    //  (inklusive GS v 0 – Header), das vom Frontend generiert wird.
    //  Wir leiten es 1:1 per USB-Bulk-Transfer an den Drucker weiter.

    webServer.on(
        "/printImage", HTTP_POST,
        // --- Response-Handler ---
        [](AsyncWebServerRequest *req) {
            if (!imgBodyBuf || imgBodyLen == 0) {
                req->send(400, "application/json", "{\"error\":\"Keine Bilddaten\"}");
                return;
            }
            if (!printerReady) {
                free(imgBodyBuf); imgBodyBuf = nullptr; imgBodyLen = 0;
                req->send(503, "application/json", "{\"error\":\"Drucker nicht verbunden\"}");
                return;
            }
            bool ok = sendToPrinter(imgBodyBuf, imgBodyLen);
            free(imgBodyBuf); imgBodyBuf = nullptr; imgBodyLen = 0;
            if (ok)
                req->send(200, "application/json", "{\"ok\":true}");
            else
                req->send(500, "application/json", "{\"error\":\"Druckfehler\"}");
        },
        NULL,
        // --- Body-Handler ---
        [](AsyncWebServerRequest * /*req*/, uint8_t *data, size_t len,
           size_t index, size_t total) {
            if (index == 0) {
                // Beim ersten Chunk  →  Puffer in PSRAM allozieren
                if (imgBodyBuf) { free(imgBodyBuf); imgBodyBuf = nullptr; }
                imgBodyBuf = (uint8_t *)ps_malloc(total);
                if (!imgBodyBuf) imgBodyBuf = (uint8_t *)malloc(total);  // Fallback
                imgBodyLen = 0;
            }
            if (imgBodyBuf && (index + len) <= total) {
                memcpy(imgBodyBuf + index, data, len);
                imgBodyLen = index + len;
            }
        });

    // ── POST /api/config  →  Konfiguration speichern ──
    webServer.on(
        "/api/config", HTTP_POST,
        [](AsyncWebServerRequest *req) {
            if (configBodyBuf.isEmpty()) {
                req->send(400, "application/json", "{\"error\":\"Keine Daten\"}");
                return;
            }
            File f = LittleFS.open("/config.json", "w");
            if (!f) {
                configBodyBuf.clear();
                req->send(500, "application/json", "{\"error\":\"FS-Schreibfehler\"}");
                return;
            }
            f.print(configBodyBuf);
            f.close();
            configBodyBuf.clear();
            webLog("[CFG] config.json gespeichert\n");
            req->send(200, "application/json", "{\"ok\":true}");
        },
        NULL,
        [](AsyncWebServerRequest *, uint8_t *data, size_t len,
           size_t index, size_t) {
            if (index == 0) configBodyBuf.clear();
            configBodyBuf += String((const char *)data, len);
        });

    // ── CORS / Not-Found ──
    webServer.onNotFound([](AsyncWebServerRequest *req) {
        if (req->method() == HTTP_OPTIONS) {
            req->send(204);   // Pre-flight OK
        } else {
            req->send(404, "text/plain", "Nicht gefunden");
        }
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    webServer.begin();
    webLog("[Web] Server gestartet auf Port 80\n");
}

// ─────────────────────────────────────────────────────────────────────────────
//  TCP Raw Print Server  (Port 9100  –  AppSocket / Raw Printing)
// ─────────────────────────────────────────────────────────────────────────────
//  Akzeptiert genau einen Client.  Alle eingehenden Bytes werden 1:1
//  über den USB-Bulk-Endpoint an den Drucker weitergeleitet.
//
//  Der Druck erfolgt chunk-weise:  Sobald Daten im TCP-Puffer bereit-
//  stehen, werden bis zu TCP_CHUNK_SIZE Bytes gelesen und mit dem
//  bestehenden printerMutex gesichert an sendToPrinter() übergeben.
//  Damit ist garantiert, dass Web-API- und TCP-Druckaufträge sich
//  niemals überschneiden.
// ─────────────────────────────────────────────────────────────────────────────

#define TCP_CHUNK_SIZE  4096          // max. Bytes pro read()-Zyklus
static uint8_t *tcpChunkBuf = nullptr; // wird in setup() aus PSRAM alloziert

static void setupTcpPrintServer()
{
    // Chunk-Puffer für TCP→USB Durchleitung (PSRAM bevorzugt)
    tcpChunkBuf = (uint8_t *)ps_malloc(TCP_CHUNK_SIZE);
    if (!tcpChunkBuf) tcpChunkBuf = (uint8_t *)malloc(TCP_CHUNK_SIZE);
    if (!tcpChunkBuf) {
        webLog("[TCP] FEHLER: Chunk-Puffer konnte nicht alloziert werden!\n");
        return;
    }

    tcpPrintServer.begin();
    tcpPrintServer.setNoDelay(true);
    webLog("[TCP] Raw Print Server gestartet auf Port 9100\n");
}

// Non-blocking:  Wird aus loop() aufgerufen.
static void handleTcpPrintServer()
{
    // ── Neue Verbindung annehmen ──
    if (tcpPrintServer.hasClient()) {
        if (tcpPrintClient && tcpPrintClient.connected()) {
            // Es ist bereits ein Client verbunden → ablehnen
            WiFiClient rejected = tcpPrintServer.accept();
            webLog("[TCP] Zweite Verbindung abgelehnt (busy)\n");
            rejected.stop();
        } else {
            tcpPrintClient = tcpPrintServer.accept();
            tcpPrintClient.setNoDelay(true);
            webLog("[TCP] Client verbunden:  %s\n",
                          tcpPrintClient.remoteIP().toString().c_str());
        }
    }

    // ── Daten vom verbundenen Client lesen und an Drucker senden ──
    if (tcpPrintClient && tcpPrintClient.connected()) {
        int avail = tcpPrintClient.available();
        if (avail > 0) {
            if (!printerReady) {
                // Drucker nicht bereit – Daten verwerfen, Client trennen
                webLog("[TCP] Drucker nicht bereit – Client getrennt\n");
                tcpPrintClient.stop();
                return;
            }

            // Chunk lesen (maximal TCP_CHUNK_SIZE)
            size_t toRead = (avail > TCP_CHUNK_SIZE) ? TCP_CHUNK_SIZE : (size_t)avail;
            size_t got = tcpPrintClient.read(tcpChunkBuf, toRead);
            if (got > 0) {
                // Mutex-gesichert an USB-Drucker senden
                bool ok = sendToPrinter(tcpChunkBuf, got);
                if (!ok) {
                    webLog("[TCP] USB-Sendefehler – Client getrennt\n");
                    tcpPrintClient.stop();
                }
            }
        }
    } else if (tcpPrintClient) {
        // Client hat die Verbindung geschlossen
        webLog("[TCP] Client getrennt\n");
        tcpPrintClient.stop();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  mDNS Auto-Discovery  (Bonjour / ZeroConf)
// ─────────────────────────────────────────────────────────────────────────────
//  Registriert den ESP32 als "thermodrucker.local" im lokalen Netz.
//  Der Service "_pdl-datastream._tcp" auf Port 9100 signalisiert macOS,
//  Windows und Linux, dass hier ein Raw-/AppSocket-Netzwerkdrucker
//  bereitsteht, der über „Drucker hinzufügen" automatisch gefunden wird.
// ─────────────────────────────────────────────────────────────────────────────

static void setupMDNS()
{
    if (!MDNS.begin(MDNS_HOSTNAME)) {
        webLog("[mDNS] Start fehlgeschlagen!\n");
        return;
    }
    webLog("[mDNS] Hostname: %s.local\n", MDNS_HOSTNAME);

    // HTTP-Webinterface
    MDNS.addService("http", "tcp", 80);

    // Raw Print / AppSocket  –  wird von OS-Druckerdialogen erkannt
    MDNS.addService("pdl-datastream", "tcp", 9100);

    // Zusätzliche TXT-Records für bessere Erkennung
    MDNS.addServiceTxt("pdl-datastream", "tcp", "txtvers", "1");
    MDNS.addServiceTxt("pdl-datastream", "tcp", "ty", "JK-5803P Thermodrucker");
    MDNS.addServiceTxt("pdl-datastream", "tcp", "product", "(ESP32-S3 Bridge)");
    MDNS.addServiceTxt("pdl-datastream", "tcp", "pdl", "application/vnd.esc-pos");
    MDNS.addServiceTxt("pdl-datastream", "tcp", "usb_MFG", "STMicroelectronics");
    MDNS.addServiceTxt("pdl-datastream", "tcp", "usb_MDL", "JK-5803P");

    webLog("[mDNS] Services registriert (http + pdl-datastream)\n");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Arduino  –  Setup
// ─────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    delay(800);

    // Web-Log Mutex VOR dem ersten webLog() anlegen
    webLogMutex = xSemaphoreCreateMutex();
    memset(webLogBuf, 0, WEBLOG_SIZE);

    webLog("\n========================================\n");
    webLog("  ESP32-S3  Thermodrucker USB-Bridge\n");
    webLog("========================================\n");

    // PSRAM prüfen
    if (psramFound()) {
        webLog("[SYS] PSRAM verfuegbar:  %d Bytes\n", ESP.getPsramSize());
    } else {
        webLog("[SYS] Kein PSRAM erkannt\n");
    }

    // LittleFS initialisieren  (true = formatieren, falls leer)
    if (!LittleFS.begin(true)) {
        webLog("[FS]  LittleFS Mount fehlgeschlagen!\n");
    } else {
        webLog("[FS]  LittleFS bereit  %d / %d Bytes belegt\n",
                      LittleFS.usedBytes(), LittleFS.totalBytes());
    }

    // WLAN
    setupWiFi();

    // mDNS (nach WiFi, vor Webserver)
    setupMDNS();

    // Webserver (Port 80)
    setupWebServer();

    // TCP Raw Print Server (Port 9100)
    setupTcpPrintServer();

    // USB-Host (startet eigene FreeRTOS-Tasks)
    setupUSBHost();

    webLog("\n[SYS] === Alle Dienste gestartet ===\n");
    webLog("[SYS]   Web:  http://%s.local/  oder  http://%s/\n",
                  MDNS_HOSTNAME, WiFi.localIP().toString().c_str());
    webLog("[SYS]   RAW:  %s:%d  (AppSocket)\n",
                  WiFi.localIP().toString().c_str(), 9100);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Arduino  –  Loop
// ─────────────────────────────────────────────────────────────────────────────
//  Non-blocking:  Pollt den TCP Print Server auf Port 9100.
//  Webserver (Port 80) und USB-Host laufen event-basiert / in eigenen Tasks.
// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    // TCP Print Server bedienen (non-blocking)
    handleTcpPrintServer();

    // Kurze Pause – gibt WiFi-Stack und FreeRTOS-Scheduler CPU-Zeit.
    // 2 ms ergibt max. ~500 Polls/s, was für Druckdaten mehr als genug ist.
    vTaskDelay(pdMS_TO_TICKS(2));
}
