#include <Adafruit_BMP085.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>


#ifndef APSSID
#define APSSID "poweramp"
#define APPSK  "power2000"
#endif

#define MY_BLUE_LED_PIN  16
#define MY_BLUE_LED_PIN2 2
#define MY_BUZZER_PIN    10

const char *softAP_ssid = APSSID;
const char *softAP_password = APPSK;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "poweramp";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[33] = "marius";
char password[65] = "myssidpass";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(10, 5, 5, 1);
IPAddress netMsk(255, 255, 0, 0);


/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
unsigned long lastConnectTry = 0;

/** Current WLAN status */
unsigned int status = WL_IDLE_STATUS;

Adafruit_BMP085 bmp;

const int _pw1 = 14; 
const int _pw2 = 13; 
const int _pw3 = 12; 
const int _pw4 = 15;
 
const int _max_duty = 1023;
const int _tmp_max = 60;

int     _start_duty = 30;
int     _start_temp = 45;
int     _alarm_temp = 80;
int     _read_interval = 100;

char    _temp[32]="";
char    _press[32]="";
char    _pwm[32]="";
int     _loop=0;
float   _ftemp = 0.0;

#define SAMPLES_MAX 600  
struct Temps
{
  int   _count;
  float _data[SAMPLES_MAX];
};


Temps  _a_temp = {0,{0}};


void shot()
{
    _ftemp = bmp.readTemperature();
    float pressu= bmp.readPressure();
    sprintf(_temp,"%f C ",_ftemp);
    sprintf(_press,"%f mmHg ",pressu);
    int pwm = _calc_pwm(_ftemp);
    analogWrite(_pw1,pwm);
    analogWrite(_pw2,pwm);
    analogWrite(_pw3,pwm); 
    analogWrite(_pw4,pwm); 
    sprintf(_pwm,"%d / [%d] ", int((float(pwm) * 100.0f) / _max_duty), pwm);
}



void setup() {

    pinMode(MY_BLUE_LED_PIN, OUTPUT);
    pinMode(MY_BLUE_LED_PIN2, OUTPUT);
    pinMode(MY_BUZZER_PIN, OUTPUT);

    digitalWrite(MY_BLUE_LED_PIN, HIGH);
    digitalWrite(MY_BLUE_LED_PIN2, HIGH);


      
    analogWriteFreq(20000);
    analogWrite(_pw1, _max_duty/2);
    analogWrite(_pw2, _max_duty/2);
    analogWrite(_pw3, _max_duty/2); 
    analogWrite(_pw4, _max_duty/2); 

    


    delay(1000);
    
    analogWrite(_pw1, _max_duty);
    analogWrite(_pw2, _max_duty);
    analogWrite(_pw3, _max_duty); 
    analogWrite(_pw4, _max_duty);

    delay(1000);
    loadConfig();
    
    delay(1000);
    Serial.begin(115200);
    Serial.println();
    Serial.println("Configuring access point...");
    
    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(softAP_ssid, softAP_password);
    delay(500); // Without delay I've seen the IP address blank
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    
    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    server.on("/", handleRoot);
    server.on("/index.html", handleRoot);
    server.on("/wifi", handleWifi);
    server.on("/wifisave", handleWifiSave);
    server.on("/config", handleConfig);
    server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server.onNotFound(handleNotFound);
    server.begin(); // Web server start
    Serial.println("HTTP server started");
    loadCredentials(); // Load WLAN credentials from network
    connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
    bmp.begin();
    shot();
    digitalWrite(MY_BLUE_LED_PIN, LOW);
    digitalWrite(MY_BLUE_LED_PIN2,LOW);
}

void connectWifi() {
    Serial.println("Connecting as wifi client...");
    
    
    
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    
    
    IPAddress _ip = IPAddress(192,168,1, 229);
    IPAddress _gw = IPAddress(192,168,1, 1);
    IPAddress _sn = IPAddress(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8); // optional
    IPAddress secondaryDNS(8, 8, 4, 4); // optional
    
    
    if (!WiFi.config(_ip, _gw, _sn, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
    }    
    
    
    int connRes = WiFi.waitForConnectResult();
    Serial.print("connRes: ");
    Serial.println(connRes);
}


int _calc_pwm(float temp)
{
    int ppwm = 0;
    float maxrange   = _tmp_max -  _start_temp; // from _startduty -> max_duty
    float fstartpwm =  (float(_start_duty) / 100.0f) * _max_duty;
    float remainpwm = _max_duty - fstartpwm;
    float ct = temp;
    if(temp - _start_temp > 0.0f)
    {
        if(ct>_tmp_max)
            ct=_tmp_max;
        float curtemp = ct - _start_temp;
        float load = curtemp / maxrange;


        float curpwm = (remainpwm * load) + fstartpwm;
        if(curpwm > 1000.0f)
            curpwm = _max_duty;
        ppwm = int(curpwm);
    }
    return ppwm; 
}

void IRAM_ATTR delayMicroseconds2(uint32_t us);
void buzz(int timed)
{
    unsigned long now = millis() + timed;
    while(millis()<timed)
    {
         delayMicroseconds2(512);
         digitalWrite(MY_BUZZER_PIN,1);
         delayMicroseconds2(512);
         digitalWrite(MY_BUZZER_PIN,0);
    }
}

unsigned long _prevtime = 0;;
static int _level = 1;
static int _buzz = 0;
static int _level2 = 1;
void loop() {
    if (connect) {
        Serial.println("Connect requested");
        connect = false;
        connectWifi();
        lastConnectTry = millis();
    }
    {
        unsigned int s = WiFi.status();
        if (s == 0 && millis() > (lastConnectTry + 60000)) {
            /* If WLAN disconnected and idle try to connect */
            /* Don't set retry time too low as retry interfere the softAP operation */
            connect = true;
        }
        if (status != s) { // WLAN status change
            Serial.print("Status: ");
            Serial.println(s);
            status = s;
            if (s == WL_CONNECTED) {
                /* Just connected to WLAN */
                Serial.println("");
                Serial.print("Connected to ");
                Serial.println(ssid);
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                
                // Setup MDNS responder
                if (!MDNS.begin(myHostname)) {
                    Serial.println("Error setting up MDNS responder!");
                } else {
                    Serial.println("mDNS responder started");
                    // Add service to MDNS-SD
                    MDNS.addService("http", "tcp", 80);
                }
            } else if (s == WL_NO_SSID_AVAIL) {
                WiFi.disconnect();
            }
        }
        if (s == WL_CONNECTED) {
            MDNS.update();
        }

        if(_loop%10==0){
              digitalWrite(MY_BLUE_LED_PIN, _level ? LOW:HIGH);
              _level = !_level;
        }
        
        if(_loop%100==0)
        {
             digitalWrite(MY_BLUE_LED_PIN, _level2 ? LOW:HIGH);
             _level2=!_level2;
          
            //_prevtime = millis();
            shot();
            Serial.println("reading temp :" + int(_ftemp));
            if(_a_temp._count>SAMPLES_MAX)
            {
              for(s=1;s<SAMPLES_MAX;s++)
              {
                 _a_temp._data[s-1]=_a_temp._data[s];
              }
              _a_temp._data[SAMPLES_MAX-1] = _ftemp;
            }else{
              _a_temp._data[_a_temp._count++] = _ftemp;
            }
            
            
            if(_ftemp > float(_tmp_max))
            {
               Serial.println("buzzing");
               buzz(2000);
            }
        }
        
    }
    dnsServer.processNextRequest();
    server.handleClient();
    delay (120);
    ++_loop;
}
