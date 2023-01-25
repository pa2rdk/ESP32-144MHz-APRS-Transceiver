// V0.9AB
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
//  |   Vcc      |     3V3          |  pin 33 with 18K to 3.3 volt and 18K to ground.
//  |   GND      |     GND          |  pin 32 (Beeper) via 2K to base V1  BC547
//  |   CS       |     15           |  Collector via beeper to 5v
//  |   Reset    |      4           |  Emmitor to ground
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

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>       // https://github.com/Bodmer/TFT_eSPI
#include <WiFi.h>
#include <WifiMulti.h>
#include <EEPROM.h>
#include "NTP_Time.h"
#include <TinyGPS++.h>
#include <LibAPRS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define offsetEEPROM       32
#define EEPROM_SIZE        2048
#define AA_FONT_SMALL "fonts/NotoSansBold15"      // 15 point sans serif bold
#define AA_FONT_LARGE "fonts/NotoSansBold36"      // 36 point sans serif bold
#define VERSION       "PA2RDK_IGATE_TCP"
#define INFO          "Arduino PARDK IGATE"

#define TFT_GREY 0x5AEB
#define TFT_LIGTHYELLOW 0xFF10
#define TFT_DARKBLUE 0x016F
#define TFT_SHADOW 0xE71C
#define TFT_BUTTONCOLOR 0xB5FE

#define BTN_NAV        32768
#define BTN_NEXT       16384
#define BTN_PREV        8192
#define BTN_CLOSE       4096
#define BTN_ARROW       2048
#define BTN_NUMERIC     1024

#define RXD2            16
#define TXD2            17
#define PTTIN           27
#define PTTOUT          33
#define BTNSMIKEPIN     35
#define SQUELSHPIN      32
#define TRXONPIN        12
#define DISPLAYLEDPIN   14
#define HIPOWERPIN      13
#define MUTEPIN         22

#define ADC_REFERENCE REF_3V3
#define OPEN_SQUELCH false

#define SCAN_STOPPED      0
#define SCAN_INPROCES     1
#define SCAN_PAUSED       2
#define SCAN_TYPE_STOP    0
#define SCAN_TYPE_RESUME  1
#define SHIFT_POS         1
#define SHIFT_NONE        0
#define SHIFT_NEG         -1
#define TONETYPENONE      0
#define TONETYPERX        1
#define TONETYPETX        2
#define TONETYPERXTX      3

typedef struct  // WiFi Access
{
  const char *SSID;
  const char *PASSWORD;
} wlanSSID;

typedef struct  // Frequency parts
{
  int fMHz;
  int fKHz;
} SFreq;

typedef struct // Buttons
{
    const char *name;     // Buttonname
    const char *caption;  // Buttoncaption
    char waarde[12];  // Buttontext
    uint16_t pageNo;
    uint16_t xPos;          
    uint16_t yPos; 
    uint16_t width;
    uint16_t height;
    uint16_t btnColor;
    uint16_t bckColor;
} Button;

const Button buttons[] = {
    {"ToLeft","<<","",  BTN_ARROW,  2,208, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"ToRight",">>","", BTN_ARROW,242,208, 74,30, TFT_BLUE, TFT_BUTTONCOLOR}, 

    {"Vol","Vol","",            1,  2,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"SQL","SQL","",            1, 82,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Scan","Scan","",          1,162,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Off","Off","",            1,242,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"Freq","Freq","",          1,  2,172, 74,30, TFT_BLUE, TFT_WHITE},
    {"RPT","RPT","",            1, 82,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"MEM","MEM","",            1,162,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"Shift","Shift","",        2,  2,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Mute","Mute","",          2, 82,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Tone","Tone","",          2,162,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Reverse","Reverse","",    2,242,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"MOX","MOX","",            2, 82,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"APRS","APRS","",          2,162,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Beacon","Beacon","",      2, 82,208, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"TXBeacon","TX Beacon","", 2,162,208, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},

    {"Power","Power","",        4,  2,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"SetBand","Band","",       4, 82,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Light","Light","",        4,162,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Calibrate","Calibrate","",4,242,136, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},   

    {"A001","1","",     BTN_NUMERIC, 42,100,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A002","2","",     BTN_NUMERIC,122,100,74,30, TFT_BLUE, TFT_BUTTONCOLOR},    
    {"A003","3","",     BTN_NUMERIC,202,100,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A004","4","",     BTN_NUMERIC, 42,136,74,30, TFT_BLUE, TFT_BUTTONCOLOR},  
    {"A005","5","",     BTN_NUMERIC,122,136,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A006","6","",     BTN_NUMERIC,202,136,74,30, TFT_BLUE, TFT_BUTTONCOLOR},    
    {"A007","7","",     BTN_NUMERIC, 42,172,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"A008","8","",     BTN_NUMERIC,124,172,74,30, TFT_BLUE, TFT_BUTTONCOLOR}, 
    {"A009","9","",     BTN_NUMERIC,202,172,74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Clear","Clear","",BTN_NUMERIC, 42,208,74,30, TFT_BLUE, TFT_BUTTONCOLOR}, 
    {"A000","0","",     BTN_NUMERIC,122,208,74,30, TFT_BLUE, TFT_BUTTONCOLOR},       
    {"Enter","Enter","",BTN_NUMERIC,202,208,74,30, TFT_BLUE, TFT_BUTTONCOLOR},  

    {"Prev","Prev","",     BTN_PREV,  2,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Next","Next","",     BTN_NEXT,242,172, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"ToLeft","<<","",      BTN_NAV,  2,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"ToRight",">>","",     BTN_NAV,162,208,154,30, TFT_BLUE, TFT_BUTTONCOLOR},
    {"Navigate","Freq","",  BTN_NAV,  2,208,314,30, TFT_BLUE, TFT_BUTTONCOLOR},   
    {"Close","Close","",  BTN_CLOSE,122,208, 74,30, TFT_BLUE, TFT_BUTTONCOLOR},     
};

typedef struct {
  byte chkDigit;
  bool isUHF;
  uint16_t aprsChannel;
  uint16_t rxChannel;
  uint16_t txChannel;
  uint8_t repeater;
  int8_t txShift;
  bool autoShift;
  bool draPower;
  byte hasTone;
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
  byte symbool;  // = auto.
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
  bool preEmphase;
  bool highPass;
  bool lowPass;
  bool isDebug;
} Settings;

typedef struct {
    char code[5];
    char tone[8];
} CTCSSCode;

typedef struct // Repeaterlist
{
    const char *name;     // Repeatername
    const char *city;  // Repeatercity
    int8_t shift;
    uint16_t channel;          
    uint16_t ctcssTone; 
    uint16_t hasTone;
} Repeater;

// Settings & Variables
bool isPTT                = false;
bool lastPTT              = false;
bool isMOX                = false;
bool isReverse            = false;
String activeBtn          = "Freq";
String freqType           = "Freq";
String commandButton      = "";
long activeBtnStart       = millis();
long lastBeacon           = millis();
int actualPage            = 1;
int lastPage              = 8;
int beforeDebugPage       = 0;
int debugPage             = 16;
int debugLine             = 0;
long startTime            = millis();
long gpsTime              = millis();
long saveTime             = millis();
long startedDebugScreen   = millis();
long aprsGatewayRefreshed = millis();
long webRefresh           = millis();
long waitForResume        = 0;
bool wifiAvailable        = false;
bool wifiAPMode           = false;
bool isOn                 = true;
bool gotPacket            = false;
bool squelshClosed        = true;
bool isMuted              = false;
bool aprsGatewayConnected = false;
int scanMode              = SCAN_STOPPED;
uint16_t lastCourse       = 0;
char httpBuf[120]         = "\0";
char buf[300]             = "\0";
int32_t keyboardNumber    = 0;

AX25Msg incomingPacket;
uint8_t *packetData;

const int ledFreq          = 5000;
const int ledResol         = 8;
const int ledChannelforTFT = 0;

#include "rdk_config.h";                  // Change to config.h

TFT_eSPI tft = TFT_eSPI();            // Invoke custom library
WiFiMulti wifiMulti;
WiFiClient httpNet;
TinyGPSPlus gps;
AsyncWebServer server(80);
AsyncEventSource events("/events");

/***************************************************************************************
**                          Setup
***************************************************************************************/
void setup(){
  pinMode(PTTOUT,OUTPUT);
  digitalWrite(PTTOUT, 0); // low no PTT
  pinMode(HIPOWERPIN,OUTPUT);
  digitalWrite(HIPOWERPIN, 0); // low power from DRA
  pinMode(TRXONPIN,OUTPUT);
  digitalWrite(TRXONPIN, 0); // low TRX On
  pinMode(DISPLAYLEDPIN, OUTPUT);
  digitalWrite(DISPLAYLEDPIN, 0);
  pinMode(MUTEPIN, OUTPUT);
  digitalWrite(MUTEPIN, 0);

  pinMode(PTTIN,INPUT_PULLUP);
  pinMode(BTNSMIKEPIN,INPUT);
  pinMode(SQUELSHPIN,INPUT);

  ledcSetup(ledChannelforTFT, ledFreq, ledResol);
  ledcAttachPin(DISPLAYLEDPIN, ledChannelforTFT);

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  tft.init();
  tft.setRotation(screenRotation);

  tft.setTouch(calData);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // add Wi-Fi networks from All_Settings.h
  for (int i = 0; i < sizeof(wifiNetworks)/sizeof(wifiNetworks[0]); i++ )
      wifiMulti.addAP(wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);

  DrawButton(80,80,160,30,"Connecting to WiFi","",TFT_RED,TFT_WHITE,"");

  if (!EEPROM.begin(EEPROM_SIZE)){
    DrawButton(80,120,160,30,"EEPROM Failed","",TFT_RED,TFT_WHITE,"");
    Serial.println("failed to initialise EEPROM"); 
    while(1);
  } 

  if (!LoadConfig()){
    Serial.println(F("Writing defaults"));
    SaveConfig();
  }
  LoadConfig();

  // show connected SSID
  wifiMulti.addAP(settings.wifiSSID,settings.wifiPass);
  if (Connect2WiFi()){
    wifiAvailable=true;
    DrawButton(80,80,160,30,"Connected to WiFi",WiFi.SSID(),TFT_RED,TFT_WHITE,"");
    delay(1000);
    udp.begin(localPort);
    syncTime();
  } else {
    wifiAPMode=true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("APRSTRX", NULL);
  } 

  ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
  if (settings.repeater>0 && !settings.isUHF) activeBtn="RPT";
  freqType = activeBtn;
  digitalWrite(HIPOWERPIN, !settings.draPower); // low power from DRA
  SetDraVolume(settings.volume);
  SetDraSettings();
  APRS_init(ADC_REFERENCE, OPEN_SQUELCH);
  APRS_setCallsign(settings.call,settings.ssid);
  APRS_setDestination(settings.dest,settings.destSsid);
  APRS_setPath1(settings.path1,settings.path1Ssid);
  APRS_setPath2(settings.path2,settings.path2Ssid);
  APRS_setPower(settings.power);
  APRS_setHeight(settings.height);
  APRS_setGain(settings.gain);
  APRS_setDirectivity(settings.directivity);
  APRS_setPreamble(settings.preAmble);
  APRS_setTail(settings.tail);
  APRS_setLat(Deg2Nmea(settings.lat,true));
  APRS_setLon(Deg2Nmea(settings.lon,false));
  APRS_printSettings();

  APRSGatewayUpdate();
  aprsGatewayRefreshed = millis();

  if (wifiAvailable || wifiAPMode){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", String(), false, Processor);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/style.css", "text/css");
    });

    server.on("/command", HTTP_GET, [] (AsyncWebServerRequest *request) {
      if (request->hasParam("button")) commandButton = request->getParam("button")->value();
      request->send(SPIFFS, "/index.html", String(), false, Processor);
    });

    server.on("/settings", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/settings.html", String(), false, Processor);
    });

    server.on("/nummers", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/nummers.html", String(), false, Processor);
    });

    server.on("/reboot", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Rebooting");
      ESP.restart();
    });

    server.on("/store", HTTP_GET, [] (AsyncWebServerRequest *request) {
      SaveSettings(request);
      SaveConfig();
      request->send(SPIFFS, "/settings.html", String(), false, Processor);
    });

    events.onConnect([](AsyncEventSourceClient *client){
      Serial.println("Connect web");
      if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);

    server.begin();
    Serial.println("HTTP server started");
  }
  DrawScreen();
}

bool Connect2WiFi(){
  startTime = millis();
  Serial.print("Connect to Multi WiFi");
  while (wifiMulti.run() != WL_CONNECTED && millis()-startTime<30000){
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  return (WiFi.status() == WL_CONNECTED);
}

/***************************************************************************************
**                          Loop
***************************************************************************************/
void loop(){
  delay(10);

  if (commandButton>""){
    if (commandButton=="ToLeft" || commandButton=="ToRight"){
      HandleButton(commandButton,-1,0);
    } else {
      HandleButton(commandButton);
    }
    commandButton = "";
    ClearButtons();
  }

  if (scanMode==SCAN_INPROCES){
    if (freqType=="Freq"){
      settings.rxChannel++;
      if (!settings.isUHF && settings.rxChannel==settings.maxChannel) settings.rxChannel=0;
      if (settings.isUHF && settings.rxChannel==settings.maxUHFChannel) settings.rxChannel=settings.minUHFChannel;
      SetFreq(0, settings.rxChannel, 0, false);
    }    
    if (freqType=="RPT"){
      if (settings.repeater<(sizeof(repeaters)/sizeof(repeaters[0]))-1) settings.repeater++; else settings.repeater=1;
      settings.rxChannel = repeaters[settings.repeater].channel;
      settings.txShift = repeaters[settings.repeater].shift;
      settings.ctcssTone = repeaters[settings.repeater].ctcssTone;
      settings.hasTone = repeaters[settings.repeater].hasTone;
      SetFreq(0, settings.rxChannel, settings.txShift + 10, false);
      DrawButton("RPT");
    }
    DrawFrequency(false);
    DrawButton("Shift");
    DrawButton("Reverse");
    DrawButton("Tone");
    RefreshWebPage();
    delay(100);
  }

  uint16_t x = 0, y = 0;
  bool pressed = tft.getTouch(&x, &y);
  if (pressed) {
    int showVal = ShowControls();
    for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
      if ((buttons[i].pageNo&showVal)>0){
        if (x >= buttons[i].xPos && x <= buttons[i].xPos+buttons[i].width && y >= buttons[i].yPos && y <= buttons[i].yPos+buttons[i].height){
          Serial.print(buttons[i].name);
          Serial.print(" pressed:");
          delay(100);
          HandleButton(buttons[i],x,y);
        }
      }
    }
  }
  WaitForWakeUp();

  bool sql = digitalRead(SQUELSHPIN);
  if (sql !=squelshClosed){
    squelshClosed = sql;
    DrawFrequency(false);
    if (scanMode==SCAN_INPROCES && !squelshClosed){
      digitalWrite(MUTEPIN,isMuted);
      scanMode=(settings.scanType==SCAN_TYPE_STOP)?SCAN_STOPPED:SCAN_PAUSED;
      DrawButton("Scan");
    }
  }

  if (scanMode==SCAN_PAUSED && !squelshClosed) waitForResume=0;
  if (scanMode==SCAN_PAUSED && squelshClosed){
    if (waitForResume==0){
      waitForResume=millis();
    }
    if (millis()-waitForResume>3000){
      digitalWrite(MUTEPIN,1);
      scanMode=SCAN_INPROCES;
      DrawButton("Scan");
      waitForResume=0;
    }
  }      
  
  int btnValue = analogRead(BTNSMIKEPIN);
  if (btnValue<2048){
    int firstBtnValue=btnValue;
    long startPress = millis();
    while (btnValue<2048 && millis()-startPress<2500){
      btnValue = analogRead(BTNSMIKEPIN);
    }
    if (millis()-startPress>2000){
      Button button = FindButtonByName("Scan");
      HandleButton(button,-1,0);
      delay(200);
    } else {
      if (firstBtnValue<2){
        Button button = FindButtonByName("ToRight");
        HandleButton(button,-1,0);
        delay(200);
      }

      if (firstBtnValue>1 && firstBtnValue<2048){
        Button button = FindButtonByName("ToLeft");
        HandleButton(button,-1,0);
        delay(200);
      }
    }
  } 
  


  if (minute()!=lastMinute){
    syncTime();
    if (actualPage<lastPage && wifiAvailable) DrawTime();
    lastMinute = minute();
  }

  while (Serial2.available()) gps.encode(Serial2.read());
  if (millis()-gpsTime>1000){
    gpsTime = millis();
    PrintGPSInfo();
  }

  bool isFromPTT = CheckAndSetPTT(false);

  if (millis()-saveTime>10000 && scanMode==SCAN_STOPPED){
    saveTime = millis();
    if (!CompareConfig()) SaveConfig();
  }

  if (millis()-activeBtnStart>10000){
    if (activeBtn!=freqType){
      activeBtn=freqType;
      DrawButtons();
    }
  }

  if (settings.useAPRS && (gps.location.isValid() || settings.isDebug)){
    bool doBeacon = false;
    long beaconInterval = settings.interval*settings.multiplier*1000;
    int gpsSpeed = gps.speed.isValid()?gps.speed.kmph():0;
    if (gpsSpeed>5) beaconInterval=settings.interval*4*1000;
    if (gpsSpeed>25) beaconInterval=settings.interval*2*1000;
    if (gpsSpeed>80) beaconInterval=settings.interval*1000;

    if (millis()-lastBeacon>beaconInterval) doBeacon=true;
    if (millis()-lastBeacon>20000 && settings.isDebug) doBeacon=true;

    if (gps.course.isValid()){
      uint16_t sbCourse = (abs(gps.course.deg() - lastCourse));
      if (sbCourse > 180) sbCourse = 360 - sbCourse;
      if (sbCourse > 27) doBeacon = true;
    }

    if (isFromPTT && settings.bcnAfterTX) doBeacon=true;

    if (doBeacon) {
      lastBeacon = millis();
      lastCourse = gps.course.isValid()?gps.course.deg():-1;
      SendBeacon(false, (isFromPTT && settings.bcnAfterTX));
    }
  }

  if (millis()-aprsGatewayRefreshed>(settings.aprsGatewayRefreshTime*1000)){
    APRSGatewayUpdate();
    aprsGatewayRefreshed = millis();
  }

  if ((millis() - webRefresh) > 2000) {
    RefreshWebPage();
  }

  if (actualPage==debugPage && millis()-startedDebugScreen>3000){
    actualPage = beforeDebugPage;
    beforeDebugPage = 0;
    DrawScreen();
  }
}

void WaitForWakeUp(){
  delay(10);
  while (!isOn){
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);
    if (pressed) ESP.restart();
  }
}

bool CheckAndSetPTT(bool isAPRS){
  bool retVal = false;
  bool isPTTIN = !digitalRead(PTTIN);
  if (isMOX || isPTTIN) isPTT = 1; else isPTT=0;

  if (isPTT && scanMode!=SCAN_STOPPED){
    digitalWrite(MUTEPIN,isMuted);
    scanMode=SCAN_STOPPED;
    DrawButton("Scan");
    delay(1000);
  } else {
    if (isPTTIN && isMOX) isMOX=false;
    if (lastPTT != isPTT && (!settings.isUHF || isAPRS)){
      digitalWrite(MUTEPIN,isPTT?true:isMuted);
      Serial.println("PTT=" + isPTT?"True":"False");
      lastPTT = isPTT;
      if (!isPTT && !isAPRS) retVal = true;
      digitalWrite(PTTOUT,isPTT);
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
  Serial.println("APRS packet received");
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

      packetData = (uint8_t*)malloc(msg->len);
      memcpy(packetData, msg->info, msg->len);
      incomingPacket.info = packetData;
   }
}

/***************************************************************************************
**            APRS Send beacon
***************************************************************************************/
void SendBeacon(bool manual, bool afterTX){
  if (gps.location.age()<5000 || manual || settings.isDebug){
    ShowDebugScreen("Send beacon");
    if ((!wifiAvailable || !APRSGatewayConnect() || settings.isDebug) && (squelshClosed || afterTX)) SendBeaconViaRadio(); // || settings.isDebug
    if (wifiAvailable && APRSGatewayConnect()) SendBeaconViaWiFi();
  }
}

void SendBeaconViaRadio(){
  if (!isPTT){
    DrawDebugInfo("Send beacon via radio..");
    int lastScanMode = scanMode; 
    scanMode = SCAN_STOPPED;
    if (lastScanMode!=SCAN_STOPPED) delay(500);
    SetFreq(0, 0, 0, true);
    isMOX = 1;
    CheckAndSetPTT(true);
    delay(500);

    if (gps.location.age()>5000){
      APRS_setLat(Deg2Nmea(settings.lat,true));
      APRS_setLon(Deg2Nmea(settings.lon,false));
    } else {
      APRS_setLat(Deg2Nmea(gps.location.lat(),true));
      APRS_setLon(Deg2Nmea(gps.location.lng(),false));
    }

    APRS_sendLoc(settings.comment, strlen(settings.comment));

    isMOX = 0;
    CheckAndSetPTT(false);
    delay(500);
    SetFreq(0, settings.rxChannel, settings.txShift+10, false);
    DrawFrequency(false);
    delay(200);
    scanMode = lastScanMode;
    if (scanMode==SCAN_INPROCES) digitalWrite(MUTEPIN,true);
  }
}

void SendBeaconViaWiFi(){
  if (APRSGatewayConnect()){
    String sLat;
    String sLon;
    if (gps.location.age()>5000){
      sLat = Deg2Nmea(settings.lat,true);
      sLon = Deg2Nmea(settings.lon,false);
    } else {
      sLat = Deg2Nmea(gps.location.lat(),true);
      sLon = Deg2Nmea(gps.location.lng(),false);
    }
    sprintf(buf,"%s-%d>%s:=%s/%s&PHG5000%s",settings.call,settings.ssid,settings.dest,sLat,sLon,settings.comment);
    DrawDebugInfo(buf);
    httpNet.println(buf);
    if (ReadHTTPNet()) DrawDebugInfo(buf);
  }
}

char* Deg2Nmea(float fdeg, boolean is_lat) {
  long deg = fdeg*1000000;
  bool is_negative=0;
  if (deg < 0) is_negative=1;

  // Use the absolute number for calculation and update the buffer at the end
  deg = labs(deg);

  unsigned long b = (deg % 1000000UL) * 60UL;
  unsigned long a = (deg / 1000000UL) * 100UL + b / 1000000UL;
  b = (b % 1000000UL) / 10000UL;

  buf[0] = '0';
  // in case latitude is a 3 digit number (degrees in long format)
  if( a > 9999) { snprintf(buf , 6, "%04u", a);} else snprintf(buf + 1, 5, "%04u", a);

  buf[5] = '.';
  snprintf(buf + 6, 3, "%02u", b);
  buf[9] = '\0';
  if (is_lat) {
    if (is_negative) {buf[8]='S';}
    else buf[8]='N';
    return buf+1;
    // buf +1 because we want to omit the leading zero
  }
  else {
    if (is_negative) {buf[8]='W';}
    else buf[8]='E';
    return buf;
  }
}

/***************************************************************************************
**            Calculate frequency parts before and after decimal sign
***************************************************************************************/
 SFreq GetFreq(int channel){
  SFreq sFreq = GetFreq(channel,80);
  return sFreq;
}

 SFreq GetFreq(int channel, int steps){
  int fMHz = floor(channel/steps) + 144;
  int fKHz = (channel - (floor(channel/steps)*steps))*(10000/steps);
  SFreq sFreq = {fMHz,fKHz};
  return sFreq;
}

/***************************************************************************************
**            Draw screen
***************************************************************************************/
void DrawScreen(){
  tft.fillScreen(TFT_BLACK);
  if (actualPage<lastPage) DrawFrequency(false);
  if (actualPage==BTN_NUMERIC) DrawKeyboardNumber(false);
  DrawButtons();
}

void ShowDebugScreen(char header[]){
  startedDebugScreen=millis();
  debugLine = 0;
  if (actualPage!=debugPage) beforeDebugPage = actualPage;
  actualPage = debugPage;
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(50);
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.drawString(header,2,10,2);
  tft.setTextColor(TFT_GREEN,TFT_BLACK);
}

void DrawKeyboardNumber(bool doReset){
  tft.setTextDatum(MC_DATUM);
  sprintf(buf,"%s",activeBtn);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf,162,15,4);

  tft.setTextDatum(MR_DATUM);
  if (doReset) keyboardNumber = 0;
  int f1 = floor(keyboardNumber/10000);
  int f2 = keyboardNumber-(f1*10000);
  int fBand=settings.isUHF?430:140;
  if (activeBtn=="Freq") sprintf(buf,"%01d.%04d",fBand+f1,f2); else sprintf(buf,"%1d",keyboardNumber);
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, 282,60,7);
}

void DrawDebugInfo(char debugInfo[]){
  if (beforeDebugPage>0) tft.drawString(debugInfo,2,25+(debugLine*8),1);
  Serial.println(debugInfo);
  debugLine++;
}
/***************************************************************************************
**            Draw frequencies
***************************************************************************************/
void DrawFrequency(bool isAPRS){
  if (actualPage<lastPage){
    int freq, fMHz;

    tft.setTextDatum(ML_DATUM);
    SFreq sFreq = GetFreq(settings.aprsChannel);
    sprintf(buf,"APRS:%01d.%03d, %s-%d, %s-%d",sFreq.fMHz,sFreq.fKHz,settings.call,settings.ssid,settings.dest,settings.destSsid);
    tft.setTextPadding(tft.textWidth(buf));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    if (settings.useAPRS) tft.drawString(buf,2,24,1);

    tft.setTextDatum(MR_DATUM);

    if (wifiAvailable || wifiAPMode){
      tft.setTextPadding(tft.textWidth(WiFi.SSID()));
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString(WiFi.SSID(),315,4,1);
    }

    int txChannel = isAPRS?settings.aprsChannel:settings.txChannel;
    int rxChannel = isAPRS?settings.aprsChannel:settings.rxChannel;
    if (isPTT^isReverse) sFreq = GetFreq(txChannel); else sFreq = GetFreq(rxChannel);
    Serial.printf("%01d.%04d",sFreq.fMHz,sFreq.fKHz);
    sprintf(buf,"%01d.%04d",sFreq.fMHz,sFreq.fKHz);
    tft.setTextPadding(tft.textWidth(buf));
    if (isPTT) tft.setTextColor(TFT_RED, TFT_BLACK); else tft.setTextColor(squelshClosed?TFT_GOLD:TFT_GREEN, TFT_BLACK);
    tft.drawString(buf, 315,60,7);

    if (isPTT^isReverse) sFreq = GetFreq(rxChannel); else sFreq = GetFreq(txChannel);
    sprintf(buf,"%01d.%04d",sFreq.fMHz,sFreq.fKHz);
    tft.fillRect(100,86,165,8,TFT_BLACK);
    tft.setTextPadding(tft.textWidth(repeaters[settings.repeater].name));
    tft.drawString(repeaters[settings.repeater].name, 140,90,1);
    tft.setTextPadding(tft.textWidth(repeaters[settings.repeater].city));
    tft.drawString(repeaters[settings.repeater].city, 220,90,1);
    tft.setTextPadding(tft.textWidth(buf));
    tft.drawString(buf, 315,90,1);
    if (isPTT){
      tft.fillCircle(50,60,24,TFT_RED);
      tft.setTextDatum(MC_DATUM);
      tft.setTextPadding(tft.textWidth("TX"));
      tft.setTextColor(TFT_BLACK, TFT_RED);
      tft.drawString("TX", 50,62,4);
    } else  tft.fillCircle(50,60,24,TFT_BLACK);
    DrawMeter(2, 100, 314, 30, isPTT?4:10, isPTT);
  }
}

/***************************************************************************************
**            Draw the time
***************************************************************************************/
void DrawTime(){

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
void DrawMeter(int xPos, int yPos, int width, int height, int value, bool isTX){
  if (!isTX) value = squelshClosed?0:10;
  if (isTX) value = settings.draPower?9:4;
  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("S"));
  tft.drawString(isTX?"P":"S", xPos+20, yPos+(height/2)+1, 4);
  tft.drawLine(xPos+40,yPos+25,xPos+200,yPos+25,TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  for (int i=0;i<9;i++){
      sprintf(buf, "%d", i+1);
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, xPos+40+(i*20), yPos+8, 1);
      tft.drawLine(xPos+40+(i*20),yPos+20,xPos+40+(i*20),yPos+25,TFT_WHITE);
  }

  tft.drawLine(xPos+200,yPos+25,xPos+300,yPos+25,TFT_BLUE);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  for (int i=10;i<50;i=i+10){
      sprintf(buf, "+%d", i);
      tft.setTextPadding(tft.textWidth(buf));
      tft.drawString(buf, xPos+200+(i*2), yPos+8, 1);
      tft.drawLine(xPos+200+(i*2),yPos+20,xPos+200+(i*2),yPos+25,TFT_BLUE);
  }

  tft.setTextPadding(tft.textWidth("dB"));
  tft.drawString("dB", xPos+300, yPos+8, 1);
  tft.drawLine(xPos+300,yPos+18,xPos+300,yPos+24,TFT_BLUE);

  for (int i=4;i<(value*4);i++){
      uint16_t signColor = TFT_WHITE;
      signColor = (i>35)?TFT_RED:TFT_WHITE;
      tft.fillRect(xPos+20+(i*5),yPos+12,4,7,signColor);
  }
}

/***************************************************************************************
**            Draw button
**            Find buttons, button values and colors
**            Draw box
***************************************************************************************/
void DrawButtons(){
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    int showVal = ShowControls();
    if ((buttons[i].pageNo&showVal)>0){
      Button button = FindButtonInfo(buttons[i]);
      button.bckColor = TFT_BUTTONCOLOR;
      if (String(button.name) == activeBtn) button.bckColor = TFT_WHITE;
      DrawButton(button.xPos,button.yPos,button.width,button.height,button.caption,button.waarde,button.btnColor,button.bckColor,button.name);
    }
  }
}

void DrawButton(String btnName){
  DrawButton(btnName,0);
}

void DrawButton(String btnName, uint16_t btnColor){
  int showVal = ShowControls();
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    if (String(buttons[i].name)==btnName && ((buttons[i].pageNo&showVal)>0)){
      Button button = FindButtonInfo(buttons[i]);
      if (btnColor==0) btnColor = button.btnColor; 
      if (String(button.name) == activeBtn) button.bckColor = TFT_WHITE;
      DrawButton(button.xPos,button.yPos,button.width,button.height,button.caption,button.waarde,btnColor,button.bckColor,button.name);
    }
  }
}

void DrawButton(int xPos, int yPos, int width, int height, String caption, String waarde, uint16_t btnColor, uint16_t bckColor, String Name){
  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);

  tft.fillRoundRect(xPos + 2,yPos + 2, width-4, (height/2)+1, 3, bckColor);
  tft.setTextPadding(tft.textWidth(caption));
  tft.setTextColor(TFT_BLACK, bckColor);
  tft.drawString(caption, xPos + (width/2), yPos + (height/2)-5, 2);

  if (Name=="Navigate"){
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(tft.textWidth("     <<     <"));
    tft.setTextColor(TFT_BLACK, bckColor);
    tft.drawString("     <<     <", 5, yPos + (height/2)-5, 2);
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth(">     >>     "));
    tft.setTextColor(TFT_BLACK, bckColor);
    tft.drawString(">     >>     ", 309, yPos + (height/2)-5, 2);
  }

  tft.fillRoundRect(xPos + 2,yPos + 2 + (height/2), width-4, (height/2)-4, 3,btnColor);
  tft.fillRect(xPos + 2,yPos + 2 + (height/2), width-4, 2, TFT_DARKBLUE);

  if (waarde!=""){
    tft.setTextPadding(tft.textWidth(waarde));
    tft.setTextColor(TFT_WHITE, btnColor);
    tft.drawString(waarde, xPos + (width/2), yPos + (height/2)+9, 1);
  }
}

int ShowControls(){
  int retVal = actualPage;
  if (actualPage == 1) retVal = retVal + BTN_NAV + BTN_NEXT;
  if (actualPage >1 && actualPage <lastPage) retVal = retVal + BTN_ARROW + BTN_NEXT + BTN_PREV;
  if (actualPage == lastPage) retVal = retVal + BTN_CLOSE;
  return retVal;
}

Button FindButtonByName(String Name){
  for (int i=0; i<sizeof(buttons)/sizeof(buttons[0]); i++) {
    if (String(buttons[i].name)==Name) return FindButtonInfo(buttons[i]);
  }
  return buttons[0];
}

Button FindButtonInfo(Button button){
  char buttonBuf[10] = "\0";
  if (button.name=="Shift"){
    if (settings.txShift==SHIFT_NONE){
      button.btnColor = TFT_BLUE;
      strcpy(button.waarde,"None");
    } 
    if (settings.txShift==SHIFT_NEG){
      button.btnColor= TFT_RED;
      strcpy(button.waarde,"-600");
    }
    if (settings.txShift==SHIFT_POS){
      button.btnColor= TFT_GREEN;
      strcpy(button.waarde,"600");
    } 
  }

  if (button.name=="MOX"){
    if (!isMOX){
      button.btnColor = TFT_BLUE;
      strcpy(button.waarde,"");
    } else {
      button.btnColor= TFT_RED;
      strcpy(button.waarde,"PTT");
    }
  }

  if (button.name=="Reverse"){
    if (!isReverse){
      button.btnColor = TFT_BLUE;
      strcpy(button.waarde,"");
    } else {
      button.btnColor= TFT_RED;
      strcpy(button.waarde,"On");
    }
  }

  if (button.name=="Vol") {
    sprintf(buttonBuf,"%d",settings.volume);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="SQL") {
    sprintf(buttonBuf,"%d",settings.squelsh);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="Light") {
    sprintf(buttonBuf,"%d",settings.currentBrightness);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="Tone") {
    String s;
    if (settings.hasTone==TONETYPERX) s="R ";
    if (settings.hasTone==TONETYPETX) s="T ";
    if (settings.hasTone==TONETYPERXTX) s="RT ";
    sprintf(buttonBuf,"%s %s", s, settings.hasTone>TONETYPENONE?cTCSSCodes[settings.ctcssTone].tone:"");
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="RPT") {
    sprintf(buttonBuf,"%s", repeaters[settings.repeater].name);
    strcpy(button.waarde,buttonBuf);
  }

  if (button.name=="Navigate") {
    if (activeBtn=="Freq") sprintf(buttonBuf,"%s", "Freq");
    else if (activeBtn=="Vol") sprintf(buttonBuf,"%s", "Vol");
    else if (activeBtn=="SQL") sprintf(buttonBuf,"%s", "SQL");
    else if (activeBtn=="Tone") sprintf(buttonBuf,"%s", "Tone");
    else if (activeBtn=="RPT") sprintf(buttonBuf,"%s", "RPT");
    else if (activeBtn=="Light") sprintf(buttonBuf,"%s", "Light");
    else if (activeBtn=="Scan") sprintf(buttonBuf,"%s", "Scan");
    else sprintf(buttonBuf,"%s", "< >");
    button.caption = buttonBuf;
  }

  if (button.name=="APRS"){
    if (!settings.useAPRS){
      button.btnColor = TFT_BLUE;
      strcpy(button.waarde,"Off");
    } else {
      button.btnColor= TFT_RED;
      strcpy(button.waarde,"On");
    }
  }

  if (button.name=="Power"){
    strcpy(button.waarde,settings.draPower?"High":"Low");
  }

  if (button.name=="Mute"){
    strcpy(button.waarde,isMuted?"True":"False");
  }

  if (button.name=="Scan"){
    if (settings.scanType==SCAN_TYPE_STOP) sprintf(buttonBuf,"%s", "Stop");
    if (settings.scanType==SCAN_TYPE_RESUME) sprintf(buttonBuf,"%s", "Resume");
    strcpy(button.waarde,buttonBuf);
    if (scanMode==SCAN_STOPPED){
      button.btnColor = TFT_BLUE;
    } else if (scanMode==SCAN_INPROCES) {
      button.btnColor= TFT_RED;
    } else {
      button.btnColor= TFT_GREEN;
    }
  }

  if (button.name=="SetBand"){
    if (!settings.isUHF){
      button.btnColor = TFT_BLUE;
      strcpy(button.waarde,"VHF");
    } else {
      button.btnColor = TFT_DARKBLUE;
      strcpy(button.waarde,"UHF");
    }
  }

  return button;
}

void DrawBox(int xPos, int yPos, int width, int height){
  tft.drawRoundRect(xPos+2,yPos+2,width,height, 4, TFT_SHADOW);
  tft.drawRoundRect(xPos,yPos,width,height, 3, TFT_WHITE);
  tft.fillRoundRect(xPos + 1,yPos + 1, width-2, height-2, 3, TFT_BLACK);
}

/***************************************************************************************
**            Handle button
***************************************************************************************/
void HandleButton(String buttonName){
  Button button = FindButtonByName(buttonName);
  HandleButton(button,0,0);
}

void HandleButton(String buttonName, int x, int y){
  Button button = FindButtonByName(buttonName);
  HandleButton(button,x,y);
}

void HandleButton(Button button, int x, int y){
  if (button.name=="ToRight"){
    activeBtnStart = millis();
    if (activeBtn=="Freq"){
      float f=(x-button.xPos);
      f=f/button.width;
      int val=pow(2,floor(f*4));
      if (x==-1) val = 1;
      if (val==8 && settings.isUHF) val = 40;
      SetFreq(val, 0, 0, false);
      DrawFrequency(false);
      DrawButton("Shift");
      DrawButton("Reverse");
      delay(200);
    }
    if (activeBtn=="Vol"){
      if (settings.volume<8) settings.volume++;
      SetDraVolume(settings.volume);
      DrawButton("Vol");
      DrawButton("Mute");
    }
    if (activeBtn=="SQL"){
      if (settings.squelsh<8) settings.squelsh++;
      SetFreq(0, 0, 0, false);
      DrawButton("SQL");
    }
    if (activeBtn=="Tone"){
      if (settings.ctcssTone<(sizeof(cTCSSCodes)/sizeof(cTCSSCodes[0]))-1) settings.ctcssTone++;
      SetFreq(0, 0, 0, false);
      DrawButton("Tone");
    }
    if (activeBtn=="Scan"){
      if (settings.scanType<1) settings.scanType++;
      DrawButton("Scan");
    }  
    if (activeBtn=="RPT"){
      if (settings.repeater<(sizeof(repeaters)/sizeof(repeaters[0]))-1) settings.repeater++;
      settings.rxChannel = repeaters[settings.repeater].channel;
      settings.txShift = repeaters[settings.repeater].shift;
      settings.ctcssTone = repeaters[settings.repeater].ctcssTone;
      settings.hasTone = repeaters[settings.repeater].hasTone;
      SetFreq(0, settings.rxChannel, settings.txShift + 10, false);
      DrawFrequency(false);
      DrawButton("RPT");
      DrawButton("Shift");
      DrawButton("Tone");
    }
    if (activeBtn=="Light"){
      if (settings.currentBrightness<100) settings.currentBrightness+=5;
      if (settings.currentBrightness>100) settings.currentBrightness=100;
      DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
    }
    DrawButton("Navigate");
  }

  if (button.name=="ToLeft"){
    activeBtnStart = millis();
    if (activeBtn=="Freq"){
      float f=button.width-(x-button.xPos);
      f=f/button.width;
      int val=pow(2,floor(f*4));
      if (x==-1) val = 1;
      if (val==8 && settings.isUHF) val = 40;
      SetFreq(val*-1, 0, 0, false);
      DrawFrequency(false);
      DrawButton("Shift");
      DrawButton("Reverse");
      delay(200);
    }
    if (activeBtn=="Vol"){
      if (settings.volume>0) settings.volume--;
      SetDraVolume(settings.volume);
      DrawButton("Vol");
      DrawButton("Mute");
    }
    if (activeBtn=="SQL"){
      if (settings.squelsh>0) settings.squelsh--;
      SetFreq(0, 0, 0, false);
      DrawButton("SQL");
    }
    if (activeBtn=="Tone"){
      if (settings.ctcssTone>1) settings.ctcssTone--;
      SetFreq(0, 0, 0, false);
      DrawButton("Tone");
    }
    if (activeBtn=="Scan"){
      if (settings.scanType>0) settings.scanType--;
      DrawButton("Scan");
    }  
    if (activeBtn=="RPT"){
      if (settings.repeater>1) settings.repeater--;
      settings.rxChannel = repeaters[settings.repeater].channel;
      settings.txShift = repeaters[settings.repeater].shift;
      settings.ctcssTone = repeaters[settings.repeater].ctcssTone;
      settings.hasTone = repeaters[settings.repeater].hasTone;
      SetFreq(0, settings.rxChannel, settings.txShift + 10, false);
      DrawFrequency(false);
      DrawButton("RPT");
      DrawButton("Shift");
      DrawButton("Tone");
    }
    if (activeBtn=="Light"){
      if (settings.currentBrightness>5) settings.currentBrightness-=5;
      if (settings.currentBrightness<5) settings.currentBrightness=5;
      DrawButton("Light");
      ledcWrite(ledChannelforTFT, 256-(settings.currentBrightness*2.56));
    }
    DrawButton("Navigate");
  }

  if (button.name=="Shift" && !settings.isUHF){
    settings.txShift++;
    if (settings.txShift>SHIFT_POS) settings.txShift=SHIFT_NEG;
    SetFreq(0, 0, settings.txShift+10, false);
    DrawFrequency(false);
    DrawButton("Shift");
    DrawButton("Reverse");
    delay(200);
  }

  if (button.name=="MOX" && !settings.isUHF){
    isMOX=!isMOX;
    DrawFrequency(false);
    DrawButton("MOX");
    delay(200);
  }

  if (button.name=="Beacon"){
    SendBeacon(true, false);
  }

  if (button.name=="TXBeacon"){
    SendBeaconViaRadio();
    delay(200);
  }

  if (button.name=="Vol"){
    if (activeBtn=="Vol"){
      keyboardNumber = settings.volume;
      actualPage = BTN_NUMERIC;
      DrawScreen();
    } else {
      activeBtn="Vol";
      activeBtnStart = millis();
      DrawButtons();
    }
  }

  if (button.name=="SQL"){
    if (activeBtn=="SQL"){
      keyboardNumber = settings.squelsh;
      actualPage = BTN_NUMERIC;
      DrawScreen();
    } else {
      activeBtn="SQL";
      activeBtnStart = millis();
      DrawButtons();
    }
  }

  if (button.name=="Freq"){
    if (activeBtn=="Freq"){
      SFreq sFreq = GetFreq(settings.rxChannel);
      if (!settings.isUHF) keyboardNumber = ((sFreq.fMHz-140)*10000)+sFreq.fKHz;
      if (settings.isUHF) keyboardNumber = ((sFreq.fMHz-430)*10000)+sFreq.fKHz;
      actualPage = BTN_NUMERIC;
      DrawScreen();
    } else {
      settings.repeater=0;
      settings.txShift = 0;  
      settings.ctcssTone = 0;
      settings.hasTone=TONETYPENONE;
      activeBtn="Freq";
      freqType="Freq";
      DrawFrequency(false);
      DrawButtons();
    }
  }

  if (button.name=="Tone" && !settings.isUHF){
    if (activeBtn=="Tone") settings.hasTone++;
    if (settings.hasTone>3) settings.hasTone=0;
    SetFreq(0, 0, 0, false);
    activeBtn="Tone";
    activeBtnStart = millis();
    DrawButtons();
  }

  if (button.name=="RPT" && !settings.isUHF){
    if (settings.repeater==0) settings.repeater=1;
    settings.rxChannel = repeaters[settings.repeater].channel;
    settings.txShift = repeaters[settings.repeater].shift;
    settings.ctcssTone = repeaters[settings.repeater].ctcssTone;
    settings.hasTone = repeaters[settings.repeater].hasTone;
    SetFreq(0, settings.rxChannel, settings.txShift+10, false);
    DrawFrequency(false);
    activeBtn="RPT";
    freqType="RPT";
    DrawButtons();
  }

  if (button.name=="Calibrate"){
    TouchCalibrate();
    actualPage=1;
    DrawScreen();
  }

  if (button.name=="Reverse"){
    if (settings.rxChannel!=settings.txChannel){
      isReverse=!isReverse;
      SetFreq(0, 0, 0, false);
      DrawFrequency(false);
      DrawButton("Shift");
      DrawButton("Reverse");
      delay(200);  
    } else isReverse=false;
  }

  if (button.name=="Light"){
    activeBtn="Light";
    activeBtnStart = millis();
    DrawButtons();
  }

  if (button.name=="Next") {
    actualPage = actualPage<<1;
    if (actualPage>lastPage) actualPage=1;
    DrawScreen();
  }

  if (button.name=="Prev") {
    if (actualPage>1) actualPage = actualPage>>1;
    DrawScreen();
  }

  if (button.name=="Close") {
    actualPage=1;
    DrawScreen();
  }

  if (button.name=="APRS") {
    settings.useAPRS = !settings.useAPRS;
    DrawScreen();
    delay(200);
  }

  if (button.name=="Power") {
    settings.draPower = !settings.draPower;
    digitalWrite(HIPOWERPIN,!settings.draPower);
    DrawButton("Power");
  }

  if (button.name=="Mute") {
    if (settings.volume>0){
      isMuted = !isMuted;
      digitalWrite(MUTEPIN,isMuted);
      DrawButton("Mute");
    }
  }

  if (button.name=="Off") {
    isOn = false;
    actualPage=1;
    DoTurnOff();
    delay(500);
  }

  if (button.name=="Scan"){
    if (scanMode!=SCAN_STOPPED){
      digitalWrite(MUTEPIN,isMuted);
      scanMode=SCAN_STOPPED;
    } else {
      if (freqType=="Freq") settings.hasTone=TONETYPENONE;
      digitalWrite(MUTEPIN,true);
      scanMode=SCAN_INPROCES;
    }
    activeBtn="Scan";
    activeBtnStart = millis();
    DrawButtons();
  }

  if (button.name=="SetBand"){
    settings.isUHF = !settings.isUHF;
    DrawButton("SetBand");
    SetFreq(0,0,0,false);
    freqType="Freq";  
    settings.repeater = 0;
    settings.txShift = 0;  
    settings.ctcssTone = 0;
    settings.hasTone = TONETYPENONE;
    DrawScreen();
    delay(200);
  }

  if (String(button.name).substring(0,3) =="A00") {
    int i = String(button.name).substring(3).toInt();
    if (activeBtn=="Freq"){
      keyboardNumber = (keyboardNumber*10)+i;
      if (!settings.isUHF && keyboardNumber>=60000) keyboardNumber=0;
      if (settings.isUHF && keyboardNumber>=100000) keyboardNumber=0;
    } 
    if (activeBtn=="Vol" || activeBtn=="SQL"){
      keyboardNumber = i;
      if (keyboardNumber>8) keyboardNumber=0;
    } 
    // else if (activeBtn=="Tone") sprintf(buf,"%s", "Tone");
    // else if (activeBtn=="Light") sprintf(buf,"%s", "Light");
    DrawKeyboardNumber(false);
  }

  if (button.name=="Enter") {
    if (activeBtn=="Vol"){
      settings.volume=keyboardNumber;
      SetDraVolume(settings.volume);
    }
    if (activeBtn=="SQL"){
      settings.squelsh=keyboardNumber;
      SetFreq(0, 0, 0, false);
    } 
    if (activeBtn=="Freq"){
      if (!settings.isUHF) settings.rxChannel = (keyboardNumber-40000)/125;
      if (settings.isUHF) settings.rxChannel = (keyboardNumber/125)+22880;

      SetFreq(0, settings.rxChannel, 0, false);
    }
    actualPage = 1;
    DrawScreen();
  }

  if (button.name=="Clear") {
    actualPage = 1;
    DrawScreen();
  }
  
}
/***************************************************************************************
**            Turn off
***************************************************************************************/
void DoTurnOff(){
  ledcWrite(ledChannelforTFT, 255);
  tft.fillScreen(TFT_BLACK);
  SetDraVolume(0);
  digitalWrite(TRXONPIN, 1); // high TRX Off
}

/***************************************************************************************
**            Calibrate touch
***************************************************************************************/
void TouchCalibrate(){
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println(); Serial.println();
  Serial.println("// Use this calibration code in setup():");
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4) Serial.print(", ");
  }

  Serial.println(" };");
  Serial.print("  tft.setTouch(calData);");
  Serial.println(); Serial.println();

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  tft.println("Calibration code sent to Serial port.");

  delay(4000);
}

/***************************************************************************************
**            Set DRA
***************************************************************************************/
void SetFreq(int step, int channel, uint8_t txShift, bool isAPRS){
  if (isAPRS){
    SetDra(settings.aprsChannel,settings.aprsChannel,0,0,settings.squelsh);
  } else {
    if (settings.isUHF && settings.rxChannel<settings.minUHFChannel) {
      settings.rxChannel=settings.minUHFChannel;
      settings.txChannel=settings.rxChannel;
      settings.repeater=0;
      settings.ctcssTone=0;
      settings.hasTone=TONETYPENONE;
    }
    if (!settings.isUHF && settings.rxChannel>settings.maxChannel){
      settings.rxChannel=0;
      settings.txChannel=settings.rxChannel;
      settings.repeater=0;
    } 

    if (step!=0 || channel!=0  || txShift!=0){
      if (step!=0) settings.rxChannel+=step;
      if (channel!=0) settings.rxChannel=channel;
      if (!settings.isUHF){
        if (settings.rxChannel>settings.maxChannel) settings.rxChannel=settings.maxChannel;
        if (settings.rxChannel<0) settings.rxChannel=0;
      } else {
        if (settings.rxChannel>settings.maxUHFChannel) settings.rxChannel=settings.maxUHFChannel;
        if (settings.rxChannel<settings.minUHFChannel) settings.rxChannel=settings.minUHFChannel;  
      }

      if (settings.autoShift) settings.txShift = (settings.rxChannel>125 && settings.rxChannel<145)?SHIFT_NEG:SHIFT_NONE;

      if (txShift>8) settings.txShift = txShift - 10;
      settings.txChannel = settings.rxChannel + (settings.txShift)*48;

      if (!settings.isUHF){
        if (settings.txChannel>settings.maxChannel || settings.txChannel<0){
          settings.txShift++;
          if (!txShift) settings.txShift=SHIFT_NONE;
          if (settings.txShift>SHIFT_POS) settings.txShift=SHIFT_NEG;
          settings.txChannel = settings.rxChannel + (settings.txShift)*48;
        }
      } else {
        settings.txChannel = settings.rxChannel;
      }

      if (settings.rxChannel==settings.txChannel) isReverse=false;
    }
    SetDra(settings.rxChannel,settings.txChannel,(settings.hasTone==TONETYPERX || settings.hasTone==TONETYPERXTX)?settings.ctcssTone:TONETYPENONE,(settings.hasTone==TONETYPETX || settings.hasTone==TONETYPERXTX)?settings.ctcssTone:TONETYPENONE,settings.squelsh);
  }
  delay(50);
}

void SetDra(uint16_t rxFreq, uint16_t txFreq, byte rxTone, byte txTone, byte squelsh) {
  SFreq rxSFreq = GetFreq(isReverse?txFreq:rxFreq);
  SFreq txSFreq = GetFreq(isReverse?rxFreq:txFreq);

  sprintf(buf,"AT+DMOSETGROUP=0,%01d.%04d,%01d.%04d,%04d,%01d,%04d",txSFreq.fMHz,txSFreq.fKHz,rxSFreq.fMHz,rxSFreq.fKHz,txTone,squelsh,rxTone);
  Serial.println();
  Serial.println(buf);
  Serial2.println(buf);
}

void SetDraVolume(byte volume) {
  isMuted=volume>0?false:true;
  digitalWrite(MUTEPIN,isMuted);
  sprintf(buf,"AT+DMOSETVOLUME=%01d",volume);
  Serial.println();
  Serial.println(buf);
  Serial2.println(buf);
}

void SetDraSettings() {
  sprintf(buf,"AT+SETFILTER=%01d,%01d,%01d",settings.preEmphase,settings.highPass,settings.lowPass);
  Serial.println();
  Serial.print("Filter:");
  Serial.println(buf);
  Serial2.println(buf);

  sprintf(buf,"AT+DMOSETMIC=%01d,0",settings.mikeLevel);
  Serial.println();
  Serial.print("Mikelevel:");
  Serial.println(buf);
  Serial2.println(buf);
}
/***************************************************************************************
**            Print PrintGPSInfo
***************************************************************************************/
void PrintGPSInfo(){
  if (actualPage<lastPage){
    char sz[80];
    tft.setTextDatum(ML_DATUM);
    sprintf(sz,"GPS :LAT:%s, LON:%s, Speed:%s KM, Age:%s     ",String(gps.location.lat(),4),String(gps.location.lng(),4), String(gps.speed.isValid()?gps.speed.kmph():0,0), gps.location.age()>5000?"Inv.":String(gps.location.age()));
    tft.setTextPadding(tft.textWidth(sz));
    if (gps.location.age()>5000) tft.setTextColor(TFT_RED, TFT_BLACK); else tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString(sz,2,14,1);
  }

  if (actualPage==lastPage){
    char sz[50];
    tft.fillRect(2,2,320,200,TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(50);
    tft.setTextColor(TFT_YELLOW,TFT_BLACK);

    tft.drawString("GPS:",2,10,2);
    tft.setTextColor(TFT_GREEN,TFT_BLACK);
    tft.drawString("Valid       :" + gps.location.isValid()?"true":"false",2,25,1);
    tft.drawString("Lat         :" + String(gps.location.lat(),6),2,33,1);
    tft.drawString("Lon         :" + String(gps.location.lng(),6),2,41,1);
    tft.drawString("Age         :" + String(gps.location.age()),2,49,1);
    sprintf(sz,    "Date & Time :%02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
    tft.drawString(sz,2,57,1);
    tft.drawString("Height      :" + String(gps.altitude.meters()),2,65,1);
    tft.drawString("Course      :" + String(gps.course.deg()),2,73,1);
    tft.drawString("Speed       :" + String(gps.speed.isValid()?gps.speed.kmph():0),2,81,1);
    sprintf(sz,    "Course valid:%s", gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "***");
    tft.drawString(sz,2,89,1);

    if (wifiAvailable || wifiAPMode){
      tft.setTextColor(TFT_YELLOW,TFT_BLACK);
      tft.drawString("WiFi:",2,105,2);
      tft.setTextColor(TFT_GREEN,TFT_BLACK);
      tft.drawString("SSID        :" + String(WiFi.SSID()),2,120,1);
      sprintf(sz,"IP Address  :%s", WiFi.localIP());

      sprintf(sz,"IP Address  :%d.%d.%d.%d", WiFi.localIP()[0],WiFi.localIP()[1],WiFi.localIP()[2],WiFi.localIP()[3]);
      tft.drawString(sz,2,128,1);
      tft.drawString("RSSI        :" + String(WiFi.RSSI()),2,136,1); 
    }
  }
}

/***************************************************************************************
**            EEPROM Routines
***************************************************************************************/
bool SaveConfig() {
  for (unsigned int t = 0; t < sizeof(settings); t++)
    EEPROM.write(offsetEEPROM + t, *((char*)&settings + t));
  EEPROM.commit();
  Serial.println("Configuration:saved");
  return true;
}

bool LoadConfig() {
  bool retVal = true;
  if (EEPROM.read(offsetEEPROM + 0) == settings.chkDigit){
    for (unsigned int t = 0; t < sizeof(settings); t++)
      *((char*)&settings + t) = EEPROM.read(offsetEEPROM + t);
  } else retVal = false;
  Serial.println("Configuration:" + retVal?"Loaded":"Not loaded");
  return retVal;
}

bool CompareConfig() {
  bool retVal = true;
  for (unsigned int t = 0; t < sizeof(settings); t++)
    if (*((char*)&settings + t) != EEPROM.read(offsetEEPROM + t)) retVal = false;
  return retVal;
}

/***************************************************************************************
**            APRS/IP Routines
***************************************************************************************/
bool APRSGatewayConnect(){
  char c[20];
  sprintf(c,"%s", WiFi.status() == WL_CONNECTED?"WiFi Connected":"WiFi NOT Connected");
  DrawDebugInfo(c);
  if (wifiAvailable && (WiFi.status() != WL_CONNECTED)){
    aprsGatewayConnected = false;
    if (!Connect2WiFi()) wifiAvailable = false;
  } else Serial.println("WiFi available and WiFiStatus connected");
  if (wifiAvailable) {
    if (!aprsGatewayConnected){
      DrawDebugInfo("Connecting to APRS server...");
      if (httpNet.connect(settings.aprsIP,settings.aprsPort)) {
        if (ReadHTTPNet()) DrawDebugInfo(httpBuf);
        sprintf(buf,"user %s-%d pass %s vers ",settings.call,settings.serverSsid,settings.aprsPassword,VERSION);
        DrawDebugInfo(buf);
        httpNet.println(buf);
        if (ReadHTTPNet()) {
          if (strstr(httpBuf," verified")){
            DrawDebugInfo(httpBuf);
            DrawDebugInfo("Connected to APRS.FI");
            aprsGatewayConnected=true;
          } else DrawDebugInfo("Not connected to APRS.FI");
        } else DrawDebugInfo("No response from ReadHTTPNet");
      } else DrawDebugInfo("Failed to connect to APRS.FI, server unavailable");
    } else DrawDebugInfo("Already connected to APRS.FI");
  } else DrawDebugInfo("Failed to connect to APRS.FI, WiFi not available");
  return aprsGatewayConnected;
}

void APRSGatewayUpdate(){
  DrawDebugInfo("aprsGatewayUpdate:");
  sprintf(buf,"Date & Time :%02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
  DrawDebugInfo(buf);
  if (APRSGatewayConnect()){
    DrawDebugInfo("Update IGate info on APRS");
    sprintf(buf,"%s-%d",settings.call,settings.serverSsid);
    DrawDebugInfo(buf);
    httpNet.print(buf);

    sprintf(buf,">APRS,TCPIP*:@%02d%02d%02dz",gps.date.day(),gps.time.hour(),gps.time.minute());
    DrawDebugInfo(buf);
    httpNet.print(buf);
    String sLat = Deg2Nmea(settings.lat,true);
    String sLon = Deg2Nmea(settings.lon,false);         
    sprintf(buf,"%s/%s",sLat,sLon);
    DrawDebugInfo(buf);
    httpNet.print(buf);

    sprintf(buf,"I/A=000012 %s",INFO);
    DrawDebugInfo(buf);
    httpNet.println(buf);
    if (!ReadHTTPNet()) aprsGatewayConnected = false;;
  } else DrawDebugInfo("APRS Gateway not connected");
}

bool ReadHTTPNet(){
  httpBuf[0] = '\0';
  bool retVal = false;
  long timeOut = millis();
  while (!httpNet.available() && millis()-timeOut<2500){}

  if (httpNet.available()) {
    int i = 0;
    while (httpNet.available() && i<118) {
      char c = httpNet.read();
      httpBuf[i++] = c;
    }
    retVal=true;
    httpBuf[i++] = '\0';
    // Serial.print("Read from HTTP:");
    // Serial.print(httpBuf);
  } else {
    Serial.print("No data returned from HTTP");
  }
  Serial.println();
  return retVal;
}

/***************************************************************************************
**            API Functions
***************************************************************************************/
void NotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

/***************************************************************************************
**            HTML Server functions
***************************************************************************************/
void FillAPRSInfo(){
  buf[0] = '\0';
  SFreq sFreq = GetFreq(settings.aprsChannel);
  sprintf(buf,"APRS:%01d.%03d, %s-%d, %s-%d",sFreq.fMHz,sFreq.fKHz,settings.call,settings.ssid,settings.dest,settings.destSsid);
}

void FillGPSInfo(){
  buf[0] = '\0';
  sprintf(buf,"GPS :LAT:%s, LON:%s, Speed:%s KM, Age:%s     ",String(gps.location.lat(),4),String(gps.location.lng(),4), String(gps.speed.isValid()?gps.speed.kmph():0,0), gps.location.age()>5000?"Inv.":String(gps.location.age()));
}

void FillRXFREQ(){
    buf[0] = '\0';
    SFreq sFreq;
    if (isPTT^isReverse) sFreq = GetFreq(settings.txChannel); else sFreq = GetFreq(settings.rxChannel);
    sprintf(buf,"<h1 style=\"text-align:center;color:%s\">%S%01d.%04d</h1>",isPTT?"red":squelshClosed?"yellow":"green",isPTT?"TX ":squelshClosed?"   ":"RX ",sFreq.fMHz,sFreq.fKHz);
}

void FillTXFREQ(){
    buf[0] = '\0';
    if (settings.rxChannel!=settings.txChannel){
      SFreq sFreq;
      if (isPTT^isReverse) sFreq = GetFreq(settings.rxChannel); else sFreq = GetFreq(settings.txChannel);
      sprintf(buf,"%01d.%04d",sFreq.fMHz,sFreq.fKHz);
    }
}

void FillREPEATERInfo(){
  buf[0] = '\0';
  sprintf(buf,"%s %s",repeaters[settings.repeater].name,repeaters[settings.repeater].city);
}

void RefreshWebPage(){
  if (wifiAvailable || wifiAPMode){
    events.send("ping",NULL,millis());
    FillAPRSInfo();
    events.send(buf,"APRSINFO",millis());
    FillGPSInfo();
    events.send(buf,"GPSINFO",millis());
    FillREPEATERInfo();
    events.send(buf,"REPEATERINFO",millis());
    FillRXFREQ();
    events.send(buf,"RXFREQ",millis());    
    FillTXFREQ();
    events.send(String(buf).c_str(),"TXFREQ",millis()); 
  }
  webRefresh = millis();
}

void ClearButtons(){
  buf[0] = '\0';
  sprintf(buf,"<div class=\"content\"><div class=\"cards\">%BUTTONS0%</div></div>");
  events.send(String(buf).c_str(),"BUTTONSERIE",millis()); 
}

String Processor(const String& var){
  //Serial.print("Process: "); Serial.println(var);
  if(var == "APRSINFO"){
    FillAPRSInfo();
    return buf;
  }
  if(var == "GPSINFO"){
    FillGPSInfo();
    return buf;
  }
  if(var == "RXFREQ"){
    FillRXFREQ();
    return buf;
  }
  if(var == "TXFREQ"){
    FillTXFREQ();
    return buf;
  }
  if(var == "REPEATERINFO"){
    FillREPEATERInfo();
    return buf;
  }

  if (var=="wifiSSID") return settings.wifiSSID;
  if (var=="wifiPass") return settings.wifiPass;
  if (var=="aprsChannel") return String(settings.aprsChannel);
  if (var=="aprsFreq"){
    SFreq sFreq = GetFreq(settings.aprsChannel);
    sprintf(buf,"%01d.%04d",sFreq.fMHz,sFreq.fKHz);
    return buf;
  }
  if (var=="aprsIP") return String(settings.aprsIP);
  if (var=="aprsPort") return String(settings.aprsPort);  
  if (var=="aprsPassword") return String(settings.aprsPassword);
  if (var=="serverSsid") return String(settings.serverSsid);  
  if (var=="aprsGatewayRefreshTime") return String(settings.aprsGatewayRefreshTime); 
  if (var=="call") return String(settings.call); 
  if (var=="ssid") return String(settings.ssid);  
  if (var=="symbool"){
    char x = settings.symbool;
    return String(x);
  }   
  if (var=="dest") return String(settings.dest);   
  if (var=="destSsid") return String(settings.destSsid);  
  if (var=="path1") return String(settings.path1);   
  if (var=="path1Ssid") return String(settings.path1Ssid); 
  if (var=="path2") return String(settings.path2);   
  if (var=="path2Ssid") return String(settings.path2Ssid);     
  if (var=="comment") return settings.comment;
  if (var=="interval") return String(settings.interval);
  if (var=="multiplier") return String(settings.multiplier);
  if (var=="power") return String(settings.power);
  if (var=="height") return String(settings.height);
  if (var=="gain") return String(settings.gain);
  if (var=="directivity") return String(settings.directivity);            
  if (var=="autoShift") return settings.autoShift?"checked":"";
  if (var=="bcnAfterTX") return settings.bcnAfterTX?"checked":"";
  if (var=="txTimeOut") return String(settings.txTimeOut);
  if (var=="lat") return String(settings.lat,6); 
  if (var=="lon") return String(settings.lon,6);   
  if (var=="maxChannel") return String(settings.maxChannel);
  if (var=="maxFreq"){
    SFreq sFreq = GetFreq(settings.maxChannel);
    sprintf(buf,"%01d.%04d",sFreq.fMHz,sFreq.fKHz);
    return buf;
  }
  if (var=="isDebug") return settings.isDebug?"checked":"";

  if(var >= "BUTTONS" && var <= "BUTTONS99"){
    buf[0] = '\0';
    int i = var.substring(7).toInt();
    if (i+1<(sizeof(buttons)/sizeof(buttons[0]))) {
      Button button = FindButtonInfo(buttons[i]);
      if ((button.pageNo<lastPage || button.pageNo==BTN_ARROW) && button.name!="MOX"){ 
        sprintf(buf, "<div id=\"BTN%s\" class=\"card\" style=\"background-color:%s\"><p><a href=\"/command?button=%s\">%s</a></p><p style=\"background-color:%s;color:white\"><span class=\"reading\"><span id=\"%s\">%s</span></span></p></div>",button.name,String(button.name) == activeBtn?"white":"lightblue",button.name,button.caption, button.btnColor==TFT_RED?"red":button.btnColor==TFT_GREEN?"green":"blue",button.name,button.waarde);
      }
      char buf2[10];
      sprintf(buf2,"%%BUTTONS%d%%",i+1);
      strcat(buf,buf2);
    }
    return buf;
  }

  if(var >= "NUMMERS0" && var <= "NUMMERS99"){
    buf[0] = '\0';
    int i = var.substring(7).toInt();
    if (i+1<(sizeof(buttons)/sizeof(buttons[0]))) {
      Button button = FindButtonInfo(buttons[i]);
      if (button.pageNo==BTN_NUMERIC){ 
        sprintf(buf, "<div id=\"BTN%s\" class=\"card\" style=\"background-color:%s\"><p><a href=\"/command?button=%s\">%s</a></p><p style=\"background-color:%s;color:white\"><span class=\"reading\"><span id=\"%s\">%s</span></span></p></div>",button.name,"lightgreen",button.name,button.caption, "blue",button.name,button.waarde);
      }
      char buf2[10];
      sprintf(buf2,"%%NUMMERS%d%%",i+1);
      strcat(buf,buf2);
    }
    return buf;
  }

  return var;
}

void SaveSettings(AsyncWebServerRequest *request){
  if (request->hasParam("wifiSSID")) request->getParam("wifiSSID")->value().toCharArray(settings.wifiSSID,25);
  if (request->hasParam("wifiPass")) request->getParam("wifiPass")->value().toCharArray(settings.wifiPass,25);
  if (request->hasParam("aprsChannel")) settings.aprsChannel = request->getParam("aprsChannel")->value().toInt();
  if (request->hasParam("aprsIP")) request->getParam("aprsIP")->value().toCharArray(settings.aprsIP,25);
  if (request->hasParam("aprsPort")) settings.aprsPort = request->getParam("aprsPort")->value().toInt();  
  if (request->hasParam("aprsPassword")) request->getParam("aprsPassword")->value().toCharArray(settings.aprsPassword,6);  
  if (request->hasParam("serverSsid")) settings.serverSsid = request->getParam("serverSsid")->value().toInt();
  if (request->hasParam("aprsGatewayRefreshTime")) settings.aprsGatewayRefreshTime = request->getParam("aprsGatewayRefreshTime")->value().toInt();
  if (request->hasParam("call")) request->getParam("call")->value().toCharArray(settings.call,8); 
  if (request->hasParam("ssid")) settings.ssid = request->getParam("ssid")->value().toInt();
  //if (request->hasParam("symbool")) DrawDebugInfo(request->getParam("symbol")->value());  
  if (request->hasParam("dest")) request->getParam("dest")->value().toCharArray(settings.dest,8); 
  if (request->hasParam("destSsid")) settings.destSsid = request->getParam("destSsid")->value().toInt();
  if (request->hasParam("path1")) request->getParam("path1")->value().toCharArray(settings.path1,8); 
  if (request->hasParam("path1Ssid")) settings.path1Ssid = request->getParam("path1Ssid")->value().toInt();
  if (request->hasParam("path2")) request->getParam("path2")->value().toCharArray(settings.path2,8); 
  if (request->hasParam("path2Ssid")) settings.path2Ssid = request->getParam("path2Ssid")->value().toInt();
  if (request->hasParam("comment")) request->getParam("comment")->value().toCharArray(settings.comment,16);
  if (request->hasParam("interval")) settings.interval = request->getParam("interval")->value().toInt();  
  if (request->hasParam("multiplier")) settings.multiplier = request->getParam("multiplier")->value().toInt();  
  if (request->hasParam("power")) settings.power = request->getParam("power")->value().toInt();  
  if (request->hasParam("height")) settings.height = request->getParam("height")->value().toInt();  
  if (request->hasParam("gain")) settings.gain = request->getParam("gain")->value().toInt();  
  if (request->hasParam("directivity")) settings.directivity = request->getParam("directivity")->value().toInt();            
  settings.autoShift = request->hasParam("autoShift");
  settings.bcnAfterTX = request->hasParam("bcnAfterTX");
  if (request->hasParam("txTimeOut")) settings.txTimeOut = request->getParam("txTimeOut")->value().toInt(); 
  if (request->hasParam("lat")) settings.lat = request->getParam("lat")->value().toFloat(); 
  if (request->hasParam("lat")) settings.lon = request->getParam("lon")->value().toFloat();     
  if (request->hasParam("maxChannel")) settings.maxChannel = request->getParam("maxChannel")->value().toInt();
  settings.isDebug = request->hasParam("isDebug");
}