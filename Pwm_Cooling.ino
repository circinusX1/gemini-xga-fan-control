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

static const   char *softAP_ssid = APSSID;
static const   char *softAP_password = APPSK;
static const   char *myHostname = "poweramp";
static char    ssid[33] = "marius";
static char    password[65] = "********";
static const   byte DNS_PORT = 53;

static DNSServer           dnsServer;
ESP8266WebServer           server(80);
static IPAddress           apIP(10, 5, 5, 1);
static IPAddress           netMsk(255, 255, 0, 0);
static IPAddress           primaryDNS(8, 8, 8, 8); // optional
static IPAddress           secondaryDNS(8, 8, 4, 4); // optional

// wifi according to local network
static IPAddress _ip(192,168,1, 228);
static IPAddress _gw(192,168,1, 1);
static IPAddress _sn(255, 255, 255, 0);

static boolean             _connect;
static unsigned long       lastConnectTry = 0;
static unsigned int        status = WL_IDLE_STATUS;
static Adafruit_BMP085     bmp;



static const int _led_by_usb = 16;
static const int _led_by_ant = 2;
static const int _pw1 = 12;
static const int _pw2 = 13;
static const int _buz = 15;
static int       _modulo=128;


static const int _max_duty = 1023;
static const int _tmp_max = 60;
static int     _start_duty = 30;
static int     _start_temp = 45;
static int     _alarm_temp = 80;
static int     _read_interval = 100;
static float   _ftemp = 0.0;

int     _test_pwm = 0;
char    _temp[32]="";
char    _press[32]="";
char    _pwm[32]="";
int     _loop=0;
bool    _alarm = false;
int     _freq = 22000;

#define SAMPLES_MAX 600
struct Temps
{
    int   _count;
    float _data[SAMPLES_MAX];
};

Temps  _a_temp = {0,{0}};

//
// read and calculate p1m
//
void shot()
{
    _ftemp = bmp.readTemperature();
    float pressu= bmp.readPressure();
    sprintf(_temp,"%f °C ",_ftemp);
    sprintf(_press,"%f mmHg ",pressu);
    int pwm = _calc_pwm(_ftemp);
    if(_ftemp > float(_alarm_temp))
    {
        pwm = _max_duty;
        Serial.println("ALARM");
        _alarm=true;
        buzz(100);
        _modulo = 32;
    }
    else
    {
        _alarm=false;
        _modulo = 128;
    }
    analogWrite(_pw1,pwm);
    analogWrite(_pw2,pwm);
    sprintf(_pwm,"%d / [%d] ", int((float(pwm) * 100.0f) / _max_duty), pwm);
}


void setup() {

    pinMode(_led_by_usb, OUTPUT);
    pinMode(_led_by_ant, OUTPUT);
    pinMode(_buz, OUTPUT);
    digitalWrite(_led_by_usb, HIGH);
    digitalWrite(_led_by_ant, HIGH);
    analogWriteFreq(_freq);
    analogWrite(_pw1, _max_duty/2);
    analogWrite(_pw2, _max_duty/2);
    delay(128);
    analogWrite(_pw1, _max_duty);
    analogWrite(_pw2, _max_duty);
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Configuring access point...");
delay(1000);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(softAP_ssid, softAP_password);
    delay(512);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
delay(1000);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);

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
    _connect = strlen(ssid) > 0; // Request WLAN _connect if there is a SSID
    bmp.begin();
   
    delay(1000);
    loadConfig();
    delay(1000);
   
    
    shot();
    pinMode(_led_by_ant, OUTPUT);
    digitalWrite(_led_by_usb, LOW);
    digitalWrite(_led_by_ant,LOW);
}

void connectWifi() {
    Serial.println("Connecting as wifi client...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, password);
    if (!WiFi.config(_ip, _gw, _sn, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
    }
    delay(100);
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
    if(_test_pwm)
      ppwm = _test_pwm % 1023; 
    return ppwm;
}

//
// buzzer
//
void IRAM_ATTR delayMicroseconds2(uint32_t us);
void buzz(int timed)
{
    int k = timed;
    digitalWrite(_led_by_ant, LOW);
    digitalWrite(_led_by_usb, LOW);
    while(k-->0)
    {
        delayMicroseconds2(128);
        digitalWrite(_buz,1);
        delayMicroseconds2(128);
        digitalWrite(_buz,0);
    }
    digitalWrite(_led_by_ant, HIGH);
    digitalWrite(_led_by_usb, HIGH);
}

void loop()
{
    if (_connect) {
        Serial.println("Connect requested");
        _connect = false;
        connectWifi();
        lastConnectTry = millis();
    }
    {
        unsigned int s = WiFi.status();
        if (s == 0 && millis() > (lastConnectTry + 60000)) {
            /* If WLAN disconnected and idle try to _connect */
            /* Don't set retry time too low as retry interfere the softAP operation */
            _connect = true;
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

        if(_loop%128==0){
            digitalWrite(_led_by_usb, LOW);
            delay(32);
            digitalWrite(_led_by_usb, HIGH);
            digitalWrite(_led_by_ant, HIGH);
        }

        if(_loop% _modulo==0)
        {
            char data[64];

            shot();
            Serial.println("reading temp ");
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

            sprintf(data, " t= %f alarm = %f  mod = %d", _ftemp, float(_alarm_temp), _modulo);
            Serial.println(data);
        }

    }
    dnsServer.processNextRequest();
    server.handleClient();
    delay (8);

    ++_loop;
}
