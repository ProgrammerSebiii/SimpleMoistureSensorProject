#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
#define LATER_REMINDER_VALUE_ADDRESS 120
#define TIME_SINCE_UNDER_THRESHOLD 140
#define DAYS_THRESHOLD_BEFORE_LIGHT_UP 10
int AirValue = 456;   // you need to replace this value with analog reading when the sensor is in dry air
int WaterValue = 145; // you need to replace this value with analog reading when the sensor is in water
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
bool LightUpAfter1Week = false;
int timeSinceUnderThreshold = 0;
#define CALIBRATION_TIMEOUT 20000L
#define EASTER_EGG_TIMEOUT 2000

int threshold = 50;

void displayPlusMinus();
void displayFirstLine(int moisture);
void displaySecondLine();
void doCalibrateProcedure();
void doNewTargetProcedure();
void doNewTargetLightUpProcedure();

int freeRam()
{
  // https://forum.arduino.cc/index.php?topic=48577.0#:~:text=If%20you're%20arduino%20resets,the%20array%20or%20buffer%20size.
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// #define hoursBeforeLightUp 168     // 168 = 168 hours = 1 week
int daysBeforeLightUp = 0;

#define SCREEN_ADDRESS 0x3C
int get_moisture_value(bool asPercent);

unsigned long displayOnTime;
bool displayOn = false;
unsigned long displayTimeout = 5000;
bool displayOnAgain = false;
unsigned long lastTimeUpdateForThresholdCheck = 0;
void displayText(int startY, int textSize, char *toDisplay);

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(PLUS_BUTTON, INPUT);
  pinMode(MINUS_BUTTON, INPUT);
  pinMode(CALIBRATE_BUTTON, INPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  Serial.print("Free ram: ");
  Serial.println(freeRam());
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  WaterValue = EEPROM.read(WATER_VALUE_ADDRESS);
  AirValue = EEPROM.read(AIR_VALUE_ADDRESS);
  LightUpAfter1Week = EEPROM.read(LATER_REMINDER_VALUE_ADDRESS);
  timeSinceUnderThreshold = EEPROM.read(TIME_SINCE_UNDER_THRESHOLD);
  daysBeforeLightUp = EEPROM.read(DAYS_THRESHOLD_BEFORE_LIGHT_UP);
  Serial.print("Light up after 1 week: ");
  Serial.println(LightUpAfter1Week);
  Serial.print("Time since under threshold: ");
  Serial.println(timeSinceUnderThreshold);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 1 seconds
  display.clearDisplay();

  Serial.println("Displaying intro text");
  char *line1 = "For my precious";
  char *line2 = "Love you";
  displayText(first_line_small_y, 1, line1);
  displayText(second_line_small_y, 1, line2);
  display.display();
  delay(2000);
  display.clearDisplay();

  displayOnTime = millis();
  displayOn = true;
  lastTimeUpdateForThresholdCheck = millis();

  Serial.print("Free ram: ");
  Serial.println(freeRam());
}

#define timeBetweenUpdates 86400000L // 86400000ms = 1 day

void loop()
{
  Serial.print("Free ram: ");
  Serial.println(freeRam());
  unsigned long current = millis();
  int moisture = get_moisture_value(true);

  if (abs(current - lastTimeUpdateForThresholdCheck) > 100000L)
  {
    Serial.println("Time between updates reached");
    if (LightUpAfter1Week)
    {
      // if (moisture < threshold)
      // {
      //   timeSinceUnderThreshold += 1;
      // }
      // else
      // {
      //   timeSinceUnderThreshold = 0;
      // }

      // Serial.print("Update time since under threshold:");
      // Serial.println(timeSinceUnderThreshold);

      // lastTimeUpdateForThresholdCheck = millis();
    }
    // if (LightUpAfter1Week)
    // {
    //   if (moisture < threshold)
    //   {
    //     timeSinceUnderThreshold += 1;
    //   }
    //   else
    //   {
    //     timeSinceUnderThreshold = 0;
    //   }

    //   Serial.print("Update time since under threshold:");
    //   Serial.println(timeSinceUnderThreshold);

    //   lastTimeUpdateForThresholdCheck = millis();
    // }
    // else
    // {
    //   Serial.println("Rest time since under threshold");
    //   timeSinceUnderThreshold = 0;
    //   lastTimeUpdateForThresholdCheck = millis();
    // }
    // EEPROM.write(TIME_SINCE_UNDER_THRESHOLD, timeSinceUnderThreshold);
    // delay(1000);
  }

  if (digitalRead(CALIBRATE_BUTTON) == HIGH)
  {
    Serial.println("Starting calibration");
    doCalibrateProcedure();
    displayOnTime = millis();
    displayOn = true;
  }

  Serial.print("Moisture: ");
  Serial.print(moisture);
  Serial.println("%");

  if (moisture < threshold)
  {
    bool lightUpBecauseFlagAndTimeSpan = LightUpAfter1Week && timeSinceUnderThreshold > daysBeforeLightUp;
    if (lightUpBecauseFlagAndTimeSpan)
    {
      digitalWrite(LED_PIN, HIGH);
    }
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

void displayText(int startY, int textSize, char *toDisplay)
{
  int strLength = strlen(toDisplay);
  int startX = getStartForString(strLength, textSize);
  Serial.print("Free ram: ");
  Serial.println(freeRam());
  display.setTextColor(WHITE);
  display.setTextSize(textSize);
  Serial.print("Free ram: ");
  Serial.println(freeRam());
  display.setCursor(startX, startY);
  Serial.print("To display:");
  Serial.println(toDisplay);
  Serial.print("Length: ");
  Serial.println(strLength);
  display.print(toDisplay);
}

void displayFirstLine(int moisture)
{
  //+ String(" ") + String(timeSinceUnderThreshold) + String("/") + String(daysBeforeLightUp)
  char buffer[30];
  itoa(moisture, buffer, 10);
  strcat(buffer, "%");
  Serial.println(buffer);
  displayText(first_line_y, normalTextSize, buffer);
}

void displaySecondLine()
{
  char buffer[10];
  strcpy(buffer, "-> ");
  char thresholdBuffer[10];
  itoa(threshold, thresholdBuffer, 10);
  strcat(buffer, thresholdBuffer);
  strcat(buffer, "%");
  displayText(second_line_y, normalTextSize, buffer);
}

int get_moisture_value(bool asPercent)
{
  soilMoistureValue = analogRead(MOISTURE_PIN); // put Sensor insert into soil
  Serial.print("Soil value: ");
  Serial.println(soilMoistureValue);

  if (!asPercent)
  {
    return soilMoistureValue;
  }

  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
  Serial.print("Air value: ");
  Serial.println(AirValue);
  Serial.print("Water value: ");
  Serial.println(WaterValue);
  Serial.print("Percent value: ");
  Serial.println(soilmoisturepercent);
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

void showEasterEgg(char *defaultLine1, char *defaultLine2)
{
  display.clearDisplay();
  char *line1_egg = "Fuer Spatz <3";
  char *line2_egg = "Von Hase - I LOVE YOU";
  displayText(first_line_small_y, 1, line1_egg);
  displayText(second_line_small_y, 1, line2_egg);
  display.display();
  delay(EASTER_EGG_TIMEOUT);
  display.clearDisplay();
  displayText(first_line_small_y, 1, defaultLine1);
  displayText(second_line_small_y, 1, defaultLine2);
  display.display();
}

bool checkForEasterEgg(char *line1, char *line2, bool plus_pressed)
{
  int button_high = PLUS_BUTTON;
  int button_missing = MINUS_BUTTON;
  if (!plus_pressed)
  {
    button_high = MINUS_BUTTON;
    button_missing = PLUS_BUTTON;
  }

  delay(100);
  Serial.print(F("Button high: "));
  Serial.print(digitalRead(button_high));
  Serial.print(F(" missing button for easter egg: "));
  Serial.println(button_missing);
  while (digitalRead(button_high) == HIGH)
  {
    if (digitalRead(button_missing) == HIGH)
    {
      Serial.println(F("Easter triggered"));
      showEasterEgg(line1, line2);

      return true;
      break;
    }
  }
  return false;
}

bool waitForInput(char *line1, char *line2)
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
  char *waitText1 = "Bitte";
  char *waitText2 = "warten...";
  displayText(first_line_y, normalTextSize, waitText1);
  displayText(second_line_y, normalTextSize, waitText2);
  delay(1000);
}

void doCalibrateProcedure()
{

  // Show Kalibrierung für eine Zeit lang
  display.clearDisplay();
  Serial.print("Free ram: ");
  Serial.println(freeRam());
  displayText(first_line_y, 1, "Kalibrierung");
  displayText(second_line_y, 2, "lol");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  char *line1 = "Skip Kalib?";
  char *line2 = "L Nein | R Ja";
  Serial.println("Skip calibration?");
  bool skip = waitForInput(line1, line2);

  if (!skip)
  {
    Serial.println(F("Do not skip calibration"));
    line1 = "Sensor in Wasser";
    line2 = "L Abbr. | R Weiter";

    if (!waitForInput(line1, line2))
    {
      return;
    }

    displayPleaseWait();
    display.display();
    long sum = 0;
    for (int i = 0; i < 100; i++)
    {
      sum += get_moisture_value(false);
      delay(20);
    }

    display.clearDisplay();

    int tempWaterValue = sum / 100;
    Serial.print(F("New water value: "));
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
      sum += get_moisture_value(false);
      delay(20);
    }

    display.clearDisplay();

    int tempAirValue = sum / 100;

    Serial.print(F("New air value: "));
    Serial.println(tempAirValue);

    EEPROM.write(WATER_VALUE_ADDRESS, tempWaterValue);
    EEPROM.write(AIR_VALUE_ADDRESS, tempAirValue);
    WaterValue = tempWaterValue;
    AirValue = tempAirValue;
  }
  else
  {
    Serial.println(F("Skip calibration"));
  }

  line1 = "Versp. Leuchten?";
  line2 = "L Nein. | R Ja";
  LightUpAfter1Week = waitForInput(line1, line2);
  EEPROM.write(LATER_REMINDER_VALUE_ADDRESS, LightUpAfter1Week);
  timeSinceUnderThreshold = 0;
  EEPROM.write(TIME_SINCE_UNDER_THRESHOLD, timeSinceUnderThreshold);

  if (LightUpAfter1Week)
  {
    doNewTargetLightUpProcedure();
  }
}

void displaySecondsLeft(unsigned long start)
{
  char secondsBuffer[10];
  itoa(int((5000 - (millis() - start)) / 1000), secondsBuffer, 10);
  strcat(secondsBuffer, "s left");
  displayText(second_line_y, normalTextSize, secondsBuffer);
}

void doNewTargetProcedure()
{
  unsigned long timeSinceLastChange = millis();
  unsigned long start = millis();
  while (millis() - start < 5000)
  {
    display.clearDisplay();

    char buffer[10];
    strcpy(buffer, "New:");
    char thresholdBuffer[10];
    itoa(threshold, thresholdBuffer, 10);
    strcat(buffer, thresholdBuffer);
    strcat(buffer, "% ");

    if (millis() - timeSinceLastChange > 500)
    {
      displaySecondsLeft(start);
    }

    displayText(first_line_y, normalTextSize, buffer);

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

void doNewTargetLightUpProcedure()
{
  unsigned long timeSinceLastChange = millis();
  unsigned long start = millis();
  while (millis() - start < 5000)
  {
    display.clearDisplay();

    char buffer[12];
    strcpy(buffer, "New:");
    char thresholdBuffer[10];
    itoa(daysBeforeLightUp, thresholdBuffer, 10);
    strcat(buffer, thresholdBuffer);
    strcat(buffer, " days");
    if (millis() - timeSinceLastChange > 500)
    {
      displaySecondsLeft(start);
    }

    displayText(first_line_y, normalTextSize, buffer);

    display.display();
    delay(50);
    if (digitalRead(PLUS_BUTTON) == HIGH)
    {
      timeSinceLastChange = millis();
      daysBeforeLightUp += 1;
      if (daysBeforeLightUp > 255)
      {
        daysBeforeLightUp = 255;
      }

      start = millis();
    }
    else if (digitalRead(MINUS_BUTTON) == HIGH)
    {
      timeSinceLastChange = millis();
      daysBeforeLightUp -= 1;
      if (daysBeforeLightUp < 0)
      {
        daysBeforeLightUp = 0;
      }
      start = millis();
    }
  }
  EEPROM.write(DAYS_THRESHOLD_BEFORE_LIGHT_UP, daysBeforeLightUp);
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