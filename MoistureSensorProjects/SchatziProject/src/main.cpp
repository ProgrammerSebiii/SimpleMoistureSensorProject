#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

#define MOISTURE_PIN A1
#define LED_PIN 6
#define PLUS_BUTTON 8
#define MINUS_BUTTON 3
#define CALIBRATE_BUTTON 5

#define OLED_RESET 4

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int normalTextSize = 2;
int setTargetTextSize = 3;
int plusMinusSize = 3;
// Calculations for cursor positions
int begin_first_line_left_x = 10;
int end_first_line_left_x = 128 - 4 * normalTextSize * 6;
int first_line_y = 0;

int begin_second_line_left_x = 10;
int end_second_line_left_x = 128 - 4 * normalTextSize * 6;
int second_line_y = 18;

int AirValue = 456;   // you need to replace this value with analog reading when the sensor is in dry air
int WaterValue = 145; // you need to replace this value with analog reading when the sensor is in water
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

int threshold = 50;

void displayPlusMinus();
void displayFirstLine(int moisture);
void displaySecondLine();
void doCalibrateProcedure();
void doNewTargetProcedure();
#define SCREEN_ADDRESS 0x3C
int get_moisture_value();

unsigned long displayOnTime;
bool displayOn = false;
unsigned long displayTimeout = 5000;
bool displayOnAgain = false;
void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(PLUS_BUTTON, INPUT);
  pinMode(MINUS_BUTTON, INPUT);
  pinMode(CALIBRATE_BUTTON, INPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  display.setTextColor(WHITE);
  delay(1000); // Pause for 2 seconds
  display.clearDisplay();

  display.println("SCHATZ");

  displayOnTime = millis();
  displayOn = true;
}

void loop()
{
  Serial.print("Display status: ");
  Serial.println(displayOn);

  if (digitalRead(CALIBRATE_BUTTON) == HIGH)
  {
    Serial.println("Starting calibration");
    doCalibrateProcedure();
  }

  int moisture = get_moisture_value();
  Serial.println(moisture);
  if (moisture < threshold)
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
  }

  if (displayOn)
  {
    Serial.println("Updating display");
    displayFirstLine(moisture);
    displaySecondLine();

    if (displayOnAgain)
    {
      delay(500);
      displayOnAgain = false;
    }

    display.display();

    if (digitalRead(PLUS_BUTTON) == HIGH || digitalRead(MINUS_BUTTON) == HIGH)
    {
      doNewTargetProcedure();
      displayOnTime = millis();
    }
    delay(250);
    display.clearDisplay();
  }

  if (millis() - displayOnTime > displayTimeout)
  {
    display.clearDisplay();
    displayOn = false;
    Serial.println("Clearing display");
    display.clearDisplay();
    display.print("");
    display.display();
  }

  if (digitalRead(PLUS_BUTTON) == HIGH || digitalRead(MINUS_BUTTON) == HIGH)
  {
    if (!displayOn)
    {
      displayOnAgain = true;
    }
    displayOn = true;
    displayOnTime = millis();
  }
}

int getStartForString(int length, int textSize)
{
  int pixel = length * 6 * textSize;
  return 64 - pixel / 2;
}

void displayText(int startY, int textSize, String toDisplay)
{
  int strLength = toDisplay.length();
  int startX = getStartForString(strLength, textSize);
  display.setTextColor(WHITE);
  display.setTextSize(textSize);
  display.setCursor(startX, startY);
  display.print(toDisplay.c_str());
}

void displayFirstLine(int moisture)
{

  String toDisplay = String(moisture) + String("%");
  displayText(first_line_y, normalTextSize, toDisplay);
}

void displaySecondLine()
{
  String toDisplay = String("-> ") + String(threshold) + String("%");
  displayText(second_line_y, normalTextSize, toDisplay);
}

int get_moisture_value()
{
  soilMoistureValue = analogRead(MOISTURE_PIN); // put Sensor insert into soil
  Serial.println(soilMoistureValue);
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  int moisture = 50;

  if (soilmoisturepercent >= 100)
  {
    moisture = 100;
  }
  else if (soilmoisturepercent <= 0)
  {
    moisture = 0;
  }
  else
  {
    moisture = soilmoisturepercent;
  }
  return moisture;
}

bool waitForInput()
{
  while (true)
  {
    if (digitalRead(PLUS_BUTTON) == HIGH)
    {
      display.clearDisplay();
      return true;
    }
    else if (digitalRead(MINUS_BUTTON) == HIGH)
    {
      display.clearDisplay();
      return false;
    }
  }
}

void displayPleaseWait()
{
  display.clearDisplay();
  String waitText1 = "Bitte";
  String waitText2 = "warten...";
  displayText(first_line_y, normalTextSize, waitText1);
  displayText(second_line_y, normalTextSize, waitText2);
}

void doCalibrateProcedure()
{

  // Show Kalibrierung für eine Zeit lang
  display.clearDisplay();
  String waitText1 = "Kalibrierung";
  String waitText2 = "lol";
  displayText(first_line_y, 1, waitText1);
  displayText(second_line_y, 2, waitText2);
  display.display();
  delay(1000);
  display.clearDisplay();

  String displayCalib = "Sensor in Wasser";
  displayText(first_line_y, 1, displayCalib);
  displayCalib = "L Abbr. | R Weiter";
  displayText(second_line_y, 1, displayCalib);
  display.display();

  if (!waitForInput())
  {
    return;
  }

  displayPleaseWait();
  display.display();
  long sum = 0;
  for (int i = 0; i < 100; i++)
  {
    sum += get_moisture_value();
    delay(20);
  }

  delay(1000);
  display.clearDisplay();

  int tempWaterValue = sum / 100;
  Serial.print("New water value: ");
  Serial.println(tempWaterValue);

  displayCalib = "Sensor an die Luft";
  displayText(first_line_y, 1, displayCalib);
  displayCalib = "L Abbr. | R Weiter";
  displayText(second_line_y, 1, displayCalib);
  display.display();
  if (!waitForInput())
  {
    return;
  }
  displayPleaseWait();
  display.display();
  sum = 0;
  for (int i = 0; i < 100; i++)
  {
    sum += get_moisture_value();
    delay(20);
  }
  delay(1000);
  display.clearDisplay();

  int tempAirValue = sum / 100;

  Serial.print("New air value: ");
  Serial.println(tempAirValue);

  WaterValue = tempWaterValue;
  AirValue = tempAirValue;
}

void doNewTargetProcedure()
{
  unsigned long timeSinceLastChange = millis();
  unsigned long start = millis();
  while (millis() - start < 5000)
  {
    display.clearDisplay();
    String newTargetString = String("New:") + String(threshold) + String("% ");

    if (millis() - timeSinceLastChange > 500)
    {
      String remainingTimeString = String(int((5000 - (millis() - start)) / 1000)) + String("s left");
      displayText(second_line_y, normalTextSize, remainingTimeString);
    }

    displayText(first_line_y, normalTextSize, newTargetString.c_str());

    display.display();
    delay(50);
    if (digitalRead(PLUS_BUTTON) == HIGH)
    {
      timeSinceLastChange = millis();
      threshold += 1;
      if (threshold > 100)
      {
        threshold = 100;
      }

      start = millis();
    }
    else if (digitalRead(MINUS_BUTTON) == HIGH)
    {
      timeSinceLastChange = millis();
      threshold -= 1;
      if (threshold < 0)
      {
        threshold = 0;
      }
      start = millis();
    }
  }
  display.clearDisplay();
}

void displayPlusMinus()
{
  /*
  display.setTextSize(plusMinusSize);
  display.setTextColor(WHITE);

  String targetString = String("Ziel neu:");

  char *targetStringPrimitive;
  targetString.toCharArray(targetStringPrimitive, targetString.length());
  Serial.print("New target: ");
  Serial.println(targetStringPrimitive);

  int y = 32 - setTargetTextSize * 6;
  int startX = 30;
  display.setCursor(startX, y);

  display.println(targetStringPrimitive);
  */
}