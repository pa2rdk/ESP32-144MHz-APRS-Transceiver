/////////////////////////////////////////////////////////////////////////////////////////
// V2.9 Bug with beacons on main frequency
// V2.8 OTA
// V2.6 Screen rotation and calibration added
// V2.5 Battery level
// V2.4 Beeper
// V2.3 Removed delay from loop
// V2.2 Added a switch to enable/disable serial debugging
// V2.1 RS232 Read tasks replaced to second task (on other core)
// V2.0 GPS and DRA separated
//      GPS is connected to pin 39 (only RX)
//      DRA is connected to 16 (RX) and 17 (TX)
//      Implemented S meter
// V1.4 Rename HandleButton->HandleFunction, change SaveConfig and replace APRS setter
// V1.3 Code review
// V1.2 Better highlighted button
// V1.1 Gradient buttons
// V1.0
// V0.9AC
//
// DRA818 Info  :http://www.dorji.com/docs/data/DRA818V.pdf
// LibAPRS from :https://codeload.github.com/tomasbrincil/LibAPRS-esp32/zip/refs/heads/master
//
//  *********************************
//  **   Display connections       **
//  *********************************
//  |------------|------------------|
//  |Display 2.8 |      ESP32       |
//  |  ILI9341   |                  |
//  |------------|------------------|
//  |   Vcc      |     3V3          |
//  |   GND      |     GND          |
//  |   CS       |     15           |
//  |   Reset    |      4           |
//  |   D/C      |      2           |
//  |   SDI      |     23           |
//  |   SCK      |     18           |
//  |   LED Coll.|     14 2K        |
//  |   SDO      |                  |
//  |   T_CLK    |     18           |
//  |   T_CS     |      5           |
//  |   T_DIN    |     23           |
//  |   T_DO     |     19           |
//  |   T_IRQ    |     34           |
//  |------------|------------------|
/////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>  // https://github.com/Bodmer/TFT_eSPI
#include <WiFi.h>
#include <WifiMulti.h>
#include <EEPROM.h>
#include "NTP_Time.h"
#include <TinyGPS++.h>
#include <LibAPRS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>
#include <HardwareSerial.h>
#include "esp_adc_cal.h"
#include <RDKOTA.h>

#define offsetEEPROM 32
#define EEPROM_SIZE 2048
#define AA_FONT_SMALL "fonts/NotoSansBold15"  // 15 point sans serif bold
#define AA_FONT_LARGE "fonts/NotoSansBold36"  // 36 point sans serif bold
#define VERSION "PA2RDK_IGATE_TCP"
#define INFO "Arduino PA2RDK IGATE"
#define WDT_TIMEOUT 10

#define TFT_GREY 0x5AEB
#define TFT_LIGTHYELLOW 0xFF10
#define TFT_DARKBLUE 0x016F
#define TFT_SHADOW 0xE71C
#define TFT_BUTTONTOPCOLOR 0xB5FE

#define BTN_NAV 32768
#define BTN_NEXT 16384
#define BTN_PREV 8192
#define BTN_CLOSE 4096
#define BTN_ARROW 2048
#define BTN_NUMERIC 1024

#define RXD1 39
#define TXD1 -1
#define RXD2 16
#define TXD2 17
#define PTTIN 27
#define PTTOUT 33
#define BTNSMIKEPIN 35
#define SQUELSHPIN 21
#define TRXONPIN 12
#define DISPLAYLEDPIN 14
#define HIPOWERPIN 13
#define MUTEPIN 22
#define BEEPPIN 32

#define ADC_REFERENCE REF_3V3
#define OPEN_SQUELCH false

#define SCAN_STOPPED 0
#define SCAN_INPROCES 1
#define SCAN_PAUSED 2
#define SCAN_TYPE_STOP 0
#define SCAN_TYPE_RESUME 1
#define SHIFT_POS 1
#define SHIFT_NONE 0
#define SHIFT_NEG -1
#define TONETYPENONE 0
#define TONETYPERX 1
#define TONETYPETX 2
#define TONETYPERXTX 3

#define LipoVoltpin 36
#define LipoMeasureTime 10  // Lipo check every 10 seconds

#define OTAHOST      "https://www.rjdekok.nl/Updates/APRSTRX"
#define OTAVERSION   "v2.9"

//#define DebugEnabled
#ifdef DebugEnabled
#define DebugPrint(x) Serial.print(x)
#define DebugPrintln(x) Serial.println(x)
#define DebugPrintf(x, ...) Serial.printf(x, __VA_ARGS__)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#define DebugPrintf(x, ...)
#endif
typedef struct {  // WiFi Access
  const char *SSID;
  const char *PASSWORD;
} wlanSSID;

typedef struct {  // Frequency parts
  int fMHz;
  int fKHz;
} SFreq;

typedef struct {        // Buttons
  const char *name;     // Buttonname
  const char *caption;  // Buttoncaption
  char waarde[12];      // Buttontext
  uint16_t pageNo;
  uint16_t xPos;
  uint16_t yPos;
  uint16_t width;
  uint16_t height;
  uint16_t bottomColor;
  uint16_t topColor;
} Button;

const Button buttons[] = {
  { "ToLeft", "<<", "", BTN_ARROW, 2, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "ToRight", ">>", "", BTN_ARROW, 242, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Vol", "Vol", "", 1, 2, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "SQL", "SQL", "", 1, 82, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Scan", "Scan", "", 1, 162, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Off", "Off", "", 1, 242, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Freq", "Freq", "", 1, 2, 172, 74, 30, TFT_BLACK, TFT_WHITE },
  { "RPT", "RPT", "", 1, 82, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "MEM", "MEM", "", 1, 162, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Shift", "Shift", "", 2, 2, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Mute", "Mute", "", 2, 82, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Tone", "Tone", "", 2, 162, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Reverse", "Reverse", "", 2, 242, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "MOX", "MOX", "", 2, 82, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "APRS", "APRS", "", 2, 162, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Beacon", "Beacon", "", 2, 82, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "TXBeacon", "TX Beacon", "", 2, 162, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Power", "Power", "", 4, 2, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "SetBand", "Band", "", 4, 82, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Light", "Light", "", 4, 162, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Calibrate", "Calibrate", "", 4, 242, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Save", "Save", "", 4, 82, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Print", "Print", "", 4, 162, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "A001", "1", "", BTN_NUMERIC, 42, 100, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A002", "2", "", BTN_NUMERIC, 122, 100, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A003", "3", "", BTN_NUMERIC, 202, 100, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A004", "4", "", BTN_NUMERIC, 42, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A005", "5", "", BTN_NUMERIC, 122, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A006", "6", "", BTN_NUMERIC, 202, 136, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A007", "7", "", BTN_NUMERIC, 42, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A008", "8", "", BTN_NUMERIC, 124, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A009", "9", "", BTN_NUMERIC, 202, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Clear", "Clear", "", BTN_NUMERIC, 42, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "A000", "0", "", BTN_NUMERIC, 122, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Enter", "Enter", "", BTN_NUMERIC, 202, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },

  { "Prev", "Prev", "", BTN_PREV, 2, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Next", "Next", "", BTN_NEXT, 242, 172, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "ToLeft", "<<", "", BTN_NAV, 2, 208, 154, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "ToRight", ">>", "", BTN_NAV, 162, 208, 154, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Navigate", "Freq", "", BTN_NAV, 2, 208, 314, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
  { "Close", "Close", "", BTN_CLOSE, 122, 208, 74, 30, TFT_BLACK, TFT_BUTTONTOPCOLOR },
};

typedef struct {
  uint16_t rxChannel;  // RX Freq channel (12.5KHz steps, 144.000MHz = 0, channel 128 is 145.600MHz)
  uint16_t txChannel;  // TX Freq channel (12.5KHz steps, 144.000MHz = 0, channel 128 is 145.600MHz)
  uint8_t repeater;    // Repeater number from the list of repeaters, overwrites RX and TX channel
  int8_t txShift;      // Shift + or -
  byte hasTone;        // 0=disabled, 1=only RX, 2=only TX, 3=RX and TX
  byte ctcssTone;      // See list of CTCSS codes in config.h
} Memory;

typedef struct {
  byte chkDigit;
  bool isUHF;
  uint16_t aprsChannel;
  uint16_t rxChannel;
  uint16_t txChannel;
  uint8_t repeater;
  uint8_t memoryChannel;
  uint8_t freqType;
  int8_t txShift;
  bool autoShift;
  bool draPower;
  byte hasTone;
  bool disableRXTone;
  byte ctcssTone;
  byte volume;
  byte squelsh;
  byte scanType;
  char wifiSSID[25];
  char wifiPass[25];
  char aprsIP[25];
  uint16_t aprsPort;
  char aprsPassword[6];
  char dest[8];
  byte destSsid;
  char call[8];
  byte ssid;
  byte serverSsid;
  uint16_t aprsGatewayRefreshTime;
  char comment[16];
  char symbool;  // = auto.
  char path1[8];
  byte path1Ssid;
  char path2[8];
  byte path2Ssid;
  byte interval;
  byte multiplier;
  byte power;
  byte height;
  byte gain;
  byte directivity;
  uint16_t preAmble;
  uint16_t tail;
  byte doTX;
  byte bcnAfterTX;
  byte txTimeOut;
  uint16_t maxChannel;
  uint16_t minUHFChannel;
  uint16_t maxUHFChannel;
  uint16_t currentBrightness;
  float lat;
  float lon;
  bool useAPRS;
  byte mikeLevel;
  byte repeatSetDRA;
  bool preEmphase;
  bool highPass;
  bool lowPass;
  bool isDebug;
  uint16_t calData0;
  uint16_t calData1;
  uint16_t calData2;
  uint16_t calData3;
  uint16_t calData4; 
  bool doRotate;
} Settings;

typedef struct {
  char code[5];
  char tone[8];
} CTCSSCode;

typedef struct {       // Repeaterlist
  char *name;          // Repeatername
  char *city;          // Repeatercity
  int8_t shift;        // Shift + or -
  uint16_t channel;    // RX Freq channel (12.5KHz steps, 144.000MHz = 0, channel 128 is 145.600MHz)
  uint16_t ctcssTone;  // See list of CTCSS codes in config.h
  uint16_t hasTone;    // 0=disabled, 1=only RX, 2=only TX, 3=RX and TX
} Repeater;

// Settings & Variables
bool isPTT = false;
bool lastPTT = false;
bool isMOX = false;
bool isReverse = false;
uint8_t activeBtn = -1;
String commandButton = "";
long activeBtnStart = millis();
long lastBeacon = millis();
long loopDelay = millis();
long lastGetDraRSSI = millis();
int actualPage = 1;
int lastPage = 8;
int beforeDebugPage = 0;
int debugPage = 16;
int debugLine = 0;
long startTime = millis();
long gpsTime = millis();
long saveTime = millis();
long scanCheck = millis();
long lastPressed = millis();
long startedDebugScreen = millis();
long aprsGatewayRefreshed = millis();
long webRefresh = millis();
long waitForResume = 0;
long LipoMeasure = millis();
bool wifiAvailable = false;
bool wifiAPMode = false;
bool isOn = true;
bool gotPacket = false;
bool squelshClosed = true;
bool isMuted = false;
bool aprsGatewayConnected = false;
int scanMode = SCAN_STOPPED;
uint16_t lastCourse = 0;
char httpBuf[120] = "\0";
char buf[300] = "\0";
char draBuffer[300] = "\0";
int draBufferLength = 0;
int oldSwr, swr = 0;
int32_t keyboardNumber = 0;
bool dirtyScreen = false;
bool doTouch = false;

AX25Msg incomingPacket;
uint8_t *packetData;

const int ledFreq = 5000;
const int ledResol = 8;
const int ledChannelforTFT = 0;

#include "config.h";  // Change to config.h

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
WiFiMulti wifiMulti;
WiFiClient httpNet;
TinyGPSPlus gps;
RDKOTA rdkOTA(OTAHOST);
AsyncWebServer server(80);
AsyncEventSource events("/events");
Memory memories[10] = {};
int memTeller = 0;

HardwareSerial GPSSerial(1);
HardwareSerial DRASerial(2);

#include "webpages.h";

/***************************************************************************************
**                          Setup
***************************************************************************************/
void setup() {
  pinMode(PTTOUT, OUTPUT);
  digitalWrite(PTTOUT, 0);  // low no PTT
  pinMode(HIPOWERPIN, OUTPUT);
  digitalWrite(HIPOWERPIN, 0);  // low power from DRA
  pinMode(TRXONPIN, OUTPUT);
  digitalWrite(TRXONPIN, 0);  // low TRX On
  pinMode(DISPLAYLEDPIN, OUTPUT);
  digitalWrite(DISPLAYLEDPIN, 0);
  pinMode(MUTEPIN, OUTPUT);
  digitalWrite(MUTEPIN, 0);
  if (BEEPPIN > -1) {
    pinMode(BEEPPIN, OUTPUT);
    digitalWrite(BEEPPIN, 0);
  }

  pinMode(PTTIN, INPUT_PULLUP);
  pinMode(BTNSMIKEPIN, INPUT);
  pinMode(SQUELSHPIN, INPUT);

  ledcSetup(ledChannelforTFT, ledFreq, ledResol);
  ledcAttachPin(DISPLAYLEDPIN, ledChannelforTFT);

  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, RXD1, TXD1);
  DRASerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  if (!EEPROM.begin(EEPROM_SIZE)) {
    DrawButton(80, 120, 160, 30, "EEPROM Failed", "", TFT_BLACK, TFT_WHITE, "");
    DebugPrintln("failed to initialise EEPROM");
    while (1)
      ;
  }

  if (!LoadConfig()) {
    DebugPrintln(F("Writing defaults"));
    SaveConfig();
    Memory myMemory = { 0, 0, 0, 0, 0, 0 };
    for (int x = 0; x < 10; x++) {
      memories[x] = myMemory;
    }
    SaveMemories();
  }

  LoadConfig();
  LoadMemories();

  tft.init();
  tft.setRotation(settings.doRotate?3:1);
  uint16_t calData[5] = {settings.calData0, settings.calData1, settings.calData2, settings.calData3, settings.calData4};
  tft.setTouch(calData);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // add Wi-Fi networks from All_Settings.h
  for (int i = 0; i < sizeof(wifiNetworks) / sizeof(wifiNetworks[0]); i++) {
    wifiMulti.addAP(wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
    DebugPrintf("Wifi:%s, Pass:%s.\r\n", wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
  }

  DrawButton(80, 80, 160, 30, "Connecting to WiFi", "", TFT_BLACK, TFT_WHITE, "");

  // show connected SSID
  wifiMulti.addAP(settings.wifiSSID, settings.wifiPass);
  DebugPrintf("Wifi:%s, Pass:%s.\r\n", settings.wifiSSID, settings.wifiPass);
  if (Connect2WiFi()) {
    wifiAvailable = true;
    DrawButton(80, 80, 160, 30, "Connected to WiFi", WiFi.SSID(), TFT_BLACK, TFT_WHITE, "");
    delay(1000);

    if (rdkOTA.checkForUpdate(OTAVERSION)){
      if (questionBox("Install update", TFT_WHITE, TFT_NAVY, 80, 80, 160, 40)){
        DrawButton(80, 80, 160, 40, "Installing update", "", TFT_BLACK, TFT_RED, "");
        rdkOTA.installUpdate();
      } 
    }

    udp.begin(localPort);
    syncTime();
  } else {
    wifiAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("APRSTRX", NULL);
  }
  DebugPrintf("Main loop running in Core:%d.\r\n", xPortGetCoreID());

  ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
  digitalWrite(HIPOWERPIN, !settings.draPower);  // low power from DRA
  delay(25);
  SetFreq(0, 0, 0, false);
  delay(100);
  SetFreq(0, 0, 0, false);
  delay(100);
  SetDraVolume(settings.volume);
  delay(100);
  SetDraSettings();
  delay(100);
  APRS_init(ADC_REFERENCE, OPEN_SQUELCH);
  SetAPRSParameters();

  xTaskCreatePinnedToCore(
    ReadTask, "ReadTask"  // A name just for humans
    ,
    10000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,
    NULL, 0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,
    NULL, 0);  // Core 0

  esp_task_wdt_init(WDT_TIMEOUT, true);  //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                //add current thread to WDT watch

  DrawScreen();
}

void ReadTask(void *pvParameters) { // This is a task.
  if (wifiAvailable || wifiAPMode) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html, Processor);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/css", css_html);
    });

    server.on("/command", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (request->hasParam("button")) commandButton = request->getParam("button")->value();
      if (request->hasParam("setfreq")) {
        SetFreq(request->getParam("setfreq")->value().toFloat());
        dirtyScreen = true;
      }
      if (request->hasParam("setrepeater")) {
        DebugPrintln(request->getParam("setrepeater")->value());
        uint8_t i = request->getParam("setrepeater")->value().toInt();
        if (i > -1) {
          settings.repeater = i;
          SetRepeater(i);
          dirtyScreen = true;
        }
      }
      delay(100);
      request->send_P(200, "text/html", index_html, Processor);
    });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", settings_html, Processor);
    });

    server.on("/nummers", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", nummers_html, Processor);
    });

    server.on("/repeaters", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", repeaters_html, Processor);
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Rebooting");
      ESP.restart();
    });

    server.on("/calibrate", HTTP_GET, [] (AsyncWebServerRequest *request) {
      if (request->client()->remoteIP()[0] == 192 || request->client()->remoteIP()[0] == 10 || request->client()->remoteIP()[0] == 172){
        request->send_P(200, "text/html", index_html, Processor);
        doTouch = true;
      }
      else
        request->send_P(200, "text/html", warning_html, Processor);
    });

    server.on("/store", HTTP_GET, [](AsyncWebServerRequest *request) {
      SaveSettings(request);
      SaveConfig();
      SetAPRSParameters();
      request->send_P(200, "text/html", settings_html, Processor);
    });

    events.onConnect([](AsyncEventSourceClient *client) {
      DebugPrintln("Connect web");
      if (client->lastId()) {
        DebugPrintf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);

    server.begin();
    DebugPrintln("HTTP server started");
    DebugPrintf("HTTP server running in Core:%d.\r\n", xPortGetCoreID());
  }

  for (;;)  // A Task shall never return or exit.
  {
    ReadDRAPort();
    readGPSData();
    vTaskDelay(50 / portTICK_PERIOD_MS);  // wait for 50 miliSec
  }
}

void SetAPRSParameters() {
  APRS_setCallsign(settings.call, settings.ssid);
  APRS_setDestination(settings.dest, settings.destSsid);
  APRS_setSymbol(settings.symbool);
  APRS_setPath1(settings.path1, settings.path1Ssid);
  APRS_setPath2(settings.path2, settings.path2Ssid);
  APRS_setPower(settings.power);
  APRS_setHeight(settings.height);
  APRS_setGain(settings.gain);
  APRS_setDirectivity(settings.directivity);
  APRS_setPreamble(settings.preAmble);
  APRS_setTail(settings.tail);
  APRS_setLat(Deg2Nmea(settings.lat, true));
  APRS_setLon(Deg2Nmea(settings.lon, false));
  APRS_printSettings();

  APRSGatewayUpdate();
  aprsGatewayRefreshed = millis();
}

bool Connect2WiFi() {
  startTime = millis();
  DebugPrint("Connect to Multi WiFi");
  while (wifiMulti.run() != WL_CONNECTED && millis() - startTime < 30000) {
    esp_task_wdt_reset();
    delay(1000);
    DebugPrint(".");
  }
  DebugPrintln();
  return (WiFi.status() == WL_CONNECTED);
}

void doBeep(int timeLen){
    if (BEEPPIN > -1) {
    digitalWrite(BEEPPIN, 1);
    delay(timeLen);
    digitalWrite(BEEPPIN, 0);
  }
}

/***************************************************************************************
**                          Loop
***************************************************************************************/
void loop() {
  esp_task_wdt_reset();

  if (doTouch){
    doTouch = false;
    TouchCalibrate();
    actualPage = 1;
    DrawScreen();
  }

  if (millis() - loopDelay > 50) {
    loopDelay = millis();

    if (commandButton > "") {
      if (commandButton == "ToLeft" || commandButton == "ToRight") {
        HandleFunction(commandButton, -1, 0);
      } else {
        HandleFunction(commandButton, false);
      }
      commandButton = "";
      ClearButtons();
    }

    if (scanMode == SCAN_INPROCES && millis() - scanCheck > 100) {
      if (settings.freqType == FindButtonIDByName("Freq")) {
        settings.rxChannel++;
        if (!settings.isUHF && settings.rxChannel == settings.maxChannel) settings.rxChannel = 0;
        if (settings.isUHF && settings.rxChannel == settings.maxUHFChannel) settings.rxChannel = settings.minUHFChannel;
        SetFreq(0, settings.rxChannel, 0, false);
      }
      if (settings.freqType == FindButtonIDByName("RPT")) {
        if (settings.repeater < (sizeof(repeaters) / sizeof(repeaters[0])) - 1) settings.repeater++;
        else settings.repeater = 1;
        SetRepeater(settings.repeater);
        DrawButton("RPT");
      }
      if (settings.freqType == FindButtonIDByName("MEM")) {
        if (settings.memoryChannel < 9) settings.memoryChannel++;
        else settings.memoryChannel = 0;
        SetMemory(settings.memoryChannel);
        DrawButton("MEM");
      }
      DrawFrequency(false, false);
      DrawButton("Shift");
      DrawButton("Reverse");
      DrawButton("Tone");
      scanCheck = millis();
    }
  }

  if (millis() - lastPressed > 200) {
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);
    if (pressed) {
      doBeep(25);
      int showVal = ShowControls();
      for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        if ((buttons[i].pageNo & showVal) > 0) {
          if (x >= buttons[i].xPos && x <= buttons[i].xPos + buttons[i].width && y >= buttons[i].yPos && y <= buttons[i].yPos + buttons[i].height) {
            DebugPrint(buttons[i].name);
            DebugPrint(" pressed:");
            HandleFunction(buttons[i], x, y);
          }
        }
      }
    }
    lastPressed = millis();
  }
  WaitForWakeUp();

  bool sql = digitalRead(SQUELSHPIN);
  if (sql != squelshClosed) {
    squelshClosed = sql;
    DrawFrequency(false);
    if (scanMode == SCAN_INPROCES && !squelshClosed) {
      digitalWrite(MUTEPIN, isMuted);
      scanMode = (settings.scanType == SCAN_TYPE_STOP) ? SCAN_STOPPED : SCAN_PAUSED;
      DrawButton("Scan");
    }
  }

  if (scanMode == SCAN_PAUSED && !squelshClosed) waitForResume = 0;
  if (scanMode == SCAN_PAUSED && squelshClosed) {
    if (waitForResume == 0) {
      waitForResume = millis();
    }
    if (millis() - waitForResume > 3000) {
      digitalWrite(MUTEPIN, 1);
      scanMode = SCAN_INPROCES;
      DrawButton("Scan");
      waitForResume = 0;
    }
  }

  int btnValue = analogRead(BTNSMIKEPIN);
  if (btnValue < 2048) {
    doBeep(25);
    int firstBtnValue = btnValue;
    long startPress = millis();
    while (btnValue < 2048 && millis() - startPress < 2500) {
      btnValue = analogRead(BTNSMIKEPIN);
    }
    if (millis() - startPress > 2000) {
      Button button = FindButtonByName("Scan");
      HandleFunction(button, -1, 0);
      delay(200);
    } else {
      if (firstBtnValue < 2) {
        Button button = FindButtonByName("ToRight");
        HandleFunction(button, -1, 0);
        delay(200);
      }

      if (firstBtnValue > 1 && firstBtnValue < 2048) {
        Button button = FindButtonByName("ToLeft");
        HandleFunction(button, -1, 0);
        delay(200);
      }
    }
  }

  if (minute() != lastMinute) {
    syncTime();
    if (actualPage < lastPage && wifiAvailable) DrawTime();
    lastMinute = minute();
  }

  //readGPSData();

  if (millis() - gpsTime > 1000) {
    gpsTime = millis();
    PrintGPSInfo();
  }

  if (!isPTT) {
    if (millis() - lastGetDraRSSI > 250) {
      GetDraRSSI();
      lastGetDraRSSI = millis();
    }
    if (oldSwr != swr) {
      oldSwr = swr;
      DrawMeter(2, 100, 314, 30, swr, isPTT, true);
    }
  }

  bool isFromPTT = CheckAndSetPTT(false);

  if (millis() - saveTime > 10000 && scanMode == SCAN_STOPPED) {
    saveTime = millis();
    SaveConfig();
  }

  if (millis() - activeBtnStart > 10000) {
    if (activeBtn != FindButtonIDByName("Freq") && activeBtn != FindButtonIDByName("RPT") && activeBtn != FindButtonIDByName("MEM") && activeBtn != FindButtonIDByName("Save")) {
      activeBtn = settings.freqType;
      DrawButtons();
    }
  }

  if (settings.useAPRS && (gps.location.isValid() || settings.isDebug)) {
    bool doBeacon = false;
    long beaconInterval = settings.interval * settings.multiplier * 1000;
    int gpsSpeed = gps.speed.isValid() ? gps.speed.kmph() : 0;
    if (gpsSpeed > 5) beaconInterval = settings.interval * 4 * 1000;
    if (gpsSpeed > 25) beaconInterval = settings.interval * 2 * 1000;
    if (gpsSpeed > 80) beaconInterval = settings.interval * 1000;

    if (millis() - lastBeacon > beaconInterval) doBeacon = true;
    if (millis() - lastBeacon > 20000 && settings.isDebug) doBeacon = true;

    if (gps.course.isValid()) {
      uint16_t sbCourse = (abs(gps.course.deg() - lastCourse));
      if (sbCourse > 180) sbCourse = 360 - sbCourse;
      if (sbCourse > 27) doBeacon = true;
    }

    if (isFromPTT && settings.bcnAfterTX) doBeacon = true;
    if (millis() - lastBeacon < 5000) doBeacon = false;

    if (doBeacon) {
      lastCourse = gps.course.isValid() ? gps.course.deg() : -1;
      SendBeacon(false, (isFromPTT && settings.bcnAfterTX));
      lastBeacon = millis();
    }
  }

  if (millis() - aprsGatewayRefreshed > (settings.aprsGatewayRefreshTime * 1000)) {
    APRSGatewayUpdate();
    aprsGatewayRefreshed = millis();
  }

  if ((millis() - webRefresh) > 1000) {
    RefreshWebPage();
    webRefresh = millis();
  }

  if (actualPage == debugPage && millis() - startedDebugScreen > 3000) {
    actualPage = beforeDebugPage;
    beforeDebugPage = 0;
    DrawScreen();
  }

  // processPacket();
  if (dirtyScreen) DrawScreen(true);

  if (actualPage < lastPage && (millis() - LipoMeasure) > LipoMeasureTime * 1000 ) {
    Lipovolt();
    LipoMeasure = millis();
  }
}

void processPacket() {
  if (gotPacket) {
    gotPacket = false;

    DebugPrint(F("Received APRS packet. SRC: "));
    DebugPrint(incomingPacket.src.call);
    DebugPrint(F("-"));
    DebugPrint(incomingPacket.src.ssid);
    DebugPrint(F(". DST: "));
    DebugPrint(incomingPacket.dst.call);
    DebugPrint(F("-"));
    DebugPrint(incomingPacket.dst.ssid);
    DebugPrint(F(". Data: "));

    for (int i = 0; i < incomingPacket.len; i++) {
      Serial.write(incomingPacket.info[i]);
    }
    DebugPrintln("");

    // Remeber to free memory for our buffer!
    free(packetData);

    // You can print out the amount of free
    // RAM to check you don't have any memory
    // leaks
    // DebugPrint(F("Free RAM: ")); DebugPrintln(freeMemory());
  }
}

void WaitForWakeUp() {
  delay(10);
  while (!isOn) {
    esp_task_wdt_reset();
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);
    if (pressed){
      doBeep(25);
      ESP.restart();
    } 
  }
}

bool CheckAndSetPTT(bool isAPRS) {
  bool retVal = false;
  bool isPTTIN = !digitalRead(PTTIN);
  if (isMOX || isPTTIN) isPTT = 1;
  else isPTT = 0;

  if (isPTT && scanMode != SCAN_STOPPED) {
    digitalWrite(MUTEPIN, isMuted);
    scanMode = SCAN_STOPPED;
    DrawButton("Scan");
    delay(1000);
  } else {
    if (isPTTIN && isMOX) isMOX = false;
    if (lastPTT != isPTT && (!settings.isUHF || isAPRS)) {
      digitalWrite(MUTEPIN, isPTT ? true : isMuted);
      DebugPrintln("PTT=" + isPTT ? "True" : "False");
      lastPTT = isPTT;
      if (!isPTT && !isAPRS) retVal = true;
      digitalWrite(PTTOUT, isPTT);
      DrawButton("MOX");
      DrawFrequency(isAPRS);
      delay(10);
    }
  }
  return retVal;
}
/***************************************************************************************
**            APRS Call back - received package
***************************************************************************************/
void aprs_msg_callback(struct AX25Msg *msg) {
  DebugPrintln("APRS packet received");
  // If we already have a packet waiting to be
  // processed, we must drop the new one.
  if (!gotPacket) {
    // Set flag to indicate we got a packet
    gotPacket = true;

    // The memory referenced as *msg is volatile
    // and we need to copy all the data to a
    // local variable for later processing.
    memcpy(&incomingPacket, msg, sizeof(AX25Msg));

    // We need to allocate a new buffer for the
    // data payload of the packet. First we check
    // if there is enough free RAM.

    packetData = (uint8_t *)malloc(msg->len);
    memcpy(packetData, msg->info, msg->len);
    incomingPacket.info = packetData;
  }
}

/***************************************************************************************
**            APRS Send beacon
***************************************************************************************/
void SendBeacon(bool manual, bool afterTX) {
  if (gps.location.age() < 5000 || manual || settings.isDebug) {
    if (settings.isDebug) ShowDebugScreen("Send beacon");
    if ((!wifiAvailable || !APRSGatewayConnect() || settings.isDebug) && (squelshClosed || afterTX)) SendBeaconViaRadio();  // || settings.isDebug
    if (wifiAvailable && APRSGatewayConnect()) SendBeaconViaWiFi();
    lastBeacon = millis();
  }
}

void SendBeaconViaRadio() {
  if (!isPTT) {
    DebugPrintf("Mutestate is %s", digitalRead(MUTEPIN) ? "True" : "False");
    buf[0] = '\0';
    sprintf(buf, "Send APRS beacon via radio");
    events.send(buf, "BEACONINFO", millis());

    DrawDebugInfo("Send beacon via radio..");
    int lastScanMode = scanMode;
    scanMode = SCAN_STOPPED;
    if (lastScanMode != SCAN_STOPPED) delay(500);
    SetFreq(0, 0, 0, true);
    isMOX = 1;
    CheckAndSetPTT(true);
    delay(500);

    if (gps.location.age() > 5000) {
      APRS_setLat(Deg2Nmea(settings.lat, true));
      APRS_setLon(Deg2Nmea(settings.lon, false));
    } else {
      APRS_setLat(Deg2Nmea(gps.location.lat(), true));
      APRS_setLon(Deg2Nmea(gps.location.lng(), false));
    }

    APRS_sendLoc(settings.comment, strlen(settings.comment));

    isMOX = 0;
    CheckAndSetPTT(false);
    delay(500);
    SetFreq(0, settings.rxChannel, settings.txShift + 10, false);
    DrawFrequency(false);
    delay(200);
    scanMode = lastScanMode;
    if (scanMode == SCAN_INPROCES) digitalWrite(MUTEPIN, true);
  }
}

void SendBeaconViaWiFi() {
  if (APRSGatewayConnect()) {
    buf[0] = '\0';
    sprintf(buf, "Send APRS beacon via WiFi");
    events.send(buf, "BEACONINFO", millis());
    if (!settings.isDebug) {
      PrintTXTLine();
      tft.fillCircle(50, 64, 24, TFT_RED);
      tft.setTextDatum(MC_DATUM);
      tft.setTextPadding(tft.textWidth("AP"));
      tft.setTextColor(TFT_BLACK, TFT_RED);
      tft.drawString("AP", 50, 66, 4);
    }
    String sLat;
    String sLon;
    if (gps.location.age() > 5000) {
      sLat = Deg2Nmea(settings.lat, true);
      sLon = Deg2Nmea(settings.lon, false);
    } else {
      sLat = Deg2Nmea(gps.location.lat(), true);
      sLon = Deg2Nmea(gps.location.lng(), false);
    }
    sprintf(buf, "%s-%d>%s:=%s/%s%sPHG5000%s", settings.call, settings.ssid, settings.dest, sLat, sLon, String(settings.symbool), settings.comment);
    DrawDebugInfo(buf);
    httpNet.println(buf);
    if (ReadHTTPNet()) DrawDebugInfo(buf);
    if (!settings.isDebug) {
      sprintf(buf, "                                                  ");
      PrintTXTLine();
      tft.fillCircle(50, 64, 24, TFT_BLACK);
    }
  }
}

char *Deg2Nmea(float fdeg, boolean is_lat) {
  long deg = fdeg * 1000000;
  bool is_negative = 0;
  if (deg < 0) is_negative = 1;

  // Use the absolute number for calculation and update the buffer at the end
  deg = labs(deg);

  unsigned long b = (deg % 1000000UL) * 60UL;
  unsigned long a = (deg / 1000000UL) * 100UL + b / 1000000UL;
  b = (b % 1000000UL) / 10000UL;

  buf[0] = '0';
  // in case latitude is a 3 digit number (degrees in long format)
  if (a > 9999) {
    snprintf(buf, 6, "%04u", a);
  } else snprintf(buf + 1, 5, "%04u", a);

  buf[5] = '.';
  snprintf(buf + 6, 3, "%02u", b);
  buf[9] = '\0';
  if (is_lat) {
    if (is_negative) {
      buf[8] = 'S';
    } else buf[8] = 'N';
    return buf + 1;
    // buf +1 because we want to omit the leading zero
  } else {
    if (is_negative) {
      buf[8] = 'W';
    } else buf[8] = 'E';
    return buf;
  }
}

/***************************************************************************************
**            Calculate frequency parts before and after decimal sign
***************************************************************************************/
SFreq GetFreq(int channel) {
  SFreq sFreq = GetFreq(channel, 80);
  return sFreq;
}

SFreq GetFreq(int channel, int steps) {
  int fMHz = floor(channel / steps) + 144;
  int fKHz = (channel - (floor(channel / steps) * steps)) * (10000 / steps);
  SFreq sFreq = { fMHz, fKHz };
  return sFreq;
}

/***************************************************************************************
**            Draw screen
***************************************************************************************/
void DrawScreen() {
  DrawScreen(false);
}

void DrawScreen(bool drawAll) {
  tft.fillScreen(TFT_BLACK);
  if (actualPage < lastPage || drawAll){
    DrawFrequency(false);
  } 
  if (actualPage == BTN_NUMERIC) DrawKeyboardNumber(false);
  DrawButtons();
  dirtyScreen = false;
}

void ShowDebugScreen(char header[]) {
  startedDebugScreen = millis();
  debugLine = 0;
  if (actualPage != debugPage) beforeDebugPage = actualPage;
  actualPage = debugPage;
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(50);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(header, 2, 10, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
}

void DrawKeyboardNumber(bool doReset) {
  tft.setTextDatum(MC_DATUM);
  sprintf(buf, "%s", FindButtonNameByID(activeBtn));
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf, 162, 15, 4);

  tft.setTextDatum(MR_DATUM);
  if (doReset) keyboardNumber = 0;
  int f1 = floor(keyboardNumber / 10000);
  int f2 = keyboardNumber - (f1 * 10000);
  int fBand = settings.isUHF ? 430 : 140;
  if (activeBtn == FindButtonIDByName("Freq")) sprintf(buf, "%01d.%04d", fBand + f1, f2);
  else sprintf(buf, "%1d", keyboardNumber);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, 282, 60, 7);
}

void DrawDebugInfo(char debugInfo[]) {
  if (beforeDebugPage > 0) tft.drawString(debugInfo, 2, 25 + (debugLine * 8), 1);
  DebugPrintln(debugInfo);
  debugLine++;
}

void Lipovolt() {
  int Yoff = 50;
  int Xoff = 5;

  tft.setTextSize(1);
  tft.setTextDatum(BC_DATUM);
  tft.fillRect(Xoff - 2 , 0 + Yoff - 6 , 14 , 32 , TFT_BLACK);

  int LipoVolt_Result = 0;
  float lipvolt = 0.0;

  LipoVolt_Result = analogRead(LipoVoltpin);
  lipvolt = readADC_Cal(LipoVolt_Result);
  lipvolt = lipvolt / 500; // 500 because millivolts and half voltage measured 

  //======== just for test to see real lipo voltage ================
  
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // tft.setCursor(0, 35);
    // tft.print(lipvolt,2);
  
  //================================================================

  tft.drawRect(Xoff - 1, Yoff - 1 , 12 , 26 , TFT_WHITE);
  tft.fillRect(Xoff + 2 , Yoff - 4,  6 , 3 , TFT_WHITE);

  if (lipvolt >= 3.9) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextPadding(tft.textWidth("100%"));
    tft.fillRect(Xoff , Yoff, 10 , 6 , TFT_GREEN);
    //tft.drawString("100%", 24 + Xoff, 7 + Yoff);
  }

  if (lipvolt >= 3.7) {
    tft.fillRect(Xoff, 6 + Yoff, 10 , 6 , TFT_YELLOW);
  }

  if (lipvolt >= 3.5) {
    tft.fillRect(Xoff, 12 + Yoff, 10 , 6 , TFT_ORANGE);
  }

  if (lipvolt >= 3.3) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextPadding(tft.textWidth("25%"));
    tft.fillRect(Xoff, 18 + Yoff, 10 , 6 , TFT_RED);
    //tft.drawString("25%", 24 + Xoff, 25 + Yoff);
  }
 
}

uint32_t readADC_Cal(int ADC_Raw) {// calibratie ADC ESP32
  esp_adc_cal_characteristics_t adc_chars;

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
/***************************************************************************************
**            Draw frequencies
***************************************************************************************/
void DrawFrequency(bool isAPRS) {
  DrawFrequency(isAPRS, true);
}

void DrawFrequency(bool isAPRS, bool doClear) {
  if (actualPage < lastPage) {
    if (doClear) tft.fillRect(0, 0, 320, 135, TFT_BLACK);
    int freq, fMHz;

    tft.setTextDatum(ML_DATUM);
    SFreq sFreq = GetFreq(settings.aprsChannel);
    sprintf(buf, "APRS:%01d.%03d, %s-%d, %s-%d,%s", sFreq.fMHz, sFreq.fKHz, settings.call, settings.ssid, settings.dest, settings.destSsid, String(settings.symbool));
    tft.setTextPadding(tft.textWidth(buf));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    if (settings.useAPRS) tft.drawString(buf, 2, 24, 1);

    tft.setTextDatum(MR_DATUM);

    if (wifiAvailable || wifiAPMode) {
      tft.setTextPadding(tft.textWidth(WiFi.SSID()));
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString(WiFi.SSID(), 315, 4, 1);
    }

    int txChannel = isAPRS ? settings.aprsChannel : settings.txChannel;
    int rxChannel = isAPRS ? settings.aprsChannel : settings.rxChannel;
    if (isPTT ^ isReverse) sFreq = GetFreq(txChannel);
    else sFreq = GetFreq(rxChannel);
    sprintf(buf, "%01d.%04d", sFreq.fMHz, sFreq.fKHz);
    tft.setTextPadding(tft.textWidth(buf));
    if (isPTT) tft.setTextColor(TFT_RED, TFT_BLACK);
    else tft.setTextColor(squelshClosed ? TFT_GOLD : TFT_GREEN, TFT_BLACK);
    tft.drawString(buf, 315, 60, 7);

    if (isPTT ^ isReverse) sFreq = GetFreq(rxChannel);
    else sFreq = GetFreq(txChannel);
    sprintf(buf, "%01d.%04d", sFreq.fMHz, sFreq.fKHz);
    tft.fillRect(100, 86, 165, 8, TFT_BLACK);
    tft.setTextPadding(tft.textWidth(repeaters[settings.repeater].name));
    tft.drawString(repeaters[settings.repeater].name, 140, 90, 1);
    tft.setTextPadding(tft.textWidth(repeaters[settings.repeater].city));
    tft.drawString(repeaters[settings.repeater].city, 220, 90, 1);
    tft.setTextPadding(tft.textWidth(buf));
    tft.drawString(buf, 315, 90, 1);

    if (isPTT) {
      tft.fillCircle(50, 60, 24, TFT_RED);
      tft.setTextDatum(MC_DATUM);
      tft.setTextPadding(tft.textWidth("TX"));
      tft.setTextColor(TFT_BLACK, TFT_RED);
      tft.drawString("TX", 50, 62, 4);
    } else tft.fillCircle(50, 60, 24, TFT_BLACK);
    DrawMeter(2, 100, 314, 30, swr, isPTT, false);
    lastMinute = -1;
    Lipovolt();
  }
}

/***************************************************************************************
**            Draw the time
***************************************************************************************/
void DrawTime() {

  // Convert UTC to local time, returns zone code in tz1_Code, e.g "GMT"
  time_t local_time = TIMEZONE.toLocal(now(), &tz1_Code);

  String timeNow = "Time:";

  if (hour(local_time) < 10) timeNow += "0";
  timeNow += hour(local_time);
  timeNow += ":";
  if (minute(local_time) < 10) timeNow += "0";
  timeNow += minute(local_time);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("Time:44:44 "));  // String width + margin
  tft.drawString(timeNow, 2, 4, 1);
}

/***************************************************************************************
**            Draw meter
***************************************************************************************/
void DrawMeter(int xPos, int yPos, int width, int height, int value, bool isTX, bool onlyBlocks) {
  //if (!isTX) value = squelshClosed ? 0 : 10;
  if (isTX) value = settings.draPower ? 9 : 4;
  if (!onlyBlocks) {
    tft.setTextDatum(MC_DATUM);
    DrawBox(xPos, yPos, width, height);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextPadding(tft.textWidth("S"));
    tft.drawString(isTX ? "P" : "S", xPos + 20, yPos + (height / 2) + 1, 4);
    tft.drawLine(xPos + 40, yPos + 25, xPos + 200, yPos + 25, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    for (int i = 0; i < 9; i++) {
      sprintf(buf, "%d", i + 1);
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, xPos + 40 + (i * 20), yPos + 8, 1);
      tft.drawLine(xPos + 40 + (i * 20), yPos + 20, xPos + 40 + (i * 20), yPos + 25, TFT_WHITE);
    }

    tft.drawLine(xPos + 200, yPos + 25, xPos + 300, yPos + 25, TFT_BLUE);
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    for (int i = 10; i < 50; i = i + 10) {
      sprintf(buf, "+%d", i);
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, xPos + 200 + (i * 2), yPos + 8, 1);
      tft.drawLine(xPos + 200 + (i * 2), yPos + 20, xPos + 200 + (i * 2), yPos + 25, TFT_BLUE);
    }

    tft.setTextPadding(tft.textWidth("dB"));
    tft.drawString("dB", xPos + 300, yPos + 8, 1);
    tft.drawLine(xPos + 300, yPos + 18, xPos + 300, yPos + 24, TFT_BLUE);
  }
  for (int i = 4; i < (14 * 4); i++) {
    uint16_t signColor = TFT_WHITE;
    signColor = (i > 35) ? TFT_RED : TFT_WHITE;
    if (i > value * 4) signColor = TFT_BLACK;
    tft.fillRect(xPos + 20 + (i * 5), yPos + 12, 4, 7, signColor);
  }
}

/***************************************************************************************
**            Draw button
**            Find buttons, button values and colors
**            Draw box
***************************************************************************************/
void DrawButtons() {
  for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    int showVal = ShowControls();
    if ((buttons[i].pageNo & showVal) > 0) DrawButton(buttons[i].name);
  }
}

void DrawButton(String btnName) {
  int showVal = ShowControls();
  Button button = FindButtonByName(btnName);
  if ((button.pageNo & showVal) > 0) {
    DrawButton(button.xPos, button.yPos, button.width, button.height, button.caption, button.waarde, button.bottomColor, button.topColor, button.name);
  }
}

void DrawButton(int xPos, int yPos, int width, int height, String caption, String waarde, uint16_t bottomColor, uint16_t topColor, String Name) {
  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);

  uint16_t gradientStartColor = TFT_BLACK;
  if (Name == FindButtonNameByID(activeBtn)) gradientStartColor = topColor;

  tft.fillRectVGradient(xPos + 2, yPos + 2, width - 4, (height / 2) + 1, gradientStartColor, topColor);
  tft.setTextPadding(tft.textWidth(caption));
  tft.setTextColor(TFT_WHITE);
  if (gradientStartColor == topColor) tft.setTextColor(TFT_BLACK);
  tft.drawString(caption, xPos + (width / 2), yPos + (height / 2) - 5, 2);

  if (Name == "Navigate") {
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(tft.textWidth("     <<     <"));
    tft.setTextColor(TFT_WHITE);
    tft.drawString("     <<     <", 5, yPos + (height / 2) - 5, 2);
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth(">     >>     "));
    tft.setTextColor(TFT_WHITE);
    tft.drawString(">     >>     ", 309, yPos + (height / 2) - 5, 2);
  }

  // tft.fillRectVGradient(xPos + 2,yPos + 2 + (height/2), width-4, (height/2)-4, TFT_BLACK, bottomColor);
  tft.fillRoundRect(xPos + 2, yPos + 2 + (height / 2), width - 4, (height / 2) - 4, 3, bottomColor);
  if (waarde != "") {
    tft.setTextPadding(tft.textWidth(waarde));
    tft.setTextColor(TFT_YELLOW);
    if (bottomColor != TFT_BLACK) tft.setTextColor(TFT_BLACK);
    if (bottomColor == TFT_RED) tft.setTextColor(TFT_WHITE);
    tft.drawString(waarde, xPos + (width / 2), yPos + (height / 2) + 9, 1);
  }
}

int ShowControls() {
  int retVal = actualPage;
  if (actualPage == 1) retVal = retVal + BTN_NAV + BTN_NEXT;
  if (actualPage > 1 && actualPage < lastPage) retVal = retVal + BTN_ARROW + BTN_NEXT + BTN_PREV;
  if (actualPage == lastPage) retVal = retVal + BTN_CLOSE;
  return retVal;
}

Button FindButtonByName(String Name) {
  for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    if (String(buttons[i].name) == Name) return FindButtonInfo(buttons[i]);
  }
  return buttons[0];
}

int8_t FindButtonIDByName(String Name) {
  for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    if (String(buttons[i].name) == Name) return i;
  }
  return -1;
}

String FindButtonNameByID(int8_t id) {
  return String(buttons[id].name);
}


Button FindButtonInfo(Button button) {
  char buttonBuf[10] = "\0";
  if (button.name == "Shift") {
    if (settings.txShift == SHIFT_NONE) {
      button.bottomColor = TFT_BLACK;
      strcpy(button.waarde, "None");
    }
    if (settings.txShift == SHIFT_NEG) {
      button.bottomColor = TFT_RED;
      strcpy(button.waarde, "-600");
    }
    if (settings.txShift == SHIFT_POS) {
      button.bottomColor = TFT_GREEN;
      strcpy(button.waarde, "600");
    }
  }

  if (button.name == "MOX") {
    if (!isMOX) {
      button.bottomColor = TFT_BLACK;
      strcpy(button.waarde, "");
    } else {
      button.bottomColor = TFT_RED;
      strcpy(button.waarde, "PTT");
    }
  }

  if (button.name == "Reverse") {
    if (!isReverse) {
      button.bottomColor = TFT_BLACK;
      strcpy(button.waarde, "");
    } else {
      button.bottomColor = TFT_RED;
      strcpy(button.waarde, "On");
    }
  }

  if (button.name == "Vol") {
    sprintf(buttonBuf, "%d", settings.volume);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "SQL") {
    sprintf(buttonBuf, "%d", settings.squelsh);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Light") {
    sprintf(buttonBuf, "%d", settings.currentBrightness);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Tone") {
    String s;
    if (settings.hasTone == TONETYPERX) s = "R ";
    if (settings.hasTone == TONETYPETX) s = "T ";
    if (settings.hasTone == TONETYPERXTX) s = "RT ";
    sprintf(buttonBuf, "%s %s", s, settings.hasTone > TONETYPENONE ? cTCSSCodes[settings.ctcssTone].tone : "");
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "RPT") {
    sprintf(buttonBuf, "%s", repeaters[settings.repeater].name);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "MEM") {
    sprintf(buttonBuf, "%d", settings.memoryChannel);
    strcpy(button.waarde, buttonBuf);
  }

  if (button.name == "Navigate") {
    sprintf(buttonBuf, "%s", FindButtonNameByID(activeBtn));
    button.caption = buttonBuf;
  }

  if (button.name == "APRS") {
    if (!settings.useAPRS) {
      button.bottomColor = TFT_BLACK;
      strcpy(button.waarde, "Off");
    } else {
      button.bottomColor = TFT_RED;
      strcpy(button.waarde, "On");
    }
  }

  if (button.name == "Power") {
    strcpy(button.waarde, settings.draPower ? "High" : "Low");
  }

  if (button.name == "Mute") {
    strcpy(button.waarde, isMuted ? "True" : "False");
  }

  if (button.name == "Scan") {
    if (settings.scanType == SCAN_TYPE_STOP) sprintf(buttonBuf, "%s", "Stop");
    if (settings.scanType == SCAN_TYPE_RESUME) sprintf(buttonBuf, "%s", "Resume");
    strcpy(button.waarde, buttonBuf);
    if (scanMode == SCAN_STOPPED) {
      button.bottomColor = TFT_BLACK;
    } else if (scanMode == SCAN_INPROCES) {
      button.bottomColor = TFT_RED;
    } else {
      button.bottomColor = TFT_GREEN;
    }
  }

  if (button.name == "SetBand") {
    if (!settings.isUHF) {
      button.bottomColor = TFT_BLACK;
      strcpy(button.waarde, "VHF");
    } else {
      button.bottomColor = TFT_DARKBLUE;
      strcpy(button.waarde, "UHF");
    }
  }

  return button;
}

void DrawBox(int xPos, int yPos, int width, int height) {
  tft.drawRoundRect(xPos + 2, yPos + 2, width, height, 4, TFT_SHADOW);
  tft.drawRoundRect(xPos, yPos, width, height, 3, TFT_WHITE);
  tft.fillRoundRect(xPos + 1, yPos + 1, width - 2, height - 2, 3, TFT_BLACK);
}

/***************************************************************************************
**            Handle button
***************************************************************************************/
void HandleFunction(String buttonName) {
  HandleFunction(buttonName, true);
}

void HandleFunction(String buttonName, bool doDraw) {
  Button button = FindButtonByName(buttonName);
  HandleFunction(button, 0, 0, doDraw);
}

void HandleFunction(String buttonName, int x, int y) {
  HandleFunction(buttonName, x, y, true);
}

void HandleFunction(String buttonName, int x, int y, bool doDraw) {
  Button button = FindButtonByName(buttonName);
  HandleFunction(button, x, y, doDraw);
}

void HandleFunction(Button button, int x, int y) {
  HandleFunction(button, x, y, true);
}

void HandleFunction(Button button, int x, int y, bool doDraw) {
  //doBeep(25);
  if (button.name == "ToRight") {
    activeBtnStart = millis();
    if (activeBtn == FindButtonIDByName("Freq")) {
      float f = (x - button.xPos);
      f = f / button.width;
      int val = pow(2, floor(f * 4));
      if (x == -1) val = 1;
      if (val == 8 && settings.isUHF) val = 40;
      SetFreq(val, 0, 0, false);
      if (doDraw) DrawFrequency(false);
      if (doDraw) DrawButton("Shift");
      if (doDraw) DrawButton("Reverse");
      delay(200);
    }
    if (activeBtn == FindButtonIDByName("Vol")) {
      if (settings.volume < 8) settings.volume++;
      SetDraVolume(settings.volume);
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    }
    if (activeBtn == FindButtonIDByName("SQL")) {
      if (settings.squelsh < 8) settings.squelsh++;
      SetFreq(0, 0, 0, false);
      if (doDraw) DrawButton("SQL");
    }
    if (activeBtn == FindButtonIDByName("Tone")) {
      if (settings.ctcssTone < (sizeof(cTCSSCodes) / sizeof(cTCSSCodes[0])) - 1) settings.ctcssTone++;
      SetFreq(0, 0, 0, false);
      if (doDraw) DrawButton("Tone");
    }
    if (activeBtn == FindButtonIDByName("Scan")) {
      if (settings.scanType < 1) settings.scanType++;
      if (doDraw) DrawButton("Scan");
    }
    if (activeBtn == FindButtonIDByName("RPT")) {
      if (settings.repeater < (sizeof(repeaters) / sizeof(repeaters[0])) - 1) {
        settings.repeater++;
        SetRepeater(settings.repeater);
        if (doDraw) DrawFrequency(false);
        if (doDraw) DrawButton("RPT");
        if (doDraw) DrawButton("Shift");
        if (doDraw) DrawButton("Tone");
      }
    }
    if (activeBtn == FindButtonIDByName("MEM")) {
      if (settings.memoryChannel < 9) {
        settings.memoryChannel++;
        SetMemory(settings.memoryChannel);
        if (doDraw) DrawFrequency(false);
        if (doDraw) DrawButton("MEM");
        if (doDraw) DrawButton("Shift");
        if (doDraw) DrawButton("Tone");
      }
    }
    if (activeBtn == FindButtonIDByName("Light")) {
      if (settings.currentBrightness < 100) settings.currentBrightness += 5;
      if (settings.currentBrightness > 100) settings.currentBrightness = 100;
      if (doDraw) DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name == "ToLeft") {
    activeBtnStart = millis();
    if (activeBtn == FindButtonIDByName("Freq")) {
      float f = button.width - (x - button.xPos);
      f = f / button.width;
      int val = pow(2, floor(f * 4));
      if (x == -1) val = 1;
      if (val == 8 && settings.isUHF) val = 40;
      SetFreq(val * -1, 0, 0, false);
      if (doDraw) DrawFrequency(false);
      if (doDraw) DrawButton("Shift");
      if (doDraw) DrawButton("Reverse");
      delay(200);
    }
    if (activeBtn == FindButtonIDByName("Vol")) {
      if (settings.volume > 0) settings.volume--;
      SetDraVolume(settings.volume);
      if (doDraw) DrawButton("Vol");
      if (doDraw) DrawButton("Mute");
    }
    if (activeBtn == FindButtonIDByName("SQL")) {
      if (settings.squelsh > 0) settings.squelsh--;
      SetFreq(0, 0, 0, false);
      if (doDraw) DrawButton("SQL");
    }
    if (activeBtn == FindButtonIDByName("Tone")) {
      if (settings.ctcssTone > 1) settings.ctcssTone--;
      SetFreq(0, 0, 0, false);
      if (doDraw) DrawButton("Tone");
    }
    if (activeBtn == FindButtonIDByName("Scan")) {
      if (settings.scanType > 0) settings.scanType--;
      if (doDraw) DrawButton("Scan");
    }
    if (activeBtn == FindButtonIDByName("RPT")) {
      if (settings.repeater > 1) {
        settings.repeater--;
        SetRepeater(settings.repeater);
        if (doDraw) DrawFrequency(false);
        if (doDraw) DrawButton("RPT");
        if (doDraw) DrawButton("Shift");
        if (doDraw) DrawButton("Tone");
      }
    }
    if (activeBtn == FindButtonIDByName("MEM")) {
      if (settings.memoryChannel > 0) {
        settings.memoryChannel--;
        SetMemory(settings.memoryChannel);
        if (doDraw) DrawFrequency(false);
        if (doDraw) DrawButton("MEM");
        if (doDraw) DrawButton("Shift");
        if (doDraw) DrawButton("Tone");
      }
    }
    if (activeBtn == FindButtonIDByName("Light")) {
      if (settings.currentBrightness > 5) settings.currentBrightness -= 5;
      if (settings.currentBrightness < 5) settings.currentBrightness = 5;
      if (doDraw) DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
    }
    if (doDraw) DrawButton("Navigate");
  }

  if (button.name == "Shift" && !settings.isUHF) {
    settings.txShift++;
    if (settings.txShift > SHIFT_POS) settings.txShift = SHIFT_NEG;
    SetFreq(0, 0, settings.txShift + 10, false);
    if (doDraw) DrawFrequency(false);
    if (doDraw) DrawButton("Shift");
    if (doDraw) DrawButton("Reverse");
    delay(200);
  }

  if (button.name == "MOX" && !settings.isUHF) {
    isMOX = !isMOX;
    if (doDraw) DrawFrequency(false);
    if (doDraw) DrawButton("MOX");
    delay(200);
  }

  if (button.name == "Beacon") {
    SendBeacon(true, false);
  }

  if (button.name == "TXBeacon") {
    SendBeaconViaRadio();
    delay(200);
  }

  if (button.name == "Vol") {
    if (activeBtn == FindButtonIDByName("Vol")) {
      keyboardNumber = settings.volume;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      activeBtn = FindButtonIDByName("Vol");
      activeBtnStart = millis();
      if (doDraw) DrawButtons();
    }
  }

  if (button.name == "SQL") {
    if (activeBtn == FindButtonIDByName("SQL")) {
      keyboardNumber = settings.squelsh;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      activeBtn = FindButtonIDByName("SQL");
      activeBtnStart = millis();
      if (doDraw) DrawButtons();
    }
  }

  if (button.name == "Freq") {
    if (activeBtn == FindButtonIDByName("Freq")) {
      SFreq sFreq = GetFreq(settings.rxChannel);
      if (!settings.isUHF) keyboardNumber = ((sFreq.fMHz - 140) * 10000) + sFreq.fKHz;
      if (settings.isUHF) keyboardNumber = ((sFreq.fMHz - 430) * 10000) + sFreq.fKHz;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      settings.repeater = 0;
      settings.txShift = 0;
      settings.ctcssTone = 0;
      settings.hasTone = TONETYPENONE;
      activeBtn = FindButtonIDByName("Freq");
      settings.freqType = FindButtonIDByName("Freq");
      if (doDraw) DrawFrequency(false);
      if (doDraw) DrawButtons();
    }
  }

  if (button.name == "Tone" && !settings.isUHF) {
    if (activeBtn == FindButtonIDByName("Tone")) settings.hasTone++;
    if (settings.hasTone > 3) settings.hasTone = 0;
    SetFreq(0, 0, 0, false);
    activeBtn = FindButtonIDByName("Tone");
    activeBtnStart = millis();
    if (doDraw) DrawButtons();
  }

  if (button.name == "RPT" && !settings.isUHF) {
    if (settings.repeater == 0) settings.repeater = 1;
    SetRepeater(settings.repeater);
    activeBtn = FindButtonIDByName("RPT");
    settings.freqType = FindButtonIDByName("RPT");
    if (doDraw) DrawFrequency(false);
    if (doDraw) DrawButtons();
  }

  if (button.name == "MEM") {
    if (activeBtn == FindButtonIDByName("MEM")) {
      keyboardNumber = settings.memoryChannel;
      actualPage = BTN_NUMERIC;
      if (doDraw) DrawScreen();
    } else {
      SetMemory(settings.memoryChannel);
      activeBtn = FindButtonIDByName("MEM");
      settings.freqType = FindButtonIDByName("MEM");
      if (doDraw) DrawFrequency(false);
      if (doDraw) DrawButtons();
    }
  }

  if (button.name == "Calibrate") {
    TouchCalibrate();
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Reverse") {
    if (settings.rxChannel != settings.txChannel) {
      isReverse = !isReverse;
      SetFreq(0, 0, 0, false);
      if (doDraw) DrawFrequency(false);
      if (doDraw) DrawButton("Shift");
      if (doDraw) DrawButton("Reverse");
      delay(200);
    } else isReverse = false;
  }

  if (button.name == "Light") {
    activeBtn = FindButtonIDByName("Light");
    activeBtnStart = millis();
    if (doDraw) DrawButtons();
  }

  if (button.name == "Next") {
    actualPage = actualPage << 1;
    lastMinute = -1;
    if (actualPage > lastPage) actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Prev") {
    lastMinute = -1;
    if (actualPage > 1) actualPage = actualPage >> 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Close") {
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "APRS") {
    settings.useAPRS = !settings.useAPRS;
    if (doDraw) DrawScreen();
    delay(200);
  }

  if (button.name == "Power") {
    settings.draPower = !settings.draPower;
    digitalWrite(HIPOWERPIN, !settings.draPower);
    if (doDraw) DrawButton("Power");
  }

  if (button.name == "Mute") {
    if (settings.volume > 0) {
      isMuted = !isMuted;
      digitalWrite(MUTEPIN, isMuted);
      if (doDraw) DrawButton("Mute");
    }
  }

  if (button.name == "Off") {
    isOn = false;
    actualPage = 1;
    DoTurnOff();
    delay(500);
  }

  if (button.name == "Scan") {
    if (scanMode != SCAN_STOPPED) {
      digitalWrite(MUTEPIN, isMuted);
      scanMode = SCAN_STOPPED;
    } else {
      if (settings.freqType == FindButtonIDByName("Freq")) settings.hasTone = TONETYPENONE;
      digitalWrite(MUTEPIN, true);
      scanMode = SCAN_INPROCES;
    }
    activeBtn = FindButtonIDByName("Scan");
    activeBtnStart = millis();
    if (doDraw) DrawButtons();
  }

  if (button.name == "SetBand") {
    settings.isUHF = !settings.isUHF;
    if (doDraw) DrawButton("SetBand");
    SetFreq(0, 0, 0, false);
    settings.freqType = FindButtonIDByName("Freq");
    settings.repeater = 0;
    settings.txShift = 0;
    settings.ctcssTone = 0;
    settings.hasTone = TONETYPENONE;
    if (doDraw) DrawScreen();
    delay(200);
  }

  if (button.name == "Save") {

    activeBtn = FindButtonIDByName("Save");
    keyboardNumber = 0;  //settings.memoryChannel;
    actualPage = BTN_NUMERIC;
    if (doDraw) DrawScreen();

    // DebugPrintln("SaveButton");
    // Memory myMemory = {settings.rxChannel, settings.txChannel, settings.repeater, settings.txShift, settings.hasTone, settings.ctcssTone};
    // memories[settings.memoryChannel] = myMemory;
    // SetMemory(settings.memoryChannel);
    // SaveMemories();
    // if (doDraw) DrawFrequency(false);
    // if (doDraw) DrawButton("MEM");
    // if (doDraw) DrawButton("Shift");
    // if (doDraw) DrawButton("Tone");
  }

  if (button.name == "Print") {
    DebugPrintln("Printing");
    for (int i = 0; i < 10; i++) {
      SFreq rxSFreq = GetFreq(memories[i].rxChannel);
      SFreq txSFreq = GetFreq(memories[i].txChannel);
      DebugPrintf("Memory %d is RX %01d.%04d and TX %01d.%04d with ctcss %d\r\n", i, rxSFreq.fMHz, rxSFreq.fKHz, txSFreq.fMHz, txSFreq.fKHz, memories[i].ctcssTone);
    }
  }

  if (String(button.name).substring(0, 3) == "A00") {
    int i = String(button.name).substring(3).toInt();
    if (activeBtn == FindButtonIDByName("Freq")) {
      keyboardNumber = (keyboardNumber * 10) + i;
      if (!settings.isUHF && keyboardNumber >= 60000) keyboardNumber = 0;
      if (settings.isUHF && keyboardNumber >= 100000) keyboardNumber = 0;
    }
    if (activeBtn == FindButtonIDByName("Vol") || activeBtn == FindButtonIDByName("SQL")) {
      keyboardNumber = i;
      if (keyboardNumber > 8) keyboardNumber = 0;
    }
    if (activeBtn == FindButtonIDByName("MEM")) {
      keyboardNumber = i;
      if (keyboardNumber > 9) keyboardNumber = 0;
    }
    if (activeBtn == FindButtonIDByName("Save")) {
      keyboardNumber = i;
      if (keyboardNumber > 9) keyboardNumber = 0;
    }
    if (doDraw) DrawKeyboardNumber(false);
  }

  if (button.name == "Enter") {
    if (activeBtn == FindButtonIDByName("Vol")) {
      settings.volume = keyboardNumber;
      SetDraVolume(settings.volume);
    }
    if (activeBtn == FindButtonIDByName("SQL")) {
      settings.squelsh = keyboardNumber;
      SetFreq(0, 0, 0, false);
    }
    if (activeBtn == FindButtonIDByName("Freq")) {
      if (!settings.isUHF) settings.rxChannel = (keyboardNumber - 40000) / 125;
      if (settings.isUHF) settings.rxChannel = (keyboardNumber / 125) + 22880;
      SetFreq(0, settings.rxChannel, 0, false);
    }
    if (activeBtn == FindButtonIDByName("MEM")) {
      settings.memoryChannel = keyboardNumber;
      SetMemory(settings.memoryChannel);
    }
    if (activeBtn == FindButtonIDByName("Save")) {
      DebugPrintln("SaveButton");
      Memory myMemory = { settings.rxChannel, settings.txChannel, settings.repeater, settings.txShift, settings.hasTone, settings.ctcssTone };
      settings.memoryChannel = keyboardNumber;
      memories[settings.memoryChannel] = myMemory;
      SetMemory(settings.memoryChannel);
      SaveMemories();
      if (doDraw) DrawFrequency(false);
      if (doDraw) DrawButton("MEM");
      if (doDraw) DrawButton("Shift");
      if (doDraw) DrawButton("Tone");
    }
    actualPage = 1;
    if (doDraw) DrawScreen();
  }

  if (button.name == "Clear") {
    actualPage = 1;
    if (doDraw) DrawScreen();
  }
  if (!doDraw) dirtyScreen = true;
}
/***************************************************************************************
**            Turn off
***************************************************************************************/
void DoTurnOff() {
  ledcWrite(ledChannelforTFT, 255);
  tft.fillScreen(TFT_BLACK);
  SetDraVolume(0);
  digitalWrite(TRXONPIN, 1);  // high TRX Off
}

/***************************************************************************************
**            Calibrate touch
***************************************************************************************/
void TouchCalibrate() {
  esp_task_wdt_reset();
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");
  esp_task_wdt_reset();
  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
  esp_task_wdt_reset();
  DebugPrintln("Calibrate started");
  DebugPrint("calData[5] = { ");
  for (uint8_t i = 0; i < 5; i++) {
    DebugPrint(calData[i]);
    if (i < 4) DebugPrint(", ");
  }
  DebugPrintln(" };");
  esp_task_wdt_reset();
  settings.calData0 = calData[0];
  settings.calData1 = calData[1];
  settings.calData2 = calData[2];
  settings.calData3 = calData[3];
  settings.calData4 = calData[4];
  if (SaveConfig()) Serial.println("Calibration data saved");
  esp_task_wdt_reset();
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  esp_task_wdt_reset();
  delay(2000);
}

/***************************************************************************************
**            Set DRA
***************************************************************************************/
void SetRepeater(int repeater) {
  settings.rxChannel = repeaters[repeater].channel;
  settings.txShift = repeaters[repeater].shift;
  settings.ctcssTone = repeaters[repeater].ctcssTone;
  settings.hasTone = repeaters[repeater].hasTone;
  SetFreq(0, settings.rxChannel, settings.txShift + 10, false);
}

void SetMemory(int channel) {
  settings.repeater = memories[channel].repeater;
  settings.rxChannel = memories[channel].rxChannel;
  settings.txChannel = memories[channel].txChannel;
  settings.txShift = memories[channel].txShift;
  settings.ctcssTone = memories[channel].ctcssTone;
  settings.hasTone = memories[channel].hasTone;
  settings.isUHF = memories[channel].rxChannel > settings.minUHFChannel;
  SetFreq(0, settings.rxChannel, settings.txShift + 10, false);
}

void SetFreq(float freq) {
  freq = freq / 10000;
  int channel = ((freq - 144) * 1000) / 12.5;
  settings.isUHF = (channel >= settings.minUHFChannel && channel <= settings.maxUHFChannel);
  SetFreq(0, channel, 0, false);
}

void SetFreq(int step, int channel, uint8_t txShift, bool isAPRS) {
  if (isAPRS) {
    SetDra(settings.aprsChannel, settings.aprsChannel, 0, 0, settings.squelsh);
  } else {
    if (settings.isUHF && settings.rxChannel < settings.minUHFChannel) {
      settings.rxChannel = settings.minUHFChannel;
      settings.txChannel = settings.rxChannel;
      settings.repeater = 0;
      settings.ctcssTone = 0;
      settings.hasTone = TONETYPENONE;
    }
    if (!settings.isUHF && settings.rxChannel > settings.maxChannel) {
      settings.rxChannel = 0;
      settings.txChannel = settings.rxChannel;
      settings.repeater = 0;
    }

    if (step != 0 || channel != 0 || txShift != 0) {
      if (step != 0) settings.rxChannel += step;
      if (channel != 0) settings.rxChannel = channel;
      if (!settings.isUHF) {
        if (settings.rxChannel > settings.maxChannel) settings.rxChannel = settings.maxChannel;
        if (settings.rxChannel < 0) settings.rxChannel = 0;
      } else {
        if (settings.rxChannel > settings.maxUHFChannel) settings.rxChannel = settings.maxUHFChannel;
        if (settings.rxChannel < settings.minUHFChannel) settings.rxChannel = settings.minUHFChannel;
      }

      if (settings.autoShift) settings.txShift = (settings.rxChannel > 125 && settings.rxChannel < 145) ? SHIFT_NEG : SHIFT_NONE;

      if (txShift > 8) settings.txShift = txShift - 10;
      settings.txChannel = settings.rxChannel + (settings.txShift) * 48;

      if (!settings.isUHF) {
        if (settings.txChannel > settings.maxChannel || settings.txChannel < 0) {
          settings.txShift++;
          if (!txShift) settings.txShift = SHIFT_NONE;
          if (settings.txShift > SHIFT_POS) settings.txShift = SHIFT_NEG;
          settings.txChannel = settings.rxChannel + (settings.txShift) * 48;
        }
      } else {
        settings.txChannel = settings.rxChannel;
      }

      if (settings.rxChannel == settings.txChannel) isReverse = false;
    }
    SetDra(settings.rxChannel, settings.txChannel, (settings.hasTone == TONETYPERX || settings.hasTone == TONETYPERXTX) ? settings.ctcssTone : TONETYPENONE, (settings.hasTone == TONETYPETX || settings.hasTone == TONETYPERXTX) ? settings.ctcssTone : TONETYPENONE, settings.squelsh);
  }
}

void SetDra(uint16_t rxFreq, uint16_t txFreq, byte rxTone, byte txTone, byte squelsh) {
  if (settings.disableRXTone) rxTone = 0;
  SFreq rxSFreq = GetFreq(isReverse ? txFreq : rxFreq);
  SFreq txSFreq = GetFreq(isReverse ? rxFreq : txFreq);
  sprintf(buf, "AT+DMOSETGROUP=0,%01d.%04d,%01d.%04d,%04d,%01d,%04d", txSFreq.fMHz, txSFreq.fKHz, rxSFreq.fMHz, rxSFreq.fKHz, txTone, squelsh, rxTone);
  for (int x = 0; x < settings.repeatSetDRA; x++) {
    DebugPrintln();
    DebugPrintln(buf);
    DRASerial.println(buf);
    delay(50);
  }
}

void SetDraVolume(byte volume) {
  isMuted = volume > 0 ? false : true;
  digitalWrite(MUTEPIN, isMuted);
  sprintf(buf, "AT+DMOSETVOLUME=%01d", volume);
  DebugPrintln();
  DebugPrintln(buf);
  DRASerial.println(buf);
  delay(50);
}

void GetDraRSSI() {
  sprintf(buf, "RSSI?");
  DRASerial.println(buf);
  delay(50);
}

void SetDraSettings() {
  sprintf(buf, "AT+SETFILTER=%01d,%01d,%01d", settings.preEmphase, settings.highPass, settings.lowPass);
  DebugPrintln();
  DebugPrint("Filter:");
  DebugPrintln(buf);
  DRASerial.println(buf);
  delay(50);

  // sprintf(buf, "AT+DMOSETMIC=%01d,0", settings.mikeLevel);
  // DebugPrintln();
  // DebugPrint("Mikelevel:");
  // DebugPrintln(buf);
  // DRASerial.println(buf);
}

void ReadDRAPort() {
  bool endLine = false;
  while (DRASerial.available() && !endLine) {
    char draChar = DRASerial.read();
    draBuffer[draBufferLength++] = draChar;
    if (draChar == 0xA) {
      draBuffer[draBufferLength++] = '\0';
      if (strcmp(draBuffer, "RSSI")) {
        swr = (atoi(&draBuffer[5]));
        //swr = swr - 24;
        if (swr < 55) {
          swr = swr / 6;
        } else {
          swr = 9 + ((swr - 54) / 10);
        }
        if (swr > 14) swr = 14;
        if (squelshClosed && swr > 2) swr = 2;
        //DebugPrintf("SWR=%d\r\n",swr);
      }
      draBuffer[0] = '\0';
      draBufferLength = 0;
      endLine = true;
    }
  }
}

void readGPSData() {
  if (GPSSerial.available()) {
    while (GPSSerial.available()) {
      //Serial.write(GPSSerial.read());
      gps.encode(GPSSerial.read());
    }
    //DebugPrintln("GPS Data received");
  }
}

/***************************************************************************************
**            Print PrintGPSInfo
***************************************************************************************/
void PrintGPSInfo() {
  if (actualPage < lastPage) {
    char sz[80];
    tft.setTextDatum(ML_DATUM);
    sprintf(sz, "GPS :LAT:%s, LON:%s, Speed:%s KM, Age:%s     ", String(gps.location.lat(), 4), String(gps.location.lng(), 4), String(gps.speed.isValid() ? gps.speed.kmph() : 0, 0), gps.location.age() > 5000 ? "Inv." : String(gps.location.age()));
    tft.setTextPadding(tft.textWidth(sz));
    if (gps.location.age() > 5000) tft.setTextColor(TFT_RED, TFT_BLACK);
    else tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString(sz, 2, 14, 1);
  }

  if (actualPage == lastPage) {
    char sz[50];
    tft.fillRect(2, 2, 320, 200, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(50);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);

    tft.drawString("GPS:", 2, 10, 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    String GPSValid = gps.location.isValid() ? "true" : "false";
    tft.drawString("Valid       :" + GPSValid, 2, 25, 1);
    tft.drawString("Lat         :" + String(gps.location.lat(), 6), 2, 33, 1);
    tft.drawString("Lon         :" + String(gps.location.lng(), 6), 2, 41, 1);
    tft.drawString("Age         :" + String(gps.location.age()), 2, 49, 1);
    sprintf(sz, "Date & Time :%02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
    tft.drawString(sz, 2, 57, 1);
    tft.drawString("Height      :" + String(gps.altitude.meters()), 2, 65, 1);
    tft.drawString("Course      :" + String(gps.course.deg()), 2, 73, 1);
    tft.drawString("Speed       :" + String(gps.speed.isValid() ? gps.speed.kmph() : 0), 2, 81, 1);
    sprintf(sz, "Course valid:%s", gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "***");
    tft.drawString(sz, 2, 89, 1);

    if (wifiAvailable || wifiAPMode) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString("WiFi:", 2, 105, 2);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("SSID        :" + String(WiFi.SSID()), 2, 120, 1);
      sprintf(sz, "IP Address  :%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      tft.drawString(sz, 2, 128, 1);
      tft.drawString("RSSI        :" + String(WiFi.RSSI()), 2, 136, 1);
    }
  }
}

void PrintTXTLine() {
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString(buf, 2, 34, 1);
}

/***************************************************************************************
**            EEPROM Routines
***************************************************************************************/

bool SaveConfig() {
  bool commitEeprom = false;
  for (unsigned int t = 0; t < sizeof(settings); t++) {
    if (*((char *)&settings + t) != EEPROM.read(offsetEEPROM + t)) {
      EEPROM.write(offsetEEPROM + t, *((char *)&settings + t));
      commitEeprom = true;
    }
  }
  if (commitEeprom) EEPROM.commit();
  return true;
}

bool LoadConfig() {
  bool retVal = true;
  if (EEPROM.read(offsetEEPROM + 0) == settings.chkDigit) {
    for (unsigned int t = 0; t < sizeof(settings); t++)
      *((char *)&settings + t) = EEPROM.read(offsetEEPROM + t);
  } else retVal = false;
  DebugPrintln("Configuration:" + retVal ? "Loaded" : "Not loaded");
  DebugPrintf("CalData[0,1,2,3,4]=%d,%d,%d,%d,%d", settings.calData0,settings.calData1,settings.calData2,settings.calData3,settings.calData4);
  return retVal;
}

bool SaveMemories() {
  for (unsigned int t = 0; t < sizeof(memories); t++)
    EEPROM.write(offsetEEPROM + sizeof(settings) + 10 + t, *((char *)&memories + t));
  EEPROM.commit();
  DebugPrintln("Memories:saved");
  return true;
}

bool LoadMemories() {
  bool retVal = true;
  for (unsigned int t = 0; t < sizeof(memories); t++)
    *((char *)&memories + t) = EEPROM.read(offsetEEPROM + sizeof(settings) + 10 + t);
  DebugPrintln("Memories:" + retVal ? "Loaded" : "Not loaded");
  return retVal;
}
/***************************************************************************************
**            APRS/IP Routines
***************************************************************************************/
bool APRSGatewayConnect() {
  char c[20];
  sprintf(c, "%s", WiFi.status() == WL_CONNECTED ? "WiFi Connected" : "WiFi NOT Connected");
  DrawDebugInfo(c);
  if (wifiAvailable && (WiFi.status() != WL_CONNECTED)) {
    aprsGatewayConnected = false;
    if (!Connect2WiFi()) wifiAvailable = false;
  } else DebugPrintln("WiFi available and WiFiStatus connected");
  if (wifiAvailable) {
    if (!aprsGatewayConnected) {
      DrawDebugInfo("Connecting to APRS server...");
      if (httpNet.connect(settings.aprsIP, settings.aprsPort)) {
        if (ReadHTTPNet()) DrawDebugInfo(httpBuf);
        sprintf(buf, "user %s-%d pass %s vers ", settings.call, settings.serverSsid, settings.aprsPassword, VERSION);
        DrawDebugInfo(buf);
        httpNet.println(buf);
        if (ReadHTTPNet()) {
          if (strstr(httpBuf, " verified")) {
            DrawDebugInfo(httpBuf);
            DrawDebugInfo("Connected to APRS.FI");
            aprsGatewayConnected = true;
          } else DrawDebugInfo("Not connected to APRS.FI");
        } else DrawDebugInfo("No response from ReadHTTPNet");
      } else DrawDebugInfo("Failed to connect to APRS.FI, server unavailable");
    } else DrawDebugInfo("Already connected to APRS.FI");
  } else DrawDebugInfo("Failed to connect to APRS.FI, WiFi not available");
  return aprsGatewayConnected;
}

void APRSGatewayUpdate() {
  DrawDebugInfo("aprsGatewayUpdate:");
  sprintf(buf, "Date & Time :%02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
  DrawDebugInfo(buf);
  if (APRSGatewayConnect()) {
    DrawDebugInfo("Update IGate info on APRS");
    sprintf(buf, "%s-%d", settings.call, settings.serverSsid);
    DrawDebugInfo(buf);
    httpNet.print(buf);

    sprintf(buf, ">APRS,TCPIP*:@%02d%02d%02dz", gps.date.day(), gps.time.hour(), gps.time.minute());
    DrawDebugInfo(buf);
    httpNet.print(buf);
    String sLat = Deg2Nmea(settings.lat, true);
    String sLon = Deg2Nmea(settings.lon, false);
    sprintf(buf, "%s/%s", sLat, sLon);
    DrawDebugInfo(buf);
    httpNet.print(buf);

    sprintf(buf, "I/A=000012 %s", INFO);
    DrawDebugInfo(buf);
    httpNet.println(buf);
    if (!ReadHTTPNet()) aprsGatewayConnected = false;
    ;
  } else DrawDebugInfo("APRS Gateway not connected");
}

bool ReadHTTPNet() {
  httpBuf[0] = '\0';
  bool retVal = false;
  long timeOut = millis();
  while (!httpNet.available() && millis() - timeOut < 2500) {}

  if (httpNet.available()) {
    int i = 0;
    while (httpNet.available() && i < 118) {
      char c = httpNet.read();
      httpBuf[i++] = c;
    }
    retVal = true;
    httpBuf[i++] = '\0';
    // DebugPrint("Read from HTTP:");
    // DebugPrint(httpBuf);
  } else {
    DebugPrint("No data returned from HTTP");
  }
  DebugPrintln();
  return retVal;
}

/***************************************************************************************
**            API Functions
***************************************************************************************/
void NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

/***************************************************************************************
**                          Draw messagebox with message
***************************************************************************************/
bool questionBox(const char *msg, uint16_t fgcolor, uint16_t bgcolor, int x, int y, int w, int h) {
  uint16_t current_textcolor = tft.textcolor;
  uint16_t current_textbgcolor = tft.textbgcolor;

  tft.loadFont(AA_FONT_SMALL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(fgcolor, bgcolor);
  tft.fillRoundRect(x, y, w, h, 5, fgcolor);
  tft.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 5, bgcolor);
  tft.setTextPadding(tft.textWidth(msg));
  tft.drawString(msg, x + 4 + w/2, y + (h / 4));

  tft.fillRoundRect(x + 4, y + (h/2) - 2, (w - 12)/2, (h - 4)/2, 5, TFT_GREEN);
  tft.setTextColor(fgcolor, TFT_GREEN);
  tft.setTextPadding(tft.textWidth("Yes"));
  tft.drawString("Yes", x + 4 + ((w - 12)/4),y + (h/2) - 2 + (h/4));
  tft.fillRoundRect(x + (w/2) + 2, y + (h/2) - 2, (w - 12)/2, (h - 4)/2, 5, TFT_RED);
  tft.setTextColor(fgcolor, TFT_RED);
  tft.setTextPadding(tft.textWidth("No"));
  tft.drawString("No", x + (w/2) + 2 + ((w - 12)/4),y + (h/2) - 2 + (h/4));
  Serial.printf("Yes = x:%d,y:%d,w:%d,h:%d\r\n",x + 4, y + (h/2) - 2, (w - 12)/2, (h - 4)/2);
  Serial.printf("No  = x:%d,y:%d,w:%d,h:%d\r\n",x + (w/2) + 2, y + (h/2) - 2, (w - 12)/2, (h - 4)/2);
  tft.setTextColor(current_textcolor, current_textbgcolor);
  tft.unloadFont();

  uint16_t touchX = 0, touchY = 0;

  long startWhile = millis();
  while (millis()-startWhile<30000) {
    bool pressed = tft.getTouch(&touchX, &touchY);
    if (pressed){
      Serial.printf("Pressed = x:%d,y:%d\r\n",touchX,touchY);
      if (touchY>=y + (h/2) - 2 && touchY<=y + (h/2) - 2 + ((h - 4)/2)){
        if (touchX>=x + 4 && touchX<=x + 4 + ((w - 12)/2)) return true;
        if (touchX>=x + (w/2) + 2 && touchX<=x + (w/2) + 2 + ((w - 12)/2)) return false;
      }
    }
  }
  return false;
}

/***************************************************************************************
**            HTML Server functions
***************************************************************************************/
void FillAPRSInfo() {
  buf[0] = '\0';
  SFreq sFreq = GetFreq(settings.aprsChannel);
  sprintf(buf, "APRS:%01d.%03d, %s-%d, %s-%d", sFreq.fMHz, sFreq.fKHz, settings.call, settings.ssid, settings.dest, settings.destSsid);
}

void FillGPSInfo() {
  buf[0] = '\0';
  sprintf(buf, "GPS :LAT:%s, LON:%s, Speed:%s KM, Age:%s     ", String(gps.location.lat(), 4), String(gps.location.lng(), 4), String(gps.speed.isValid() ? gps.speed.kmph() : 0, 0), gps.location.age() > 5000 ? "Inv." : String(gps.location.age()));
}

void FillRXFREQ() {
  buf[0] = '\0';
  SFreq sFreq;
  if (isPTT ^ isReverse) sFreq = GetFreq(settings.txChannel);
  else sFreq = GetFreq(settings.rxChannel);
  sprintf(buf, "<h1 style=\"text-align:center;color:%s\">%S%01d.%04d</h1>", isPTT ? "red" : squelshClosed ? "yellow"
                                                                                                          : "green",
          isPTT ? "TX " : squelshClosed ? "   "
                                        : "RX ",
          sFreq.fMHz, sFreq.fKHz);
}

void FillTXFREQ() {
  buf[0] = '\0';
  if (settings.rxChannel != settings.txChannel) {
    SFreq sFreq;
    if (isPTT ^ isReverse) sFreq = GetFreq(settings.rxChannel);
    else sFreq = GetFreq(settings.txChannel);
    sprintf(buf, "%01d.%04d", sFreq.fMHz, sFreq.fKHz);
  } else sprintf(buf, "&nbsp");
}

void FillKEYBFREQ() {
  buf[0] = '\0';
  SFreq sFreq;
  if (isPTT ^ isReverse) sFreq = GetFreq(settings.txChannel);
  else sFreq = GetFreq(settings.rxChannel);
  sprintf(buf, "%01d.%04d", sFreq.fMHz, sFreq.fKHz);
}

void FillREPEATERInfo() {
  buf[0] = '\0';
  sprintf(buf, "%s %s", repeaters[settings.repeater].name, repeaters[settings.repeater].city);
}

void RefreshWebPage() {
  if (wifiAvailable || wifiAPMode) {
    events.send("ping", NULL, millis());
    FillAPRSInfo();
    events.send(buf, "APRSINFO", millis());
    FillGPSInfo();
    events.send(buf, "GPSINFO", millis());
    FillREPEATERInfo();
    events.send(buf, "REPEATERINFO", millis());
    buf[0] = '\0';
    events.send(buf, "BEACONINFO", millis());
    FillRXFREQ();
    events.send(buf, "RXFREQ", millis());
    FillTXFREQ();
    events.send(String(buf).c_str(), "TXFREQ", millis());
    buf[0] = '\0';
    sprintf(buf, "%d", swr);
    events.send(buf, "SWRINFO", millis());
  }
}

void ClearButtons() {
  buf[0] = '\0';
  sprintf(buf, "<div class=\"content\"><div class=\"cards\">%BUTTONS0%</div></div>");
  events.send(String(buf).c_str(), "BUTTONSERIE", millis());
}

String Processor(const String &var) {
  if (var == "APRSINFO") {
    FillAPRSInfo();
    return buf;
  }
  if (var == "GPSINFO") {
    FillGPSInfo();
    return buf;
  }
  if (var == "BEACONINFO") {
    buf[0] = '\0';
    return buf;
  }
  if (var == "RXFREQ") {
    FillRXFREQ();
    return buf;
  }
  if (var == "TXFREQ") {
    FillTXFREQ();
    return buf;
  }
  if (var == "KEYBFREQ") {
    FillKEYBFREQ();
    return buf;
  }
  if (var == "REPEATERINFO") {
    FillREPEATERInfo();
    return buf;
  }

  if (var == "wifiSSID") return settings.wifiSSID;
  if (var == "wifiPass") return settings.wifiPass;
  if (var == "aprsChannel") return String(settings.aprsChannel);
  if (var == "aprsFreq") {
    SFreq sFreq = GetFreq(settings.aprsChannel);
    sprintf(buf, "%01d.%04d", sFreq.fMHz, sFreq.fKHz);
    return buf;
  }
  if (var == "aprsIP") return String(settings.aprsIP);
  if (var == "aprsPort") return String(settings.aprsPort);
  if (var == "aprsPassword") return String(settings.aprsPassword);
  if (var == "serverSsid") return String(settings.serverSsid);
  if (var == "aprsGatewayRefreshTime") return String(settings.aprsGatewayRefreshTime);
  if (var == "call") return String(settings.call);
  if (var == "ssid") return String(settings.ssid);
  if (var == "symbool") {
    char x = settings.symbool;
    return String(x);
  }
  if (var == "dest") return String(settings.dest);
  if (var == "destSsid") return String(settings.destSsid);
  if (var == "path1") return String(settings.path1);
  if (var == "path1Ssid") return String(settings.path1Ssid);
  if (var == "path2") return String(settings.path2);
  if (var == "path2Ssid") return String(settings.path2Ssid);
  if (var == "comment") return settings.comment;
  if (var == "interval") return String(settings.interval);
  if (var == "multiplier") return String(settings.multiplier);
  if (var == "power") return String(settings.power);
  if (var == "height") return String(settings.height);
  if (var == "gain") return String(settings.gain);
  if (var == "directivity") return String(settings.directivity);
  if (var == "autoShift") return settings.autoShift ? "checked" : "";
  if (var == "disableRXTone") return settings.disableRXTone ? "checked" : "";
  if (var == "bcnAfterTX") return settings.bcnAfterTX ? "checked" : "";
  if (var == "txTimeOut") return String(settings.txTimeOut);
  if (var == "lat") return String(settings.lat, 6);
  if (var == "lon") return String(settings.lon, 6);
  if (var == "maxChannel") return String(settings.maxChannel);
  if (var == "maxFreq") {
    SFreq sFreq = GetFreq(settings.maxChannel);
    sprintf(buf, "%01d.%04d", sFreq.fMHz, sFreq.fKHz);
    return buf;
  }
  if (var == "mikeLevel") return String(settings.mikeLevel);
  if (var == "repeatSetDRA") return String(settings.repeatSetDRA);
  if (var == "isDebug") return settings.isDebug ? "checked" : "";
  if (var == "doRotate") return settings.doRotate ? "checked" : "";

  if (var >= "REPEATERS" && var <= "REPEATERS99") {
    buf[0] = '\0';
    int i = var.substring(9).toInt();
    if (i + 1 < (sizeof(repeaters) / sizeof(repeaters[0]))) {
      sprintf(buf, "<option value=\"%d\">%s/%s</option>", i, repeaters[i].name, repeaters[i].city);
      char buf2[10];
      sprintf(buf2, "%%REPEATERS%d%%", i + 1);
      strcat(buf, buf2);
    }
    return buf;
  }

  if (var >= "BUTTONS" && var <= "BUTTONS99") {
    buf[0] = '\0';
    int i = var.substring(7).toInt();
    if (i + 1 < (sizeof(buttons) / sizeof(buttons[0]))) {
      Button button = FindButtonInfo(buttons[i]);
      if ((button.pageNo < lastPage || button.pageNo == BTN_ARROW) && button.name != "MOX") {
        sprintf(buf, "<div id=\"BTN%s\" class=\"card\" style=\"background-color:%s; border:solid; border-radius: 1em; background-image: linear-gradient(%s)\"><p><a href=\"/command?button=%s\">%s</a></p><p style=\"background-color:%s;color:white\"><span class=\"reading\"><span id=\"%s\">%s</span></span></p></div>",
                button.name,
                String(button.name) == FindButtonNameByID(activeBtn) ? "white" : "lightblue",
                String(button.name) == FindButtonNameByID(activeBtn) ? "white, #6781F1" : "#6781F1, #ADD8E6",
                button.name,
                button.caption,
                button.bottomColor == TFT_RED ? "red" : button.bottomColor == TFT_GREEN ? "green"
                                                                                        : "blue",
                button.name,
                button.waarde);
      }
      char buf2[10];
      sprintf(buf2, "%%BUTTONS%d%%", i + 1);
      strcat(buf, buf2);
    }
    return buf;
  }

  if (var >= "NUMMERS0" && var <= "NUMMERS99") {
    buf[0] = '\0';
    int i = var.substring(7).toInt();
    if (i + 1 < (sizeof(buttons) / sizeof(buttons[0]))) {
      Button button = FindButtonInfo(buttons[i]);
      if (button.pageNo == BTN_NUMERIC) {
        sprintf(buf, "<div id=\"BTN%s\" class=\"card\" style=\"border:solid; border-color: black; border-radius: 1em; background-image: linear-gradient(%s)\"><p>%s</p><p style=\"background-color:%s;color:white\"><span class=\"reading\"><span id=\"%s\">%s</span></span></p></div>",
                button.name,
                "#6781F1, blue",
                button.caption,
                "blue",
                button.name,
                button.waarde);
      }
      char buf2[10];
      sprintf(buf2, "%%NUMMERS%d%%", i + 1);
      //DebugPrintln(buf);
      strcat(buf, buf2);
    }
    return buf;
  }

  return var;
}

void SaveSettings(AsyncWebServerRequest *request) {
  if (request->hasParam("wifiSSID")) request->getParam("wifiSSID")->value().toCharArray(settings.wifiSSID, 25);
  if (request->hasParam("wifiPass")) request->getParam("wifiPass")->value().toCharArray(settings.wifiPass, 25);
  if (request->hasParam("aprsChannel")) settings.aprsChannel = request->getParam("aprsChannel")->value().toInt();
  if (request->hasParam("aprsIP")) request->getParam("aprsIP")->value().toCharArray(settings.aprsIP, 25);
  if (request->hasParam("aprsPort")) settings.aprsPort = request->getParam("aprsPort")->value().toInt();
  if (request->hasParam("aprsPassword")) request->getParam("aprsPassword")->value().toCharArray(settings.aprsPassword, 6);
  if (request->hasParam("serverSsid")) settings.serverSsid = request->getParam("serverSsid")->value().toInt();
  if (request->hasParam("aprsGatewayRefreshTime")) settings.aprsGatewayRefreshTime = request->getParam("aprsGatewayRefreshTime")->value().toInt();
  if (request->hasParam("call")) request->getParam("call")->value().toCharArray(settings.call, 8);
  if (request->hasParam("ssid")) settings.ssid = request->getParam("ssid")->value().toInt();
  //if (request->hasParam("symbool")) DrawDebugInfo(request->getParam("symbol")->value());
  if (request->hasParam("dest")) request->getParam("dest")->value().toCharArray(settings.dest, 8);
  if (request->hasParam("destSsid")) settings.destSsid = request->getParam("destSsid")->value().toInt();
  if (request->hasParam("path1")) request->getParam("path1")->value().toCharArray(settings.path1, 8);
  if (request->hasParam("path1Ssid")) settings.path1Ssid = request->getParam("path1Ssid")->value().toInt();
  if (request->hasParam("path2")) request->getParam("path2")->value().toCharArray(settings.path2, 8);
  if (request->hasParam("path2Ssid")) settings.path2Ssid = request->getParam("path2Ssid")->value().toInt();
  if (request->hasParam("comment")) request->getParam("comment")->value().toCharArray(settings.comment, 16);
  if (request->hasParam("interval")) settings.interval = request->getParam("interval")->value().toInt();
  if (request->hasParam("multiplier")) settings.multiplier = request->getParam("multiplier")->value().toInt();
  if (request->hasParam("power")) settings.power = request->getParam("power")->value().toInt();
  if (request->hasParam("height")) settings.height = request->getParam("height")->value().toInt();
  if (request->hasParam("gain")) settings.gain = request->getParam("gain")->value().toInt();
  if (request->hasParam("directivity")) settings.directivity = request->getParam("directivity")->value().toInt();
  settings.autoShift = request->hasParam("autoShift");
  settings.disableRXTone = request->hasParam("disableRXTone");
  settings.bcnAfterTX = request->hasParam("bcnAfterTX");
  if (request->hasParam("txTimeOut")) settings.txTimeOut = request->getParam("txTimeOut")->value().toInt();
  if (request->hasParam("lat")) settings.lat = request->getParam("lat")->value().toFloat();
  if (request->hasParam("lat")) settings.lon = request->getParam("lon")->value().toFloat();
  if (request->hasParam("maxChannel")) settings.maxChannel = request->getParam("maxChannel")->value().toInt();
  if (request->hasParam("mikeLevel")) settings.mikeLevel = request->getParam("mikeLevel")->value().toInt();
  if (request->hasParam("repeatSetDRA")) settings.repeatSetDRA = request->getParam("repeatSetDRA")->value().toInt();
  settings.isDebug = request->hasParam("isDebug");
  settings.doRotate = request->hasParam("doRotate");
}