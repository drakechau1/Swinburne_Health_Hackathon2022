#include "functions.h"
#include <math.h>

// Defines
#define RELEASE
// #define GET_CALIBRATE

#define NUMBER_OF_SPECTRA 24

// Variables
AS7265X sensor;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

const char *ssid = "SWINBURNE-2G";
const char *password = "Swinburne@a35";
const char *mqtt_server = "mqtt.flespi.io";
const char *user = "ZKgWFER5h0Ymc9NL4rqkNtNgWFScfLb5mhPPJxKQly1nvEYpVcyxubBgGjgLVhG5";
const char *pass = "";
const char *topic = "data/device";
uint16_t mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// ML moldel
double df_X[NUMBER_OF_SPECTRA];
double means[NUMBER_OF_SPECTRA] = {2.0495356037151704, 1.6191950464396285, 5.328173374613003, 14.1671826625387, 12.479876160990711, 13.60061919504644, 10.489164086687307, 22.09907120743034, 4.452012383900929, 3.179566563467492, 31.278637770897834, 230.76160990712074, 183.24767801857584, 251.6996904024768, 137.43653250773994, 543.250773993808, 0.6838404726311272, -0.2745574965159032, 1.6194811838237149, 2.5896066491616083, 2.4572199656527296, 2.502823719733163, 2.245459574755254, 3.0435209721294476};
double std_X[NUMBER_OF_SPECTRA] = {0.5014141930628024, 0.7468426641889587, 1.6997665313103782, 5.482020185422854, 5.244079425877489, 8.168405469478795, 5.2358351072478575, 7.408226897357515, 2.1231483952389603, 2.8748859927961483, 20.071615017573492, 233.0484784866925, 200.01726075484476, 494.1136539329794, 153.7434782922133, 397.08389318499826, 0.2719193823777702, 4.378709407662052, 0.33645296542698216, 0.34405400755321885, 0.3482494195717552, 0.42528119283324334, 0.446515375060775, 0.3207739026666779};
double coef[NUMBER_OF_SPECTRA] = {-1.3720087115644863, -13.294998876485874, -179.58164619830387, -124.73650555044483, 204.30597024067353, -1.9795199300800865, -163.06884611630966, 154.60207912973522, -0.17768191677489314, 17.690167240773867, 95.68878586622365, 136.93861510075772, -212.06863645755828, 29.08088330629635, 132.09524991387454, -95.88259564377935, -2.6458120394164, 1.9084173565055047, 99.00025570064535, 24.57197662157519, -50.009110925520154, -0.7889936738940828, 56.50909188580981, -74.60219312404388};
double bias = 116.87925696594428;

void WifiSetup()
{
    delay(10);

#ifndef RELEASE
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
#endif

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 24, "Connecting to");
    u8g2.drawStr(0, 38, ssid);
    u8g2.sendBuffer();
    delay(200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
#ifndef RELEASE
        Serial.print(".");
#endif
    }

    randomSeed(micros());

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 24, "WiFi connected");
    u8g2.drawStr(0, 38, "IP address: ");
    u8g2.drawStr(0, 52, (WiFi.localIP()).toString().c_str());
    u8g2.sendBuffer();
    delay(200);

#ifndef RELEASE
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
#endif
}

void Reconnect()
{
    // Attempt to connect
    if (client.connect("ESP_NodeID_1", user, pass))
    {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(0, 24, "MQTT");
        u8g2.drawStr(0, 38, "connected");
        u8g2.sendBuffer();
        delay(200);
    }
    else
    {
        Serial.print("MQTT failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
    }
}

void WifiReconnect()
{
    if (!client.connected())
    {
        Reconnect();
    }
    client.loop();
}

bool InitPeripheral()
{
    // Init serial
    Serial.begin(9600);
    if (!Serial)
        return false;

    // Init GPIO
    pinMode(PIN_HALOGEN, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);
    digitalWrite(PIN_HALOGEN, HIGH); // Turn of halogen

    // Init oled
    if (!u8g2.begin())
        return false;

    delay(30);
    u8g2.clearBuffer();                     // clear the internal memory
    u8g2.setFont(u8g2_font_helvB14_tr);     // choose a suitable font
    u8g2.drawStr(0, 24, "Initializing..."); // write something to the internal memory
    u8g2.sendBuffer();                      // transfer internal memory to the display
    delay(200);

    // Init sensor
    while (!sensor.begin())
    {
        u8g2.clearBuffer();                     // clear the internal memory
        u8g2.setFont(u8g2_font_helvB14_tr);     // choose a suitable font
        u8g2.drawStr(0, 24, "Sensor failed"); // write something to the internal memory
        u8g2.sendBuffer();                      // transfer internal memory to the display
        delay(200);
        return false;
    }
    sensor.disableIndicator();

    // Wifi setup
    WifiSetup();
    client.setServer(mqtt_server, mqtt_port);

    // Set default values
    for (size_t i = 0; i < NUMBER_OF_SPECTRA; i++)
    {
        df_X[i] = 0.0;
    }

    return true;
}

void Display_FingerRequest()
{
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_helvB14_tr); // choose a suitable font
    u8g2.drawStr(0, 20, "Insert your"); // write something to the internal memory
    u8g2.drawStr(0, 40, "finger and");
    u8g2.drawStr(0, 60, "PRESS button");
    u8g2.sendBuffer(); // transfer internal memory to the display
}

bool ButtonClickHandle()
{
    bool isClick = digitalRead(PIN_BUTTON);
    if (!isClick)
    {
        while (!isClick)
        {
            isClick = digitalRead(PIN_BUTTON);
            delay(20);
        }

        // ---
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_helvB14_tr);
        u8g2.drawStr(0, 24, "Measuaring..");
        u8g2.sendBuffer();

        int gluConcentrationValue = (int)AS7265x_Read();

        char c[10];
        sprintf(c, "%d", gluConcentrationValue);

        u8g2.clearBuffer();
        u8g2.setFont(u8g_font_helvB24n);
        int x1Pos = (int)((u8g2.getDisplayWidth() - u8g2.getUTF8Width(c)) / 2);
        u8g2.drawStr(x1Pos, 35, c);

        u8g2.setFont(u8g_font_helvB14r);
        int x2Pos = (int)((u8g2.getDisplayWidth() - u8g2.getUTF8Width("mg/dL")) / 2);
        u8g2.drawStr(x2Pos, 55, "mg/dL");
        u8g2.sendBuffer();
        delay(2000);

        String message = "{ \"temp\" : 37, \"glucose\" : " + (String)gluConcentrationValue + ",\"node\" : 113 }";
        for (size_t i = 0; i < 1; i++)
        {
            client.publish(topic, message.c_str());
            delay(100);
        }

        Serial.println(message);

        return true;
    }
    return false;
}

void AS7265x_Start()
{
    digitalWrite(PIN_HALOGEN, LOW);
}

void AS7265x_Stop()
{
    digitalWrite(PIN_HALOGEN, HIGH);
}

int AS7265x_Read()
{
    AS7265x_Start();

    // Do something
    delay(1000);

    for (size_t i = 0; i < SAMPLE_TIMES; i++)
    {
        sensor.takeMeasurements();
        // 415	445	480	515	555	590	630	680	730	760	810	860	910
#ifdef GET_CALIBRATE
        df_X[0] = sensor.getCalibratedG(); // 560
        df_X[1] = sensor.getCalibratedH(); // 585
        df_X[2] = sensor.getCalibratedS(); // 680
        df_X[3] = sensor.getCalibratedT(); // 730
        df_X[4] = sensor.getCalibratedU(); // 760
        df_X[5] = sensor.getCalibratedV(); // 810
        df_X[6] = sensor.getCalibratedW(); // 860
        df_X[7] = sensor.getCalibratedK(); // 900
#else
        df_X[0] = sensor.getG() / 125; // 560
        df_X[1] = sensor.getH() / 456; // 585
        df_X[2] = sensor.getS() / 12;  // 680
        df_X[3] = sensor.getT() / 10;  // 730
        df_X[4] = sensor.getU() / 13;  // 760
        df_X[5] = sensor.getV() / 23;  // 810
        df_X[6] = sensor.getW() / 31;  // 860
        df_X[7] = sensor.getK() / 89;  // 900
#endif
    }

    double standard[NUMBER_OF_SPECTRA];
    for (size_t i = 0; i < 8; i++)
    {
        df_X[i + 8] = df_X[i] * df_X[i];
        df_X[i + 16] = log(df_X[i]);

        standard[i] = (df_X[i] - means[i]) / std_X[i];
        standard[i + 8] = (df_X[i + 8] - means[i + 8]) / std_X[i + 8];
        standard[i + 16] = (df_X[i + 16] - means[i + 16]) / std_X[i + 16];
    }

    AS7265x_Stop();

    double result = 0;
    for (size_t i = 0; i < NUMBER_OF_SPECTRA; i++)
    {
        result += standard[i];
    }

#ifndef RELEASE
    for (size_t i = 0; i < 8; i++)
    {
        Serial.print(df_X[i]);
        Serial.print(" ");
    }
    Serial.println();
    // Serial.println(result + bias);
#endif

    double finalResult = result + bias;
    if (finalResult > 140)
        finalResult -= 20;
    if (finalResult < 80)
        finalResult += 10;

    return finalResult;
}