#define TIMEZONE euCET // See NTP_Time.h tab for other "Zone references", UK, usMT etc

const wlanSSID wifiNetworks[] {
    {"PI4RAZ","PI4RAZ_Zoetermeer"},
    {"Loretz_Gast", "Lor_Steg_98"}
};

const Repeater repeaters[] = {
    {"","",0,0,0,0},
    {"PI3UTR","IJsselstein",-1,126,4,2},
    {"PI3ALK","Alkmaar",-1,128,8,3},
    {"PI3ZVL","Overslag",-1,128,0,0},
    {"PI3TWE","Eibergen",-1,128,4,3},
    {"PI3SRT","Venlo",-1,128,2,3},
    {"PI3APM","Appingendam",-1,129,6,3},
    {"PI3BOZ","Bergen/Zoom",-1,130,8,3},
    {"PI3ZOD","Emmen",-1,130,0,0},
    {"PI3AMF","Amersfoort",-1,130,0,0},
    {"PI3FRL","Leeuwarden",-1,131,6,3},
    {"PI3ZUT","Zutphen",-1,131,0,0},
    {"PI3MEP","Meppel",-1,132,6,3},
    {"PI3BRD","Breda",-1,132,2,3},
    {"PI3VLI","Vlissingen",-1,133,8,3},
    {"PI3RTD","Rotterdam",-1,134,8,3},
    {"PI3NOV","'t Harde",-1,134,4,3},
    {"PI3VHP","Vroomshoop",-1,135,4,3},
    {"PI3YMD","IJmuiden",-1,135,8,3},
    {"PI3HVN","Heerenveen",-1,136,6,3},
    {"PI3EHV","Eindhoven",-1,136,2,3},
    {"PI3DTC","Doetinchem",-1,136,4,3},
    {"PI3ZAZ","Wormerveer",-1,137,8,3},
    {"PI3GOE","Goes",-1,138,8,3},
    {"PI3APD","Apeldoorn",-1,138,4,3},
    {"PI3ZLB","Geleen",-1,138,2,3},
    {"PI3FLD","Lelystad",-1,139,4,3},
    {"PI3NYM","Nijmegen",-1,140,4,3},
    {"PI3GRN","Groningen",-1,140,6,3},
    {"PI3RAZ","Zoetermeer",-1,140,8,3},
    {"PI3ALM","Almere",-1,141,0,0},
    {"PI3ASD","Amsterdam",-1,142,8,3}
};

const CTCSSCode cTCSSCodes[] = {
    {"0000","None"},
    {"0001","67"},
    {"0002","71,9"},
    {"0003","74,4"},
    {"0004","77"},
    {"0005","79,7"},
    {"0006","82,5"},
    {"0007","85,4"},
    {"0008","88,5"},
    {"0009","91,5"},
    {"0010","94,8"},
    {"0011","97,4"}, 
    {"0012","100"}, 
    {"0013","103,5"},
    {"0014","107,2"},
    {"0015","110,9"},
    {"0016","114,8"},
    {"0017","118,8"},
    {"0018","123"},
    {"0019","127,3"},
    {"0020","131,8"},
    {"0021","136,5"},
    {"0022","141,3"},
    {"0023","146,2"},
    {"0024","151,4"},
    {"0025","156,7"},
    {"0026","162,2"},
    {"0027","167,9"},
    {"0028","173,8"},
    {"0029","179,9"},
    {"0030","186,2"},
    {"0031","192,8"},
    {"0032","203,5"},
    {"0033","210,7"},
    {"0034","218,1"},
    {"0035","225,7"},
    {"0036","233,6"},
    {"0037","241,8"},
    {"0038","250,3"}
};

Settings settings = {
    '%',                //chkDigit
    0,                  //isUHF
    64,                 //aprsChannel
    140,                //rxChannel
    92,                 //txChannel
    0,                  //repeater
    0,                  //memorychannel
    0,                  //freqType
    -1,                 //txShift (-1=-600,  = 0, 1=+600)
    1,                  //autoShift
    0,                  //low DRAPower
    3,                  //hasTone (0=none, 1=rx, 2=tx, 3=rxtx)
    8,                  //ctcssTone
    8,                  //volume
    1,                  //squelsh
    1,                  //scanType (0=stop, 1=resume)
    "YourSSID",         //wifiSSID[25];
    "WiFiPassword",     //wifiPass[25];
    "rotate.aprs.net",  //aprsIP
    14580,              //aprsPort
    "99999",            //aprsPassword    
    "APZRAZ",           //dest[8]
    0,                  //destSsid
    "PI4RAZ",           //call[8]
    7,                  //ssid
    14,                 //serverSsid
    600,                //aprsGatewayRefreshTime
    "73 de PI4RAZ",     //comment[16]
    '>',                //symbool
    "WIDE1",            //path1[8]
    1,                  //path1Ssid
    "WIDE2",            //path2[8]
    2,                  //path2Ssid
    30,                 //interval (in seconds, complete interval = interval*multiplier)
    10,                 //multiplier
    5,                  //power
    0,                  //height
    0,                  //gain
    0,                  //directivity
    500,                //preAmble  
    100,                //tail
    1,                  //doTX
    0,                  //bcnAfterTX
    120,                //txTimeOut
    160,                //maxChannel (160=146.000MHz)
    22880,              //minUHFChannel (430.000MHz)
    23680,              //maxUHFChannel (440.000MHz)
    100,                //Brightness %
    52.903611,          //latitude
    4.461667,           //longitude
    1,                  //useAPRS
    8,                  //mikeLevel
    0,                  //preEmphase
    0,                  //highPass
    0,                  //lowPass
    0                   //isDebug
};

int screenRotation  = 1;                            // 0=0, 1=90, 2=180, 3=270
uint16_t calData[5] = { 378, 3473, 271, 3505, 7 };