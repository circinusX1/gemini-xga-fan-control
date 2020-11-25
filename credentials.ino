/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0 + sizeof(ssid), password);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password) > 0 ? "********" : "<no password>");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0 + sizeof(ssid), password);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}


const int offset = 128;
/** Load WLAN credentials from EEPROM */
void loadConfig() {
  char  _sstart_temp[24]={0};
  char  _sstart_duty[24]={0};
  char  _salarm_temp[24]={0};
  char  _marker[4]={0};
  
  EEPROM.begin(512);
  EEPROM.get(offset, _marker);
  EEPROM.get(offset + sizeof(_marker), _sstart_temp);
  EEPROM.get(offset + sizeof(_sstart_temp) + sizeof(_marker), _sstart_duty);
  EEPROM.get(offset + sizeof(_sstart_temp) + sizeof(_sstart_duty) + sizeof(_marker), _salarm_temp);
  EEPROM.end();
  if(_marker[0]=='Y')
  {
      _start_temp = atoi(_sstart_temp);
      _start_duty = atoi(_sstart_duty);
      _alarm_temp= atoi(_salarm_temp);
  }
  else
  {
    _start_duty = 30;
    _start_temp  = 45;
    _alarm_temp = 80;
  }
  
}

/** Store WLAN credentials to EEPROM */
void saveConfig() {
  char  _sstart_temp[24];
  char  _sstart_duty[24];
  char  _salarm_temp[24]={0};
  char  _marker[4]="Y";

  sprintf(_sstart_temp,"%d",_start_temp);
  sprintf(_sstart_duty,"%d",_start_duty);
  sprintf(_salarm_temp,"%d",_alarm_temp);
  
  EEPROM.begin(512);
  EEPROM.put(offset, _marker);
  EEPROM.put(offset + sizeof(_marker), _sstart_temp);
  EEPROM.put(offset + sizeof(_sstart_temp) + sizeof(_marker), _sstart_duty);
  EEPROM.put(offset + sizeof(_sstart_temp) + sizeof(_sstart_duty) + sizeof(_marker), _salarm_temp);
  EEPROM.commit();
  EEPROM.end();
}
