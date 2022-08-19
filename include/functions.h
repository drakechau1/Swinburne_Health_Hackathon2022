#include "Arduino.h"
#include "SparkFun_AS7265X.h"
#include "U8g2lib.h"
#include "ESP8266WiFi.h"
#include "PubSubClient.h"


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define PIN_HALOGEN D7
#define PIN_BUTTON  D6

#define SAMPLE_TIMES 1

// blabla
void WifiReconnect();
bool InitPeripheral();
void Display_FingerRequest();
bool ButtonClickHandle();

// AS7265x
void AS7265x_Start();
void AS7265x_Stop();
int AS7265x_Read();