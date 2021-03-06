#if Termostato_nodi>0

#include <TPush.h>
#include "DHT.h"
#include "SSD1306.h"

#define DHTTYPE DHT22
#define Termostato_TEMPO_TERMOSTATO     5000

float   soglia_scatto_Termostato1 = 0.25;
float   soglia_aggiornamento_temperatura = 0.2;

String Termostato_DEF     = "TER";
String Termostato_ON      = "on";
String Termostato_OFF     = "off";
String Termostato_MAN     = "man";
String Termostato_AUTO    = "auto";
String Termostato_STATO   = "stato";
String Termostato_READ    = "read";
String Termostato_ACK_ON  = "on";
String Termostato_ACK_OFF = "off";
String Termostato_ACK_MAN = "man";
String Termostato_ACK_AUTO = "auto";
String Termostato_Topic_ACK_suffisso = "/ack";

typedef struct {
  String topic;
  String topic_ACK;
  int rele;
  int dht;
  int i2c;
  int sda;
  int scl;
  float t;
  float h;
  float hi;
  float termostato;
  boolean AUTO;
} InfoRecordTermostatoType ;

InfoRecordTermostatoType Termostato[1];

TPush Termostato_timer[Termostato_nodi];
DHT dht(Termostato1_GPIO_DHT, DHTTYPE);
SSD1306 display(Termostato1_I2C_DISPLAY_ADDRESS, Termostato1_GPIO_SDA, Termostato1_GPIO_SCL);

void setup_Termostato() {
  Termostato[0].topic = Termostato1_Topic;
  Termostato[0].topic_ACK = Termostato1_Topic;
  Termostato[0].topic_ACK += Termostato_Topic_ACK_suffisso;
  Termostato[0].rele = Termostato1_GPIO_rele;
  Termostato[0].dht = Termostato1_GPIO_DHT;
  Termostato[0].i2c = Termostato1_I2C_DISPLAY_ADDRESS;
  Termostato[0].sda = Termostato1_GPIO_SDA;
  Termostato[0].scl = Termostato1_GPIO_SCL;
  Termostato[0].AUTO = false;
  Termostato[0].termostato = 18.00;

  for (int i = 0; i < Termostato_nodi; i++) {
    Debug_MSG_LN("- Termostato " + String(i + 1) + " : " + String(Termostato[i].topic));
    Debug_MSG_LN("RELE termostato " + String(i + 1) + "      = GPIO" + String(Termostato[i].rele));
    Debug_MSG_LN("DTH22 termostato " + String(i + 1) + "       = GPIO" + String(Termostato[i].dht));
    Debug_MSG_LN("SSD1306 ADDRESS termostato " + String(i + 1) + " = 0x"   + String(Termostato[i].i2c, HEX));
    Debug_MSG_LN("SSD1306 SDA termostato " + String(i + 1) + " = GPIO" + String(Termostato[i].sda));
    Debug_MSG_LN("SSD1306 SCL termostato " + String(i + 1) + " = GPIO" + String(Termostato[i].scl));
    pinMode(Termostato[i].rele, OUTPUT);
    EEPROM_Termostato_read(i);
  }
  dht.begin();
  Debug_MSG_LN("Inizializzazione il display");
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  String s;
  s = "SmartHome v";
  s += Versione;
  display.drawString(0, 0, s);
  //display.drawString(0, 45,   Termostato[i].topic);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 20, "by roncoa@gmail.com");
  display.display();
}

void ACK_Termostato() {
  String payload;
  for (int i = 0; i < Termostato_nodi; i++) {
    payload = "{\"MAC_address\":\"";
    payload += macToStr(mac);
    payload += "\",\"NODO\":\"";
    payload += Termostato_DEF;
    payload += "\",\"Topic\":\"";
    payload += Termostato[i].topic;
    payload += "\",\"Topic_ACK\":\"";
    payload += Termostato[i].topic_ACK;
    payload += "\",\"availability_topic\":\"";
    payload += WillTopicStr;
    payload += "\"}";
    client.publish(ACK_Topic, (char*) payload.c_str());
    delay(250);
    stato_TER(i);
  }
}

void HA_discovery_Termostato() {
}

void Subscribe_Termostato() {
  for (int i = 0; i < Termostato_nodi; i++) {
    client.subscribe((char*) Termostato[i].topic.c_str());
    delay(250);
    Debug_MSG_LN("MQTT subscribe " + String(  Termostato[i].topic));
  }
}

void sendTemperature() {
  String payload;
  for (int i = 0; i < Termostato_nodi; i++) {
    float old_t = Termostato[i].t;
    float old_h = Termostato[i].h;
    Termostato[i].t = dht.readTemperature(false);
    Termostato[i].h = dht.readHumidity();
    Termostato[i].hi = dht.computeHeatIndex(Termostato[i].t, Termostato[i].h, false);
    // if ((Termostato[i].t == old_t) and (Termostato[i].h == old_h)) {
    if (fabs(Termostato[i].t - old_t) <= soglia_aggiornamento_temperatura and fabs(Termostato[i].h - old_h) <= soglia_aggiornamento_temperatura) {
      Termostato[i].t = old_t;
      Termostato[i].h = old_h;
      return;
    }
    Debug_MSG((String) Termostato[i].topic);
    if (isnan(Termostato[i].h) || isnan(Termostato[i].t)) {
      Debug_MSG_LN(" Failed to read from DHT sensor!");
      return;
    }
    else
    {
      Debug_MSG_LN(" " + (String) Termostato[i].t + "°C - " + (String) Termostato[i].h + "%");
    }
    stato_TER(i);
  }
}

void stato_TER(int i) {
  String payload;
  payload = "{\"NODO\":\"";
  payload += Termostato_DEF;
  payload += "\",\"T\":\"";
  payload += Termostato[i].t;
  payload += "\",\"H\":\"";
  payload += Termostato[i].h;
  payload += "\",\"HI\":\"";
  payload += Termostato[i].hi;
  payload += "\",\"AUTO\":\"";
  if (Termostato[i].AUTO == true) payload += Termostato_ACK_AUTO;
  else payload += Termostato_ACK_MAN;
  payload += "\",\"RELE\":\"";
  if (digitalRead(Termostato[i].rele) == true ^ Flag_inversione_RELE) payload += Termostato_ACK_ON;
  if (digitalRead(Termostato[i].rele) == false ^ Flag_inversione_RELE) payload += Termostato_ACK_OFF;
  payload += "\",\"TERMOSTATO\":\"";
  payload += (String) Termostato[i].termostato;
  payload += "\"}";
  client.publish((char*) Termostato[i].topic_ACK.c_str(), (char*) payload.c_str());
  delay(100);
}

void callback_Termostato(char* topic, byte * message, unsigned int length) {
  String payload;
  String message_String;
  for (int i = 0; i < Termostato_nodi; i++) {
    if (String(topic) ==   Termostato[i].topic) {                                                                             // se arriva il comando sul topic "Termostato1_Topic"
      message_String = "";
      for (int ii = 0; ii < length; ii++) {
        message_String += (char)message[ii];
      }
      if (message_String == Termostato_READ) {       // Topic "Termostato1_Topic" = "read"
        sendTemperature();                                                                                                // sendTemperature()
      }
      if (message_String == Termostato_ON) {                                                     // Topic "Termostato1_Topic" = "on"
        digitalWrite(Termostato[i].rele, 1 ^ Flag_inversione_RELE);                                                        // Accendo il RELE
        stato_TER(i);
        EEPROM_Termostato_write(i);
      }
      if (message_String == Termostato_OFF) {                     // Topic "Termostato1_Topic" = "off"
        digitalWrite(Termostato[i].rele, 0 ^ Flag_inversione_RELE);                                                        // Spengo il RELE
        stato_TER(i);
        EEPROM_Termostato_write(i);
      }
      if (((char)message[0] == 'T' or (char)message[0] == 't') and (char)message[1] == '=' ) {                                  // Topic "Termostato1_Topic" = "T=" o "t="
        String stringtmp = "";
        for (unsigned int i = 2; i < length; i++)  {
          stringtmp = stringtmp + (char)message[i];
        }
        Debug_MSG_LN(stringtmp);
        Termostato[i].termostato = stringtmp.toFloat();
        Debug_MSG_LN((String) Termostato[i].termostato);
        EEPROM_Termostato_write(i);
        sendTemperature();
      }
      if (message_String == Termostato_STATO) {                                                  // Topic "Termostato1_Topic" = "stato"
        payload =   Termostato[i].topic;
        payload += " RSSI=";
        payload += WiFi.RSSI();
        payload += "dB ";
        payload += " T=";
        char tmp[4];                            // string buffer
        String stringtmp = "";                  //data on buff is copied to this string
        dtostrf(Termostato[i].termostato, 4, 1, tmp);
        for (int i = 0; i < sizeof(tmp); i++)  {
          stringtmp += tmp[i];
        }
        payload += stringtmp;
        if (Termostato[i].AUTO == true) payload += " AUTO";
        else payload += " MAN";
        payload += " RELE=";
        if (digitalRead(Termostato[i].rele) == true ^ Flag_inversione_RELE) payload += "ON";
        if (digitalRead(Termostato[i].rele) == false ^ Flag_inversione_RELE) payload += "OFF";
        client.publish(ACK_Topic, (char*) payload.c_str());
        delay(100);
      }
      if (message_String == Termostato_AUTO) {       // Topic "Termostato1_Topic" = "auto"
        Termostato[i].AUTO = true;
        stato_TER(i);
        EEPROM_Termostato_write(i);
      }
      if (message_String == Termostato_MAN) {                                  // Topic "Termostato1_Topic" = "man"
        Termostato[i].AUTO = false;
        stato_TER(i);
        EEPROM_Termostato_write(i);
      }
    }
  }
}

void loop_Termostato() {
  for (int i = 0; i < Termostato_nodi; i++) {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    if (digitalRead(Termostato[i].rele) == true ^ Flag_inversione_RELE) {
      display.drawString(128, 0, "ON");
    }
    if (digitalRead(Termostato[i].rele) == false ^ Flag_inversione_RELE) {
      display.drawString(128, 0, "OFF");
    }
    if (client.connected()) {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 0, String(Termostato[i].termostato));
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      if (Termostato[i].AUTO) {
        display.drawString(68, 0, "AUTO");
      }
      else {
        display.drawString(68, 0,  "MAN");
      }
    }
    else {
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 5, "No connection!");
    }
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 27, "Temp:");
    display.drawString(0, 45, String(Termostato[i].t) + " °C");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 27, "Umidità :");
    display.drawString(128, 45, String(Termostato[i].h) + " %");
    display.display();
    if (Termostato_timer[i].Wait(Termostato_TEMPO_TERMOSTATO)) {
      sendTemperature();
      if (Termostato[i].AUTO) {
        if ((Termostato[i].t < (Termostato[i].termostato - soglia_scatto_Termostato1)) and (digitalRead(Termostato[i].rele) == 0  ^ Flag_inversione_RELE)) {
          digitalWrite(Termostato[i].rele, 1 ^ Flag_inversione_RELE);                                                        // Accendo il RELE
          Debug_MSG_LN("Accendo");
          stato_TER(i);
          EEPROM_Termostato_write(i);
        }
        if ((Termostato[i].t > (Termostato[i].termostato + soglia_scatto_Termostato1)) and (digitalRead(Termostato[i].rele) == 1  ^ Flag_inversione_RELE)) {
          digitalWrite(Termostato[i].rele, 0 ^ Flag_inversione_RELE);                                                        // Spengo il RELE
          Debug_MSG_LN("Spengo");
          stato_TER(i);
          EEPROM_Termostato_write(i);
        }
      }
    }
  }
}

void EEPROM_Termostato_write(int i) {
  Debug_MSG("Writing EEPROM MAN/AUTO Termostato " + String(i + 1) + " :");
  if (Termostato[i].AUTO == true) {
    int tmp = 1;
    eepromWrite(EEPROM_Termostato_auto_rele + (i * 2), tmp);
    Debug_MSG_LN("AUTO");
  }
  if (Termostato[i].AUTO == false) {
    int tmp = 0;
    eepromWrite(EEPROM_Termostato_auto_rele + (i * 2), tmp);
    Debug_MSG_LN("MAN");
  }
  Debug_MSG("Writing EEPROM MAN/AUTO Termostato " + String(i + 1) + " :");
  if (digitalRead(Termostato[i].rele) == 1 ^ Flag_inversione_RELE) {
    int tmp = 1;
    eepromWrite(EEPROM_Termostato_auto_rele + (i * 2) + 1, tmp);
    Debug_MSG_LN("ON");
  }
  if (digitalRead(Termostato[i].rele) == 0 ^ Flag_inversione_RELE) {
    int tmp = 0;
    eepromWrite(EEPROM_Termostato_auto_rele + (i * 2) + 1, tmp);
    Debug_MSG_LN("OFF");
  }
  Debug_MSG("Writing EEPROM TERMOSTATO " + String(i + 1) + " :");
  Debug_MSG_LN((String) Termostato[i].termostato);
  eepromWriteFloat(EEPROM_Termostato_termostato + (i * 4), Termostato[i].termostato);
  EEPROM.commit();
}

void EEPROM_Termostato_read(int i) {
  int tmp;
  Debug_MSG("Reading EEPROM MAN/AUTO Termostato " + String(i + 1) + " :");
  tmp = eepromRead(EEPROM_Termostato_auto_rele + (i * 2));
  if (tmp == 1) {
    Termostato[i].AUTO = true;
    Debug_MSG_LN("AUTO");
  }
  if (tmp == 0) {
    Termostato[i].AUTO = false;
    Debug_MSG_LN("MAN");
  }
  if ((tmp != 1 and tmp != 0) or isnan(tmp)) {
    Termostato[i].AUTO = false;
    Debug_MSG_LN("ERR -> MAN");
  }
  Debug_MSG("Reading EEPROM MAN/AUTO Termostato " + String(i + 1) + " :");
  tmp = eepromRead(EEPROM_Termostato_auto_rele + (i * 2) + 1);
  if (tmp == 1) {
    digitalWrite(Termostato[i].rele, 1 ^ Flag_inversione_RELE);
    Debug_MSG_LN("ON");
  }
  if (tmp == 0) {
    digitalWrite(Termostato[i].rele, 0 ^ Flag_inversione_RELE);
    Debug_MSG_LN("OFF");
  }
  if ((tmp != 1 and tmp != 0) or isnan(tmp)) {
    digitalWrite(Termostato[i].rele, 0 ^ Flag_inversione_RELE);
    Debug_MSG_LN("ERR -> OFF");
  }
  Debug_MSG("Reading EEPROM TERMOSTATO " + String(i + 1) + " :");
  Termostato[i].termostato = eepromReadFloat(EEPROM_Termostato_termostato + (i * 4));
  if (Termostato[i].termostato<2 or Termostato[i].termostato>30 or isnan(Termostato[i].termostato)) Termostato[i].termostato = 18;
  Debug_MSG_LN((String) Termostato[i].termostato);
}

#endif
