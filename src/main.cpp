#include <Arduino.h>
#include "Plug_Controller.h"
#include "PushButton.h"
#include "Lamp.h"
#include "Memory.h"

const uint32_t LOOP_DELAY = 50;

PushButton button(GPIO_NUM_12);
Lamp lamp(13);

enum DIMMER_STATE {
  COUNT_UP,
  COUNT_DOWN
} direction;

void setup() {
  Serial.begin(115200);

  Serial.print("Setting up button\n");
  button.DoublePressAction = []{
    Serial.print("Double Press\n");
  };
  lamp.Print();
  
  button.PressAction = []{
    lamp.Toggle();
    Serial.printf("Toggeling\n");
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
    // Memory::Instance().SetBrightness(lamp.GetBrightness());
  };
  button.StateChangeAction = [](PushButton::State oldState, PushButton::State newState){
    Serial.printf("State Change: %d -> %d\n", oldState, newState);
  };  
}

void loop() {
  button.Evaluate();
  if(lamp.GetLampState() == Lamp::STATE::OFF)
  {
    button.SleepUntilButtonPressed();
  }
  delay(LOOP_DELAY);
}