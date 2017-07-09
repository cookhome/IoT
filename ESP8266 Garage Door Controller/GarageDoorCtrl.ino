#include <ESP8266WiFi.h>

// Include API-Headers
extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#define PIN_RELAY 2
#define PIN_LIGHT 0

const char webpage[] = "HTTP/1.0 200 OK\r\nContent-Type:text/html\r\nPragma:no-cache\r\n\r\n"
"<html lang=\"zh\"><body><center><p style=\"font-size:6em\"><a href=\"?LIGHT=ON\"><button style=\"background-color:red;font-size:1em\">ON</button></a>   "
"<a href=\"?LIGHT=OFF\"><button style=\"background-color:green;font-size:1em\">OFF</button></a></p><br><form method=\"POST\">"
"<input type=\"password\" style=\"font-size:6em\" size=\"12\" name=\"PASSWORD\"><br><br><br><input type=\"submit\" style=\"background-color:blue;font-size:8em\" value=\"DOOR\"></form></center></body></html>";

bool post_found = false;
uint16_t timeRelay = 0;
  #define TIME_RELAY 5
uint16_t timeLight = 0;
  #define TIME_LIGHT 1800
os_timer_t timerCtrl;

void timerCallbackCtrl(void *pArg)
{
  if(timeRelay > 0)
  {
    timeRelay--;
    digitalWrite(PIN_RELAY, LOW); // switch on relay
  }
  else digitalWrite(PIN_RELAY, HIGH); // switch off relay

  if(timeLight > 0)
  {
    if(timeLight < 65535) timeLight--;
    digitalWrite(PIN_LIGHT, LOW); // switch on light
  }
  else digitalWrite(PIN_LIGHT, HIGH); // switch off light
}
void SdkWebServer_recv(void *arg, char *pusrdata, unsigned short length)
{
  char *found;
  
  if(!post_found)
  {
    found = os_strstr(pusrdata, "POST");
    if(found != NULL)
    {
      post_found = true;
      found += 4;
    }
    else found = pusrdata; 
  }
  else found = pusrdata; 

  if(post_found)
  {
    found = os_strstr(found, "PASSWORD=");
    if(found == NULL) return;
    found += 9;
    if(os_strstr(found, "12345678") == found)
    {
      Serial.println("PASSWORD is correct");
      timeRelay = 5;
      if(timeLight < TIME_LIGHT) timeLight = TIME_LIGHT;
    }
    else Serial.println("PASSWORD is incorrect");
  }
  else
  {
    found = os_strstr(found, "GET /?LIGHT=");
    if(found != NULL)
    {
      found += 12;
      if(os_strstr(found, "ON") == found)
      {
        Serial.println("LIGHT is ON");
        timeLight = 65535;
      }
      else if(os_strstr(found, "OFF") == found)
      {
        Serial.println("LIGHT is OFF");
        timeLight = 0;
      }
    }
  }
  
  espconn_set_opt((espconn *)arg, ESPCONN_REUSEADDR);
  espconn_sent((espconn *)arg, (uint8 *)webpage, os_strlen(webpage));
  espconn_disconnect((espconn *)arg);
}

void SdkWebServer_discon(void *arg)
{
}

void SdkWebServer_recon(void *arg, sint8 err)
{
}

void SdkWebServer_listen(void *arg)
{
  espconn_regist_recvcb((espconn *)arg, SdkWebServer_recv);
  espconn_regist_reconcb((espconn *)arg, SdkWebServer_recon);
  espconn_regist_disconcb((espconn *)arg, SdkWebServer_discon);
}

void SdkWebServer_Init(int port)
{
  LOCAL struct espconn esp_conn;
  LOCAL esp_tcp esptcp;
  esp_conn.type = ESPCONN_TCP;
  esp_conn.state = ESPCONN_NONE;
  esp_conn.proto.tcp = &esptcp;
  esp_conn.proto.tcp->local_port = port;
  esp_conn.recv_callback = NULL;
  esp_conn.sent_callback = NULL;
  esp_conn.reverse = NULL;
  espconn_regist_time(&esp_conn,0,0); //Register the connection timeout(0=no timeout)
  espconn_regist_connectcb(&esp_conn, SdkWebServer_listen); //Register connection callback
  espconn_accept(&esp_conn); //Start Listening for connections
}

void setup()
{
  Serial.begin(115200);
  Serial.print("Connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin("SSID", "Password");
  WiFi.hostname("GarageDoor");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  SdkWebServer_Init(80);
  Serial.println(WiFi.localIP());

  digitalWrite(PIN_RELAY, HIGH);
  digitalWrite(PIN_LIGHT, HIGH);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LIGHT, OUTPUT);
  
  os_timer_setfn(&timerCtrl, timerCallbackCtrl, NULL);
  os_timer_arm(&timerCtrl, 100, true);
}

void loop()
{
}


