#define BLYNK_TEMPLATE_ID "TMPL3AXg0LWup"
#define BLYNK_TEMPLATE_NAME "Water Monitoring System"
#define BLYNK_AUTH_TOKEN "Tr4CpwRpuGxexw_ZTY7a48Tjg6jJEy-z"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

//================ WiFi Credentials ===================
char ssid[] = "rasperry";
char pass[] = "12345678";

//================ Virtual Pins =======================
#define VP_MAIN_FLOW   V0
#define VP_BRANCH1     V1
#define VP_BRANCH2     V2
#define VP_TDS         V3
#define VP_LEAK        V4

//================ Sensor Pins ========================
#define FLOW1 27
#define FLOW2 26
#define FLOW3 25
#define TDS_PIN 34

//================ Variables ==========================
volatile int count1 = 0;
volatile int count2 = 0;
volatile int count3 = 0;

float flowRate1, flowRate2, flowRate3;
float tdsValue;

bool leakageDetected = false;

BlynkTimer timer;

//================ Interrupt Functions ================
void IRAM_ATTR pulse1() { count1++; }
void IRAM_ATTR pulse2() { count2++; }
void IRAM_ATTR pulse3() { count3++; }

//================ Sensor Reading Function ============
void readSensors()
{

  int pulse1Copy = count1;
  int pulse2Copy = count2;
  int pulse3Copy = count3;

  count1 = 0;
  count2 = 0;
  count3 = 0;

  // Flow rate calculation
  flowRate1 = pulse1Copy / 7.5;
  flowRate2 = pulse2Copy / 7.5;
  flowRate3 = pulse3Copy / 7.5;

  // ===== TDS SENSOR =====
  int analogValue = analogRead(TDS_PIN);
  float voltage = analogValue * (3.3 / 4095.0);

  tdsValue = (133.42 * voltage * voltage * voltage
             -255.86 * voltage * voltage
             +857.39 * voltage) * 0.5;

  // ===== SERIAL MONITOR =====
  Serial.print("Main Flow: ");
  Serial.print(flowRate1);
  Serial.print(" L/min | ");

  Serial.print("Branch1 Flow: ");
  Serial.print(flowRate2);
  Serial.print(" L/min | ");

  Serial.print("Branch2 Flow: ");
  Serial.print(flowRate3);
  Serial.print(" L/min | ");

  Serial.print("TDS: ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  // ===== SEND DATA TO BLYNK =====
  Blynk.virtualWrite(VP_MAIN_FLOW, flowRate1);
  Blynk.virtualWrite(VP_BRANCH1, flowRate2);
  Blynk.virtualWrite(VP_BRANCH2, flowRate3);
  Blynk.virtualWrite(VP_TDS, tdsValue);

  // ===== LEAKAGE DETECTION =====

  float diff1 = flowRate1 - flowRate2;
  float diff2 = flowRate2 - flowRate3;

  if(diff1 > 1.0 || diff2 > 1.0)
  {
    if(!leakageDetected)
    {
      leakageDetected = true;

      Blynk.virtualWrite(VP_LEAK, 255);

      String leakLocation = "";

      if(diff1 > 1.0)
      {
        leakLocation = "Leak between MAIN and BRANCH 1";
      }
      else if(diff2 > 1.0)
      {
        leakLocation = "Leak between BRANCH 1 and BRANCH 2";
      }
      else
      {
        leakLocation = "Leak after BRANCH 2";
      }

      Serial.print("⚠ ");
      Serial.println(leakLocation);

      Blynk.logEvent("leakage_alert", leakLocation);
    }
  }
  else
  {
    leakageDetected = false;

    Blynk.virtualWrite(VP_LEAK, 0);

    Serial.println("✅ System Normal");
  }

  // ===== TDS ALERT =====
  if(tdsValue > 500)
  {
    Blynk.logEvent("tds_alert", "⚠ High TDS Level Detected!");
  }

  Serial.println("---------------------------");
}

//================ Setup ==============================
void setup()
{

  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(FLOW1, INPUT_PULLUP);
  pinMode(FLOW2, INPUT_PULLUP);
  pinMode(FLOW3, INPUT_PULLUP);

  pinMode(TDS_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(FLOW1), pulse1, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW2), pulse2, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW3), pulse3, RISING);

  timer.setInterval(1000L, readSensors);
}

//================ Loop ===============================
void loop()
{
  Blynk.run();
  timer.run();
}
