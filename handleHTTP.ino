/** Handle root or redirect to captive portal */

extern char _temp[32];
extern char _press[32];
extern char _pwm[32];

extern int     _start_duty;
extern int     _start_temp;
extern int     _alarm_temp;
struct Temps;
extern Temps  _a_temp;


void handleRoot() 
{
  if (captivePortal()) 
  {
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page += F(
            "<!DOCTYPE html><html lang='en'><head>\n"
            "<meta http-equiv='refresh' content='15' URL='/' />\n"
            "<meta name='viewport' content='width=device-width'>\n"
            "<title>Marius Power Amp</title></head>\n<body>\n"
            "<h4><div align='center' style='width:400px'>");
            
  if (server.client().localIP() == apIP) 
  {
    Page += String(F("<p>AP ON: ")) + softAP_ssid + F("</p>\n");
  } 
  else 
  {
    Page += String(F("<p>WIFI ON: ")) + ssid + F("</p>\n");
  }
  
  Page += F("<br>\n<li>Temperature: ");
  Page += _temp;
  Page += F("<br>\n<li>Pressure:");
  Page += _press;
  Page += F("<br>\n<li>PWM:");
  Page += _pwm; 
  Page += F("<br>\n<li>Trigger Temp: ");
  Page += _start_temp;
  Page += F("<br>\n<li>Alarm Temp: ");
  Page += _alarm_temp;
  Page += F("<br>\n<li>Min PWM: ");
  Page += _start_duty;
  Page += F("<br>\n");       
  
 
  Page += F(
            "<li><a href='/wifi'>WIFI CONFIG</a>.</li>\n"
            "<li><a href='/config'>FANS CONFIG</a>.</li>\n"
            "</div></h4>\n");

  Page += F("\n<canvas width='610' height='256' id='ka'>\n");
  
            
 
  
  Page += F("\n<script>\n");
  Page += F("function SCAL_E(x) { return 2.0 * x;};\n");
  Page += F("var c=document.getElementById(\"ka\");\n");
  Page += F("var ctx=c.getContext(\"2d\");\n");
  Page += F("var w=c.width;\n");
  Page += F("var h=c.height;\n");
  
  Page += F("var _alarm_temp=");
  Page += _alarm_temp;
  Page += F(";\n");
  Page += F("var _start_temp=");
  Page += _start_temp;
  Page += F(";\n");

  Page += F("var tempss = [");
  for(int e = 0; e < SAMPLES_MAX-1;e++)
  {
    Page += int(_a_temp._data[e] * 2.5);
    Page += F(",");
  }
  Page += F("0];\n");
  
  Page += F("ctx.beginPath();\n");
  Page += F("ctx.rect(0, 0, w, h);\n");
  Page += F("ctx.fillStyle = \"#323\";\n");
  Page += F("ctx.fill(); \n");


  Page += F("ctx.resetTransform();\n");
  Page += F("ctx.scale(1, -1);\n");
  Page += F("ctx.translate(0,-h);\n");

  Page += F("ctx.beginPath();\n");
  Page += F("ctx.strokeStyle = \"#F55\";\n");
  
  Page += F("ctx.moveTo(10,10);\n");
  Page += F("ctx.lineTo(10,h);\n");
  Page += F("ctx.moveTo(10,10);\n");
  Page += F("ctx.lineTo(w,10);\n");
  Page += F("ctx.stroke();\n");

  Page += F("ctx.beginPath();\n");
  Page += F("ctx.strokeStyle = \"#A33\";\n");
  Page += F("ctx.moveTo(10,SCAL_E(_alarm_temp)+10);\n");
  Page += F("ctx.lineTo(w,SCAL_E(_alarm_temp)+10);\n");
  Page += F("ctx.stroke();\n");

  Page += F("ctx.beginPath();\n");
  Page += F("ctx.strokeStyle = \"#355\";\n");
  Page += F("ctx.strokeStyle = \"#3A3\";\n");
  Page += F("ctx.moveTo(10,SCAL_E(_start_temp)+10);\n");
  Page += F("ctx.lineTo(w,SCAL_E(_start_temp)+10);\n");
   Page += F("ctx.stroke();\n");
  
  Page += F("ctx.beginPath();\n");
  Page += F("ctx.strokeStyle = \"#DFD\";\n");
  Page += F("var xo=0;\n");
  Page += F("var yo=tempss[0];\n");
  Page += F("for(var x=1; x < tempss.length; x++)\n");
  Page += F("{\n");
  Page += F("    ctx.moveTo(xo+10, yo+10);\n");
  Page += F("    ctx.lineTo(x+10, tempss[x]+10);\n");
  Page += F("    if(tempss[x]==0) break;\n");
  Page += F("    xo=x;\n");
  Page += F("    yo=tempss[x];\n");
  Page += F("}\n");
  Page += F("ctx.stroke();\n");
  Page += F("ctx.resetTransform();\n");
  Page += F("ctx.font = \"12px Arial\";\n");
  Page += F("ctx.fillStyle = \"#FFF\";\n");
  Page += F("ctx.fillText(\"Alarm:\"+_alarm_temp,11, h/4);\n");
  Page += F("ctx.fillText(\"Trigger:\"+_start_temp,11, h/2);\n");
  
  Page += F("</script>\n\n");
  Page += F("</body>\n</html>\n");
   
  server.send(200, "text/html", Page);
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle 
the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local")) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Wifi config page handler */
void handleWifi() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page += F(
            "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>poweramp</title></head><body><h4>"
            "<h1>Wifi config</h1>");
  if (server.client().localIP() == apIP) {
    Page += String(F("<p>Connected toSoft AP: ")) + softAP_ssid + F("</p>");
  } else {
    Page += String(F("<p>Connected to Wifi network: ")) + ssid + F("</p>");
  }
  Page +=
    String(F(
             "\r\n<br />"
             "<table><tr><th align='left'>SoftAP config</th></tr>"
             "<tr><td>SSID ")) +
    String(softAP_ssid) +
    F("</td></tr>"
      "<tr><td>IP ") +
    toStringIp(WiFi.softAPIP()) +
    F("</td></tr>"
      "</table>"
      "\r\n<br />"
      "<table><tr><th align='left'>WLAN config</th></tr>"
      "<tr><td>SSID ") +
    String(ssid) +
    F("</td></tr>"
      "<tr><td>IP ") +
    toStringIp(WiFi.localIP()) +
    F("</td></tr>"
      "</table>"
      "\r\n<br />"
      "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>");
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      Page += String(F("\r\n<tr><td>SSID ")) + WiFi.SSID(i) + ((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? F(" ") : F(" *")) + F(" (") + 
WiFi.RSSI(i) + F(")</td></tr>");
    }
  } else {
    Page += F("<tr><td>No WLAN found</td></tr>");
  }
  Page += F(
            "</table>"
            "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
            "<input type='text' placeholder='network' name='n'/>"
            "<br /><input type='password' placeholder='password' name='p'/>"
            "<br /><input type='submit' value='Connect/Disconnect'/></form>"
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</h4></body></html>");
  server.send(200, "text/html", Page);
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  Serial.println("wifi save");
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}


void handleConfig() {
  char tempstart[8]="";
  char pwmstart[8]="";
  char alarmtemp[8]="";
    
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  bool saved = false ;
  if(server.args()!=0)
  {

    server.arg("T").toCharArray(tempstart, sizeof(tempstart) - 1);
    server.arg("D").toCharArray(pwmstart, sizeof(pwmstart) - 1);
    server.arg("A").toCharArray(alarmtemp, sizeof(alarmtemp) - 1);
    _start_duty = ::atoi(pwmstart);
    _start_temp = ::atoi(tempstart);
    _alarm_temp = ::atoi(alarmtemp);
    saveConfig();
    saved =  true; 
  }
  
  String Page;
  Page += F(
            "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>poweramp</title></head><body><h4>"
            "<h2>Fans config</h2>");
// extern int     _start_duty;
// extern int     _start_temp;
  Page +=  F("\n<form method='POST' action='config'>\n");
  Page +=  F("<li>Start Temperature: <input type='text' name='T' value='");
  Page += String(_start_temp); 
  Page +=  F("'></li>\n");          
  Page +=  F("<li>Alarm Temperature: <input type='text' name='A' value='");
  Page += String(_alarm_temp); 
  Page +=  F("'></li>\n");          
  Page +=  F("<li>Start Duty: <input type='text' name='D' value='");
  Page += String(_start_duty); 
  Page +=  F("'></li>\n");          
  Page +=  F("<input type='submit' value='Save'/></form>\n");
  if(saved)
    Page += F("\n<br><a href='/'>Config Saved</a>\n"); 
  else  
    Page += F("\n<br><a href='/'>HOME</a>\n"); 
  Page += F("</h4></body></html>");
  server.send(200, "text/html", Page);
  server.client().stop(); // Stop is needed because we sent no content length
}
