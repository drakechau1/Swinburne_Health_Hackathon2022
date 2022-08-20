#include "functions.h"

void setup()
{
  InitPeripheral();
}

void loop()
{
  WifiReconnect();
  Display_FingerRequest();
  ButtonClickHandle();
}