#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <EEPROM.h>
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
int first_line_small_y = 5;

int begin_second_line_left_x = 10;
int end_second_line_left_x = 128 - 4 * normalTextSize * 6;
int second_line_y = 18;
int second_line_small_y = 24;

#define WATER_VALUE_ADDRESS 69
#define AIR_VALUE_ADDRESS 96
int AirValue = 456;   // you need to replace this value with analog reading when the sensor is in dry air
int WaterValue = 145; // you need to replace this value with analog reading when the sensor is in water
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
#define CALIBRATION_TIMEOUT 20000L
#define EASTER_EGG_TIMEOUT 2000

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
void displayText(int startY, int textSize, String toDisplay);

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

  WaterValue = EEPROM.read(WATER_VALUE_ADDRESS);
  AirValue = EEPROM.read(AIR_VALUE_ADDRESS);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 1 seconds
  display.clearDisplay();

  Serial.println("Displaying intro text");
  String line1 = "For my precious";
  String line2 = "Love you";
  displayText(first_line_small_y, 1, line1);
  displayText(second_line_small_y, 1, line2);
  display.display();
  delay(2000);
  display.clearDisplay();

  displayOnTime = millis();
  displayOn = true;
}

void loop()
{
  // Serial.print("Display status: ");
  // Serial.println(displayOn ? "ON" : "OFF");

  if (digitalRead(CALIBRATE_BUTTON) == HIGH)
  {
    Serial.println("Starting calibration");
    doCalibrateProcedure();
    displayOnTime = millis();
    displayOn = true;
  }

  int moisture = get_moisture_value();
  Serial.print("Moisture: ");
  Serial.print(moisture);
  Serial.println("%");
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

void showEasterEgg(String defaultLine1, String defaultLine2)
{
  display.clearDisplay();
  String line1_egg = "Fuer Spatz <3";
  String line2_egg = "Von Hase - I LOVE YOU";
  displayText(first_line_small_y, 1, line1_egg);
  displayText(second_line_small_y, 1, line2_egg);
  display.display();
  delay(EASTER_EGG_TIMEOUT);
  display.clearDisplay();
  displayText(first_line_small_y, 1, defaultLine1);
  displayText(second_line_small_y, 1, defaultLine2);
  display.display();
}

bool checkForEasterEgg(String line1, String line2, bool plus_pressed)
{
  int button_high = PLUS_BUTTON;
  int button_missing = MINUS_BUTTON;
  if (!plus_pressed)
  {
    button_high = MINUS_BUTTON;
    button_missing = PLUS_BUTTON;
  }

  delay(100);
  Serial.print("Button high: ");
  Serial.print(digitalRead(button_high));
  Serial.print(" missing button for easter egg: ");
  Serial.println(button_missing);
  while (digitalRead(button_high) == HIGH)
  {
    if (digitalRead(button_missing) == HIGH)
    {
      Serial.println("Easter triggered");
      showEasterEgg(line1, line2);

      return true;
      break;
    }
  }
  return false;
}

bool waitForInput(String line1, String line2)
{
  display.clearDisplay();
  displayText(first_line_small_y, 1, line1);
  displayText(second_line_small_y, 1, line2);
  display.display();
  delay(200);

  unsigned long start = millis();
  /*
  Serial.print("Abort?");
  unsigned long compare = millis() - start;
  Serial.print(compare);
  Serial.print("<");
  Serial.print(20000uL);
  Serial.print("? ");
  Serial.println(compare < 20000uL);
  */
  while (millis() - start < CALIBRATION_TIMEOUT)
  {
    if (digitalRead(PLUS_BUTTON) == HIGH)
    {

      // Easter egg
      bool easterEggShown = checkForEasterEgg(line1, line2, true);

      if (easterEggShown)
      {
        start = millis();
      }
      else
      {
        Serial.println("Pressed next (plus button)");
        display.clearDisplay();
        return true;
      }
    }
    else if (digitalRead(MINUS_BUTTON) == HIGH)
    {
      // Easter egg
      bool easterEggShown = checkForEasterEgg(line1, line2, false);

      if (easterEggShown)
      {
        start = millis();
      }
      else
      {
        Serial.println("Pressed abort (minus button)");
        display.clearDisplay();
        return false;
      }
    }
  }
  Serial.println("Calibration timeout reached");
  return false;
}

void displayPleaseWait()
{
  display.clearDisplay();
  String waitText1 = "Bitte";
  String waitText2 = "warten...";
  displayText(first_line_y, normalTextSize, waitText1);
  displayText(second_line_y, normalTextSize, waitText2);
  delay(1000);
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

  String line1 = "Sensor in Wasser";
  String line2 = "L Abbr. | R Weiter";

  if (!waitForInput(line1, line2))
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

  display.clearDisplay();

  int tempWaterValue = sum / 100;
  Serial.print("New water value: ");
  Serial.println(tempWaterValue);

  line1 = "Sensor an die Luft";
  line2 = "L Abbr. | R Weiter";
  if (!waitForInput(line1, line2))
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

  display.clearDisplay();

  int tempAirValue = sum / 100;

  Serial.print("New air value: ");
  Serial.println(tempAirValue);

  EEPROM.write(WATER_VALUE_ADDRESS, tempWaterValue);
  EEPROM.write(AIR_VALUE_ADDRESS, tempAirValue);
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