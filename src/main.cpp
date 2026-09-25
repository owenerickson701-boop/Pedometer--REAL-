#include <Arduino.h>
#include <Wire.h>
#include "SparkFun_BNO08x_Arduino_Library.h" // I used the wrong library for this??
#include <SPI.h>
#include <Adafruit_ST7789.h>
#include <cmath>

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);

BNO08x myIMU;

#define BNO08X_ADDR 0x4A

void setReports(void);

long debounceTime = 80;
volatile long prev1 = 0;
volatile long prev2 = 0;
volatile long prev0 = 0;
volatile bool b1flag = false;
volatile bool b2flag = false;
volatile bool b0flag = false;

float x = 0;
float y = 0;
float z = 0;
float a = 0;

int64_t stepNumber = 0; // current number of steps
byte stepFlag = 0;

float distance = 0;     // total distance traveled in feet
float stepDistance = 1; // how long a step is in feet

float stepHi = 2;  // To start a step
float stepLo = .5; // To end a step

enum menuState
{
  steps,
  stepSize,
  distanceTraveled,
  raw,
  mCount
};

menuState currentMenu = raw;

void IRAM_ATTR button1()
{
  long now = millis();
  if (now > prev1 + debounceTime)
  {
    b1flag = true;
    prev1 = now;
  }
}

void IRAM_ATTR button2()
{
  long now = millis();
  if (now > prev2 + debounceTime)
  {
    b2flag = true;
    prev2 = now;
  }
}

void IRAM_ATTR button0()
{
  long now = millis();
  if (now > prev0 + debounceTime)
  {
    b0flag = true;
    prev0 = now;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Wire.begin();

  if (!myIMU.begin(BNO08X_ADDR, Wire, -1, -1))
  {
    Serial.println("BNO085 not detected!");
    while (1)
      delay(100);
  }

  setReports();

  display.init(135, 240);
  display.setRotation(3);
  canvas.setTextColor(ST77XX_CYAN);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 1);

  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), button1, RISING);

  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), button2, RISING);

  pinMode(0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), button0, RISING);
}

void setReports(void)
{
  Serial.println("Setting desired reports");
  if (myIMU.enableLinearAccelerometer() == true)
  {
    Serial.println(F("Linear Accelerometer enabled"));
    Serial.println(F("Output in form x, y, z, in m/s^2"));
  }
  else
  {
    Serial.println("Could not enable linear accelerometer");
  }
}

float lastUpdate = 0;
float now = 0;

void loop()
{
  if (myIMU.getSensorEvent())
  {
    if (myIMU.getSensorEventID() == SENSOR_REPORTID_LINEAR_ACCELERATION)
    {
      x = myIMU.getLinAccelX();
      y = myIMU.getLinAccelY();
      z = myIMU.getLinAccelZ();

      a = sqrt(x * x + y * y + z * z);

      lastUpdate = millis();
    }
    else
    {
      Serial.println("Wrong event ID");
    }
  }

  if (stepFlag) // step is begun
  {
    if (a < stepLo) // if low accel
    {
      stepFlag = 0;    // flag off
      stepNumber += 1; // counter increment
      distance += stepDistance;
    }
  }
  else if (a > stepHi) // if not currently stepping and if accel hi
  {
    stepFlag = 1; // start step
  }

  // RULES: Button 1 switch menu
  // Button 0 up length, button 2 down length
  // Button 0 up units, button 2 down units
  // If on steps and press 0 AND 2, reset
  // All buttons should reset their own flags.

  if (b2flag & b0flag) // if BOTH pressed, reset counter!
  {
    stepNumber = 0;
    distance = 0;
    b0flag = false;
    b2flag = false;
  }

  else if (b1flag)
  {
    b1flag = false;
    currentMenu = (menuState)(((int)currentMenu + 1) % (int)menuState::mCount);
  }

  else if (b2flag)
  {
    b2flag = false;

    if (currentMenu == menuState::stepSize)
    {
      stepDistance += (1 / 12); // longer by 1 inch
    }
    else if (currentMenu == menuState::distanceTraveled)
    { // units
    }
  }

  else if (b0flag)
  {
    b0flag = false;
    if (currentMenu == menuState::stepSize)
    {
      stepDistance += (-1 / 12); // shorter by 1 inch
    }
  }

  canvas.setCursor(2, 20);
  canvas.setTextSize(2);

  if (currentMenu == menuState::steps)
  {
    canvas.fillScreen(ST77XX_RED);
    canvas.setTextColor(ST77XX_BLACK);
    canvas.println("Current Steps: ");
    canvas.println(stepNumber);
  }
  else if (currentMenu == menuState::stepSize)
  {
    canvas.fillScreen(ST77XX_BLUE);
    canvas.setTextColor(ST77XX_BLACK);
    canvas.print("Step Length: ");
    canvas.print(stepDistance);
  }
  else if (currentMenu == menuState::distanceTraveled)
  {
    canvas.fillScreen(ST77XX_ORANGE);
    canvas.setTextColor(ST77XX_WHITE);
    canvas.println("Distance Traveled: ");
    canvas.print(distance);
    canvas.print(" ft");
  }
  else if (currentMenu == menuState::raw)
  {
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setTextColor(ST77XX_GREEN);
    canvas.print("X: ");
    canvas.println(x);
    canvas.print("Y: ");
    canvas.println(y);
    canvas.print("Z: ");
    canvas.println(z);
    canvas.println(a);

    canvas.println(stepFlag);
  }

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
}