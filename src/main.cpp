#include "Plug_Controller.h"
#include "PushButton.h"
#include "Lamp.h"
#include "Memory.h"
#include <Arduino.h>
#include <WiFi.h>
#include "OtaUpdate.h"

#if __has_include("myconfig.h")
  #include "myconfig.h"
#else
  #warning "Using Defaults: Copy myconfig.sample.h to myconfig.h and edit that to use your own settings"
  #include "myconfig.sample.h"
#endif

const uint32_t LOOP_DELAY = 50;

PushButton button(GPIO_NUM_12);
Lamp lamp(13);

enum ModeState {
  OPERATION,
  PREPARE_DOWN,
  PREPARE_UP,
  DISCO_MODE,
  CANDLE_MODE
} mode;
bool strobo = true;
int modeCounter;

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
    try
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
    catch(const std::exception& ex)
    {
        Serial.printf("Exception caught while trying to switch smart plug\n");
    }
  }  
}

uint8_t wlanConnectAttempts = 0;

void setup() {
  Serial.begin(115200);
  modeCounter = 0;
  button.DoublePressAction = []{
    if(mode != PREPARE_DOWN && mode != PREPARE_UP)
    {
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
    }
    else
    {
      mode = mode == PREPARE_DOWN ? CANDLE_MODE : DISCO_MODE;
      mode == CANDLE_MODE ? Serial.printf("Time for some romance ;)\n") : Serial.printf("Welcome to the disco\n");
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
    Serial.printf("Mode Counter: %i ", modeCounter);

    if(direction == COUNT_UP)
    {
      lamp.Brighter();
    }
    else
    {
      lamp.Darker();
    }
    modeCounter++;
  };
  button.LongPressFinishAction = []{
    direction = direction == COUNT_UP ? COUNT_DOWN : COUNT_UP;
    if(modeCounter > 10000 / LOOP_DELAY)
    {
      mode = direction == COUNT_UP ? PREPARE_DOWN : PREPARE_UP;
      Serial.printf("Switched to mode %i\n", mode);
    }
    modeCounter = 0;
  };
}

void loop() {
  button.Evaluate();
  if(lamp.GetLampState() == Lamp::STATE::OFF)
  {
    mode = OPERATION;
    button.SleepUntilButtonPressed();
    if(WiFi.status() == WL_CONNECTED)
    {
      WiFi.disconnect();
    }    
  }
  
  if((mode == DISCO_MODE || mode == CANDLE_MODE))
  {
    if(mode == DISCO_MODE)
    {
      strobo ? lamp.SetBrightness(255, false, true) : lamp.SetBrightness(1, false, true);
      strobo = !strobo;
    }
    else if(mode == CANDLE_MODE)
    {
      uint32_t val = rand() % 100 + 155;
      lamp.SetBrightness(val, true);
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
        checkOta();
      }
    }
  }
  delay(LOOP_DELAY);
}