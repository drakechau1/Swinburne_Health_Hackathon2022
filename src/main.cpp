#include "functions.h"

void setup() {

  if (InitPeripheral())
    Display_FingerRequest();
}

void loop() {
  WifiReconnect();

  if (ButtonClickHandle())
  {
    Display_FingerRequest();
  }
}