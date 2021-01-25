#include "Plug_Controller.h"
#include "PushButton.h"
#include "Lamp.h"
#include "Memory.h"
#include <Arduino.h>
#include <WiFi.h>

#if __has_include("myconfig.h")
  #include "myconfig.h"
#else
  #warning "Using Defaults: Copy myconfig.sample.h to myconfig.h and edit that to use your own settings"
  #include "myconfig.sample.h"
#endif

const uint32_t LOOP_DELAY = 50;

PushButton button(GPIO_NUM_12);
Lamp lamp(13);

enum DIMMER_STATE {
  COUNT_UP,
  COUNT_DOWN
} direction;

bool useSmartplug = false;
IPAddress smartPlugIp = { 192, 168, 2, 126 };
PlugController smartPlug(smartPlugIp, 9999);

void setSmartPlug(Lamp::STATE state)
{
  if(WiFi.status() == WL_CONNECTED)
  {
    if(state == Lamp::STATE::OFF)
    {
      Serial.printf("Turn off smartplug\n");
      smartPlug.off();
    }
    else
    {
      Serial.printf("Turn on smartplug\n");
      smartPlug.on();
    }
  }  
}

uint8_t wlanConnectAttempts = 0;

void setup() {
  Serial.begin(115200);

  button.DoublePressAction = []{
    useSmartplug = !useSmartplug;
    if(useSmartplug == false && WiFi.status() == WL_CONNECTED)
    {
      setSmartPlug(Lamp::STATE::OFF);
      WiFi.disconnect();
    }
    else if(WiFi.status() == WL_CONNECTED)
    {
      setSmartPlug(lamp.GetLampState());
    }
  };
  lamp.Print();
  
  button.PressAction = []{
    if(useSmartplug)
    {
      setSmartPlug(lamp.GetLampState() ? Lamp::STATE::OFF : Lamp::STATE::ON);
    }
    lamp.Toggle();
  };
  button.LongPressAction = []{
    if(direction == COUNT_UP)
    {
      lamp.Brighter();
    }
    else
    {
      lamp.Darker();
    }
  };
  button.LongPressFinishAction = []{
    direction = direction == COUNT_UP ? COUNT_DOWN : COUNT_UP;
  };
}

void loop() {
  button.Evaluate();
  if(lamp.GetLampState() == Lamp::STATE::OFF)
  {
    button.SleepUntilButtonPressed();
    if(WiFi.status() == WL_CONNECTED)
    {
      WiFi.disconnect();
    }    
  }
  if(useSmartplug)
  {
    if(WiFi.status() != WL_CONNECTED)
    {
      if(wlanConnectAttempts == 0)
      {
        Serial.printf("WLAN begin\n");
        WiFi.begin(WLAN_SSID, WLAN_PW);
      }
      else if(wlanConnectAttempts % 20 == 0)
      {
        Serial.printf("WLAN reconnect\n");
        WiFi.reconnect();
      }
      wlanConnectAttempts++;
      Serial.printf("WLAN connect attempts: %i\n", wlanConnectAttempts);
    }
    else
    {
      if(wlanConnectAttempts > 0)
      {
        Serial.printf("WLAN connected\n");
        setSmartPlug(lamp.GetLampState());
        wlanConnectAttempts = 0;
      }
    }
  }
  delay(LOOP_DELAY);
}