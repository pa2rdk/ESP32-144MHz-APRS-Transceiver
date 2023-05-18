const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>APRS Radio Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- <meta http-equiv="refresh" content="1">  -->
    <link rel="icon" href="data:,">
    <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
  <div class="topnav">
    <h1>APRS Radio Server</h1>
  </div>
  <hr>
  <div class="divinfo">
    <h4 style="text-align:left;color:lightblue"><span id="APRSINFO">%APRSINFO%</span></h4>
    <h4 style="text-align:left;color:orange"><span id="GPSINFO">%GPSINFO%</span></h4>
    <h4 style="text-align:left;color:red"><span id="BEACONINFO">%BEACONINFO%</span></h4>
  </div>
  <hr>
  <div class="freqinfo">
    <h1><span id="RXFREQ">%RXFREQ%</span></h1>
  </div>
  <div class="divinfo">    
    <h4 style="text-align:center;color:yellow""><span id="REPEATERINFO">%REPEATERINFO%</span>      <span id="TXFREQ">%TXFREQ%</span></h4>
  </div>
  <hr>
  <div class="content">
    <div class="cards">
      %BUTTONS0%
    </div>
  </div>
  <hr>
  <div class="topnav" style="background-color: lightblue; ">
    <table width="100%">
      <tbody>
        <tr>
          <td style="text-align:left">
            <h4>Copyright (c) Robert de Kok, PA2RDK</h4>
          </td>
          <td style="text-align:right">
            <a href="/settings"><button>Settings</button></a>
            <a href="/reboot"><button>Reboot</button></a>
          </td>
        </tr>
      </tbody>
    </table>
  </div>

  <script>
  if (!!window.EventSource) {
    var source = new EventSource('/events');
  
    source.addEventListener('open', function(e) {
      console.log("Events Connected");
    }, false);
  
    source.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    }, false);
  
    source.addEventListener('message', function(e) {
      console.log("message", e.data);
    }, false);
  
    source.addEventListener('APRSINFO', function(e) {
      console.log("APRSINFO", e.data);
      document.getElementById("APRSINFO").innerHTML = e.data;
    }, false);
  
    source.addEventListener('GPSINFO', function(e) {
      console.log("GPSINFO", e.data);
      document.getElementById("GPSINFO").innerHTML = e.data;
    }, false);

    source.addEventListener('RXFREQ', function(e) {
      console.log("RXFREQ", e.data);
      document.getElementById("RXFREQ").innerHTML = e.data;
    }, false);

    source.addEventListener('REPEATERINFO', function(e) {
      console.log("REPEATERINFO", e.data);
      document.getElementById("REPEATERINFO").innerHTML = e.data;
    }, false);

    source.addEventListener('BEACONINFO', function(e) {
      console.log("BEACONINFO", e.data);
      document.getElementById("BEACONINFO").innerHTML = e.data;
    }, false);

    source.addEventListener('TXFREQ', function(e) {
      console.log("TXFREQ", e.data);
      document.getElementById("TXFREQ").innerHTML = e.data;
    }, false);
  }
  
  if(typeof window.history.pushState == 'function') {
    if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10){
      console.log(window.location.href);
      window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
    }
  }

  document.getElementById('BTNFreq').onclick = function(e){
    myObj = document.getElementById("BTNFreq");
    if (myObj.style.backgroundColor=="white"){
      var s = window.location.href.substring(0,window.location.href.lastIndexOf('/')) + '/nummers';
      window.location.href = s;  
      return false;
    } else return true;
  }

  document.getElementById('BTNRPT').onclick = function(e){
    myObj = document.getElementById("BTNRPT");
    if (myObj.style.backgroundColor=="white"){
      var s = window.location.href.substring(0,window.location.href.lastIndexOf('/')) + '/repeaters';
      window.location.href = s;  
      return false;
    } else return true;
  }
  </script>
  </body>
  </html>v
)rawliteral";

const char nummers_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>APRS Radio Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- <meta http-equiv="refresh" content="1">  -->
    <link rel="icon" href="data:,">
    <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
  <div class="topnav">
    <h1>APRS Radio Server</h1>
  </div>
  <hr>
    <div class="freqinfo">
      <h1><span id="RXFREQ"><input class="freqdisp" readonly type="text" placeholder="Type " id="inputFreq" value="%KEYBFREQ%"></span></h1>
    </div>
  <hr>  
  <hr>
  <div class="divinfo">
    <form action="/numbers" method="get">

    <div class="content">
        <div class="cards">
            %NUMMERS0%
        </div>
    </div>

    </form><br>
  </div>
  <hr>
  <div class="topnav" style="background-color: lightblue; ">
  <table width="100%">
      <tbody>
      <tr>
          <td style="text-align:left">
          <h4>Copyright (c) Robert de Kok, PA2RDK</h4>
          </td>
          <td style="text-align:right">
            <a href="/"><button>Main</button></a>
          </td>
      </tr>
      </tbody>
  </table>
  </div>

  <script>
  if(typeof window.history.pushState == 'function') {
    if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10){
      console.log(window.location.href);
      window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
    }
  }

  document.getElementById('BTNA000').onclick = function(e) {
    handleNummer(0);
    return false;
  }

  document.getElementById('BTNA001').onclick = function(e) {
    handleNummer(1);
    return false;
  }

  document.getElementById('BTNA002').onclick = function(e) {
    handleNummer(2);
    return false;
  }

  document.getElementById('BTNA003').onclick = function(e) {
    handleNummer(3);
    return false;
  }

  document.getElementById('BTNA004').onclick = function(e) {
    handleNummer(4);
    return false;
  }

  document.getElementById('BTNA005').onclick = function(e) {
    handleNummer(5);
    return false;
  }

  document.getElementById('BTNA006').onclick = function(e) {
    handleNummer(6);
    return false;
  }

  document.getElementById('BTNA007').onclick = function(e) {
    handleNummer(7);
    return false;
  }

  document.getElementById('BTNA008').onclick = function(e) {
    handleNummer(8);
    return false;
  }

  document.getElementById('BTNA009').onclick = function(e) {
    handleNummer(9);
    return false;
  }

  document.getElementById('BTNEnter').onclick = function(e) {
    var i = document.getElementById('inputFreq').value;
    i = (i*10000)
    var s = window.location.href.substring(0,window.location.href.lastIndexOf('/')) + '/command?setfreq=' + i;
    window.location.href = s;
    return false;
  }

  document.getElementById('BTNClear').onclick = function(e) {
    var s = window.location.href.substring(0,window.location.href.lastIndexOf('/')) + '/';
    window.location.href = s;  
    return false;
  }

  function handleNummer(nummer){
    var i = document.getElementById('inputFreq').value;
    i = (i*100000) + nummer;
    i = i/10000;
    if (i>440 || (i>146 && i<430)) i = nummer/10000;
    document.getElementById('inputFreq').value = parseFloat(i).toFixed(4);
  }
  </script>

  </body>
  </html>
)rawliteral";

const char repeaters_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>APRS Radio Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- <meta http-equiv="refresh" content="1">  -->
    <link rel="icon" href="data:,">
    <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
  <div class="topnav">
    <h1>APRS Radio Server</h1>
  </div>
  <hr>
    <div class="freqinfo">
      <select id="repeaters" onChange="selectRepeater()">
        %REPEATERS0%
      </select>
    </div>

  <hr>  
  <div class="topnav" style="background-color: lightblue; ">
  <table width="100%">
      <tbody>
      <tr>
          <td style="text-align:left">
          <h4>Copyright (c) Robert de Kok, PA2RDK</h4>
          </td>
          <td style="text-align:right">
            <a href="/"><button>Main</button></a>
          </td>
      </tr>
      </tbody>
  </table>
  </div>

  <script>
  if(typeof window.history.pushState == 'function') {
    if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10){
      console.log(window.location.href);
      window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
    }
  }

  function selectRepeater() {
    var x = document.getElementById("repeaters").value;
    var s = window.location.href.substring(0,window.location.href.lastIndexOf('/')) + '/command?setrepeater=' + x;
    window.location.href = s;
    return false;
  }
  </script>

  </body>
  </html>
)rawliteral";

const char settings_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>APRS Radio Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- <meta http-equiv="refresh" content="1">  -->
    <link rel="icon" href="data:,">
    <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
  <div class="topnav">
    <h1>APRS Radio Server</h1>
    <br>
    <h2>Settings</h2>
  </div>
  <hr>
  <div class="divinfo">
    <form action="/store" method="get">

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              WiFi SSID: 
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="wifiSSID" value="%wifiSSID%">
            </td>
          </tr>
        </table>
      </div>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              WiFi Password:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="password" name="wifiPass" value="%wifiPass%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Channel:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="aprsChannel" value="%aprsChannel%" min="0" max="%maxChannel%">
              <span style="color:red;font-size: medium;">%aprsFreq%</span>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS IP:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="aprsIP" value="%aprsIP%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Port:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="aprsPort" value="%aprsPort%" min="0" max="99999">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Password:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="aprsPassword" value="%aprsPassword%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Server SSID:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="serverSsid" value="%serverSsid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Server refresh interval:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="aprsGatewayRefreshTime" value="%aprsGatewayRefreshTime%" min="0" max="86400">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Call:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="call" value="%call%">
              <input type="number" name="ssid" value="%ssid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Symbol:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="symbool" value="%symbool%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Dest:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="dest" value="%dest%">
              <input type="number" name="destSsid" value="%destSsid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Path1:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="path1" value="%path1%">
              <input type="number" name="path1Ssid" value="%path1Ssid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>     

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Path2:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="path2" value="%path2%">
              <input type="number" name="path2Ssid" value="%path2Ssid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Comment:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="comment" value="%comment%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Interval:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="interval" value="%interval%" min="0" max="255">
              &nbsp;Multiplier:
              <input type="number" name="multiplier" value="%multiplier%" min="0" max="255">
            </td>
          </tr>
        </table>
      </div>
      <br> 
      
      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Power:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="power" value="%power%" min="0" max="255">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Height:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="height" value="%height%" min="0" max="255">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Gain:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="gain" value="%gain%" min="0" max="255">
            </td>
          </tr>
        </table>
      </div>
      <br> 
      
      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Directivity:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="directivity" value="%directivity%" min="0" max="359">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Auto shift:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="autoShift" value="autoShift" %autoShift%>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Disable RX tone:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="disableRXTone" value="disableRXTone" %disableRXTone%>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Beacon after TX:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="bcnAfterTX" value="bcnAfterTX" %bcnAfterTX%>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              TX Timeout:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="txTimeOut" value="%txTimeOut%" min="0" max="240">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Lat:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="lat" value="%lat%" min="-90" max="90" step="any">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Lon:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="lon" value="%lon%" min="-180" max="180" step="any">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              MAX Channel:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="maxChannel" value="%maxChannel%" min="0" max="320">
              <span style="color:red;font-size: medium;">%maxFreq%</span>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Mike level:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="mikeLevel" value="%mikeLevel%" min="0" max="8">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Repeat Set DRA:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="repeatSetDRA" value="%repeatSetDRA%" min="1" max="5">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Debugmode:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="isDebug" value="isDebug" %isDebug%>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="fwidth" style="text-align:center;font-size: medium;">
              <input style="font-size: medium;"" type="submit" value="Submit">
            </td>
          </tr>
        </table>
      </div>
      
    </form><br>
  </div>
  <hr>

  <div class="topnav" style="background-color: lightblue; ">
    <table class="fwidth">
        <tr>
          <td class="hwidth" style="text-align:left">
            <h4>Copyright (c) Robert de Kok, PA2RDK</h4>
          </td>
          <td style="text-align:right">
            <a href="/"><button>Main</button></a>
            <a href="/reboot"><button>Reboot</button></a>
          </td>
        </tr>
    </table>
  </div>

  <script>
    if(typeof window.history.pushState == 'function') {
      if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10 || window.location.href.lastIndexOf('/store')>10){
        console.log(window.location.href);
        window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
      }
    }
    </script>

  </body>
  </html>  
)rawliteral";

const char css_html[] PROGMEM = R"rawliteral(
html {
    font-family: Arial; 
    display: inline-block; 
    text-align: center;
    background-color: black; 
}
p { 
    font-size: 1.2rem;
}
body {  
    margin: 0;
}
.hwidth {
    width: 50%;
}

.fwidth {
    width: 100%;
}

.freqdisp {
    border: none;
    background-color: black;
    color: yellow;
    font-size: xx-large;
    border-bottom: 1px solid gray;
    text-align: center;
}

.topnav { 
    overflow: hidden; 
    background-color: blue; 
    color: white; 
    font-size: 1rem; 
    line-height: 0%;
}
.divinfo { 	
    overflow: hidden; 
    background-color: black; 
    color: white; 
    font-size: 0.7rem; 
    height:100;
    line-height: 0%;
}
.freqinfo { 	
    overflow: hidden; 
    background-color: black;
    color: yellow; 
    font-size: 1rem; 
    line-height: 0%;
}
.content { 
    padding: 20px; 
}
.card { 
    background-color: lightblue; 
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); 
    font-size: 0.7rem; 
}
.cards { 
    max-width: 1000px; 
    margin: 0 auto; 
    display: grid; 
    grid-gap: 1rem; 
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); 
}
.reading { 
    font-size: 1.4rem;  
}
})rawliteral";

