#include <Arduino.h>
#include "Plug_Controller.h"
#include "PushButton.h"

const uint32_t LOOP_DELAY = 50;

PushButton button(12);

void setup() {
  Serial.begin(115200);

  Serial.print("Setting up button\n");
  button.DoublePressAction = []{
    Serial.print("Double Press\n");
  };
  button.PressAction = []{
    Serial.print("Press\n");
  };
  button.LongPressAction = []{
    Serial.print("Long Press\n");
  };
  // button.StateChangeAction = [](PushButton::State oldState, PushButton::State newState){
  //   Serial.printf("State Change: %d -> %d\n", oldState, newState);
  // };
}

void loop() {
  Serial.print(".");
  button.Evaluate();
  delay(LOOP_DELAY);
}