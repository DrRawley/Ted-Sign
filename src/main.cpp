#include <Arduino.h>
#include <FastLED.h>
#include <ezButton.h>
#include <EEPROM.h>
#include <math.h>

// Digital Pins
#define DATA_PIN 2
#define SELECT_BUTTON_PIN 10

// Analog Pins
#define POT_PIN 0
#define MIC_PIN 1

// Mic Sensitivities
#define EEPROM_MIC_BASE_LEVEL_LOCATION 1
float micBaseLevel = 3.0;
float micScalingFactor = 10.0;
int micOffset = 0;

#define NUM_LEDS 39
#define MAX_BRIGHTNESS 0x64
CRGB leds[NUM_LEDS];

// Setup buttons
ezButton button1(SELECT_BUTTON_PIN);

byte selection = 0;

// Dither Array Setup
const byte ditherSize = 8;
const float logDithersize = log10((float)(ditherSize + 1));
const byte dither[ditherSize] =
    {
        (byte)(MAX_BRIGHTNESS),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 1) / logDithersize),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 2) / logDithersize),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 3) / logDithersize),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 4) / logDithersize),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 5) / logDithersize),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 6) / logDithersize),
        (byte)(MAX_BRIGHTNESS * log10(ditherSize - 7) / logDithersize)};

// Define LEDs per Letter
const int letter1[10] = {5, 6, 7, 8, 9, 4, 3, 2, 1, 0};
const int letter2[15] = {17, 16, 15, 18, 14, 19, 10, 11, 12, 13, 20, 21, 22, 23, 24};
const int letter3[14] = {25, 26, 36, 37, 38, 27, 35, 28, 34, 29, 33, 32, 31, 30};
const int sizeLetter1 = 10;
const int sizeLetter2 = 15;
const int sizeLetter3 = 14;

// Define LEDs per row
const int numRows = 6;
const int rows[numRows][9] = {
    {0, 21, 22, 23, 24, 33, 32, 31, 30},
    {1, 20, 34, 29},
    {2, 19, 10, 11, 12, 13, 35, 28},
    {3, 18, 14, 36, 37, 38, 27},
    {4, 17, 16, 15, 26},
    {5, 6, 7, 8, 9, 25}};
const int rowSizes[numRows] = {9, 4, 8, 7, 5, 6};

// Define Leds per column
const int numColumns = 17;
const int columns[numColumns][6] = {
    {5},
    {6},
    {7, 4, 3, 2, 1, 0},
    {8},
    {9},
    {},
    {18, 19, 20},
    {17, 10, 21},
    {16, 11, 22},
    {15, 12, 23},
    {14, 13, 24},
    {},
    {35, 34},
    {36, 33},
    {37, 32},
    {38, 31},
    {25, 26, 27, 28, 29, 30}};
const int colSizes[numColumns] = {1, 1, 6, 1, 1, 0, 3, 3, 3, 3, 3, 0, 2, 2, 2, 2, 6};

// Define LEDs per Diagonal
const int numDiags = 22;
const int diags[numDiags][4] = {
    {},
    {},
    {0},
    {1},
    {2},
    {5, 3},
    {6, 4},
    {7, 20, 21},
    {8, 19, 22},
    {9, 18, 10, 23},
    {11, 24},
    {17, 12},
    {16, 13},
    {15, 14, 34, 33},
    {35, 32},
    {31},
    {36, 30},
    {37, 29},
    {38, 28},
    {27},
    {26},
    {25}};
const int diagsSizes[numDiags] = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 2, 2, 2, 4, 2, 1, 2, 2, 2, 1, 1, 1};

// Define LEDs for opposite diagonal
const int diagsBackwards[numDiags][5] = {
    {5},
    {6},
    {7},
    {8, 4},
    {9, 3},
    {2},
    {1},
    {0},
    {17, 18},
    {16, 19},
    {15, 10, 20},
    {11},
    {14, 12, 21},
    {13, 22},
    {23},
    {36, 35, 24},
    {25, 37, 34},
    {26, 38},
    {27, 33},
    {28, 32},
    {29, 31},
    {30}};
const int diagsBackwardsSizes[numDiags] = {1, 1, 1, 2, 2, 1, 1, 1, 2, 2, 3, 1, 3, 2, 1, 3, 3, 2, 2, 2, 2, 1};

// Declare function definitions
const int numberSelections = 10;
void solid(byte hue);
void colorLettersStatic(byte hue);
void chase(byte hue);
void verticalChase(byte hue);
void verticalRainbow(byte potValue);
void horizontalChase(byte hue);
void diagChase(byte hue, byte diagsDirection);

// Function definitions for audio input
int getMicValue(void);
void serialCheckAndSet(void);
float setMicBaseLevel(float);
float setMicScalingFactor(float);
int setMicOffset(int);
float getSerialFloat(void);
void basicVUMeter(int);
void intensityVUMeter(int);

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(2));
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  button1.setDebounceTime(100);
  selection = EEPROM.read(0);
  if (!(selection < numberSelections))
  {
    selection = 0;
  }
  float readValue = 0;
  EEPROM.get(EEPROM_MIC_BASE_LEVEL_LOCATION, readValue);
  setMicBaseLevel(readValue);
  Serial.print("Loaded mic base level: ");
  Serial.print(micBaseLevel);
  EEPROM.get(EEPROM_MIC_BASE_LEVEL_LOCATION + sizeof(float), readValue);
  setMicScalingFactor(readValue);
  Serial.print("  Loaded mic scaling factor: ");
  Serial.println(micScalingFactor);
  int readIntValue = 0;
  EEPROM.get(EEPROM_MIC_BASE_LEVEL_LOCATION + sizeof(float) * 2, readIntValue);
  setMicOffset(readIntValue);
  Serial.print("  Loaded mic offset: ");
  Serial.println(micOffset);
  Serial.println("Enter 'b' to set Mic Base Level, 'f' to set Mic Scaling Factor, or 'o' to set Mic Offset.");
}

void loop()
{
  static int potValue = 0;
  button1.loop();

  serialCheckAndSet();

  if (button1.isPressed())
  {
    selection++;
    if (!(selection < numberSelections))
    {
      selection = 0;
    }
    EEPROM.update(0, selection);
  }
  potValue = (int)map(analogRead(POT_PIN), 0, 1024, 0, 0xff);

  switch (selection)
  {
  case 0:
    // colorLettersStatic(potValue);
    colorLettersStatic(potValue); // Make zero so that it stays on green-blue-green
    break;
  case 1:
    solid(potValue);
    break;
  case 2:
    chase(potValue);
    break;
  case 3:
    verticalChase(potValue);
    break;
  case 4:
    verticalRainbow(potValue);
    break;
  case 5:
    horizontalChase(potValue);
    break;
  case 6:
    diagChase(potValue, 0);
    break;
  case 7:
    diagChase(potValue, 1); // 1 for backwards diagonals
    break;
  case 8:
    basicVUMeter(potValue);
    break;
  case 9:
    intensityVUMeter(potValue);
    break;
  default:
    break;
  }
}

void solid(byte hue)
{
  for (int dot = 0; dot < NUM_LEDS; dot++)
  {
    leds[dot].setHSV(hue, 0xff, 0x40);
  }
  FastLED.show();
}

void chase(byte hue)
{
  static int offset = 0;
  static byte iLimit = (byte)(NUM_LEDS / ditherSize) + 1;
  static unsigned long timer = millis();

  if (millis() - timer > 100)
  {
    for (int i = 0; i < iLimit; i++)
    {
      for (int j = 0; j < ditherSize; j++)
      {
        int index = i * ditherSize + j;
        if (index < NUM_LEDS) // Make sure not writing outside of length of leds[]
        {
          leds[i * ditherSize + j].setHSV(hue, 0xff, dither[(ditherSize - j + offset) % ditherSize]); // this index was annoying
        }
      }
    }
    FastLED.show();
    offset++;
    timer = millis();
  }
}

void verticalChase(byte hue)
{
  static int offset = 0;
  static unsigned long timer = millis();

  if (millis() - timer > 100)
  {
    for (int i = 0; i < numColumns; i++)
    {
      for (int j = 0; j < colSizes[i]; j++)
      {
        leds[columns[i][j]].setHSV(hue, 0xff, dither[(ditherSize - i % ditherSize + offset) % ditherSize]);
      }
    }
    FastLED.show();
    offset++;
    timer = millis();
  }
}

void verticalRainbow(byte potValue)
{
  static int offset = 0;
  static unsigned long timer = millis();
  int unsigned delay = map(potValue, 0, 255, 20, 2000);

  if (millis() - timer > delay)
  {
    for (int i = 0; i < numColumns; i++)
    {
      for (int j = 0; j < colSizes[i]; j++)
      {
        leds[columns[i][j]].setHSV((i * 10 + offset) % 255, 0xff, 0x40);
      }
    }
    FastLED.show();
    offset++;
    timer = millis();
  }
}

void horizontalChase(byte hue)
{
  static int offset = 0;
  static unsigned long timer = millis();

  if (millis() - timer > 100)
  {
    for (int i = 0; i < numRows; i++)
    {
      for (int j = 0; j < rowSizes[i]; j++)
      {
        leds[rows[i][j]].setHSV(hue, 0xff, dither[(ditherSize - i % ditherSize + offset) % ditherSize]);
      }
    }
    FastLED.show();
    offset++;
    timer = millis();
  }
}

void colorLettersStatic(byte hue)
{
  static int offset = 0;
  static unsigned long timer = millis();
  static int brightness = 0x40;

  if (millis() - timer > 200)
  {
    for (int i = 0; i < sizeLetter1; i++)
    {
      leds[letter1[i]].setHSV((0 + hue) % 0xff, 0xff, brightness); // 0 = red
    }
    for (int i = 0; i < sizeLetter2; i++)
    {
      leds[letter2[i]].setHSV((85 + hue) % 0xff, 0xff, brightness); // 85 = green
    }
    for (int i = 0; i < sizeLetter3; i++)
    {
      leds[letter3[i]].setHSV((171 + hue) % 0xff, 0xff, brightness); // 171 = blue
    }

    FastLED.show();
    offset++;
  }
}

void diagChase(byte hue, byte diagsDirection)
{
  static int offset = 0;
  static unsigned long timer = millis();
  int size = 0;

  if (millis() - timer > 100)
  {
    for (int i = 0; i < numDiags; i++)
    {
      if (diagsDirection == 0)
      {
        size = diagsSizes[i];
      }
      else
      {
        size = diagsBackwardsSizes[i];
      }
      for (int j = 0; j < size; j++)
      {
        if (diagsDirection == 0)
        {
          leds[diags[i][j]].setHSV(hue, 0xff, dither[(ditherSize - i % ditherSize + offset) % ditherSize]);
        }
        else
        {
          leds[diagsBackwards[i][j]].setHSV(hue, 0xff, dither[(ditherSize - i % ditherSize + offset) % ditherSize]);
        }
      }
    }
    FastLED.show();
    offset++;
    timer = millis();
  }
}

int getMicValue(void)
{
  int val = 0;
  const int sampleSize = 32;

  int max = 0;
  int min = 1024;
  for (int i = 0; i < sampleSize; i++)
  {
    val = analogRead(MIC_PIN);
    max = max(max, val);
    min = min(min, val);
  }
  int delta = max - min - micOffset;
  if (delta > 0)
  {
    return delta;
  }
  else
  {
    return 0;
  }
}

float setMicBaseLevel(float newMicBaseLevel)
{
  if (newMicBaseLevel > 0)
  {
    micBaseLevel = newMicBaseLevel;
  }

  return micBaseLevel;
}

float setMicScalingFactor(float newMicScalingFactor)
{
  if (newMicScalingFactor > 0)
  {
    micScalingFactor = newMicScalingFactor;
  }

  return micScalingFactor;
}

int setMicOffset(int newMicOffset)
{
  if (newMicOffset >= 0 && newMicOffset < 1024)
  {
    micOffset = newMicOffset;
  }

  return micOffset;
}

float getSerialFloat(void)
{
  byte index = 0;
  char endMarker = '\n';
  char rc;
  bool receivedEndMarker = false;
  const byte numChars = 6;
  char receivedChars[numChars];
  while (receivedEndMarker == false)
  {
    while (!Serial.available())
      ;

    rc = Serial.read();
    if (rc != endMarker)
    {
      if (isDigit(rc) || rc == '.')
      {
        receivedChars[index] = rc;
        index++;
        Serial.print(rc);
      }

      if (index >= numChars)
      {
        index = numChars - 1;
      }
    }
    else
    {
      receivedChars[index] = '\0';
      index = 0;
      receivedEndMarker = true;
      Serial.println("");
    }
  }

  return atof(receivedChars);
}

void serialCheckAndSet()
{
  if (Serial.available())
  {
    char option = Serial.read();
    Serial.println(option);

    if (option == '\n')
    {
      Serial.print("Current mic base level: ");
      Serial.print(micBaseLevel);
      Serial.print("  Current mic scaling factor: ");
      Serial.println(micScalingFactor);
      Serial.print("  Current mic offset: ");
      Serial.println(micOffset);
      Serial.println("Enter 'b' to set Mic Base Level, 'f' to set Mic Scaling Factor, or 'o' to set Mic Offset.");
    }
    if (option == 'b' || option == 'B')
    {
      Serial.print("Current mic base level: ");
      Serial.println(micBaseLevel);
      Serial.print("Enter new mic base level: ");

      float result = getSerialFloat();
      result = setMicBaseLevel(result);
      EEPROM.put(EEPROM_MIC_BASE_LEVEL_LOCATION, micBaseLevel);

      Serial.print("New Mic Base Level: ");
      Serial.println(result);
    }
    if (option == 'f' || option == 'F')
    {
      Serial.print("Current mic scaling factor: ");
      Serial.println(micScalingFactor);
      Serial.print("Enter new mic scaling factor: ");

      float result = getSerialFloat();
      result = setMicScalingFactor(result);
      EEPROM.put(EEPROM_MIC_BASE_LEVEL_LOCATION + sizeof(float), micScalingFactor);

      Serial.print("New Mic Scaling Factor: ");
      Serial.println(result);
    }
    if (option == 'o' || option == 'O')
    {
      Serial.print("Current mic offset: ");
      Serial.println(micOffset);
      Serial.print("Enter new mic offset factor: ");

      int result = (int)getSerialFloat();
      result = setMicOffset(result);
      EEPROM.put(EEPROM_MIC_BASE_LEVEL_LOCATION + sizeof(float) * 2, micOffset);

      Serial.print("New Mic Offset: ");
      Serial.println(result);
    }
  }
}

void basicVUMeter(int hue)
{
  // int numIntensities = numColumns * 2; // 2 intensities per column
  int numIntensities = numRows * 4;
  int value = getMicValue();

  int intensity = (int)(micScalingFactor * log10((float)value / micBaseLevel));

  if (intensity > numIntensities - 1)
  {
    intensity = numIntensities - 1;
  }
  else if (intensity < 0)
  {
    intensity = 0;
  }
  Serial.print(value);
  Serial.print(" - ");
  Serial.println(intensity);

  // const byte brightnessGradient[4] = {32, 64, 96, 128};
  const byte brightnessGradient[4] = {MAX_BRIGHTNESS / 4, MAX_BRIGHTNESS / 2, MAX_BRIGHTNESS * 3 / 4, MAX_BRIGHTNESS};
  const byte colorGradient[6] = {96, 96, 96, 96, 64, 0}; // grn, grn, grn, grn, yellow, red
  // Loop to set row color and brightness based off of intensity; only necessary to loop up to the intensity
  for (int i = 0; i < intensity; i++)
  {
    // int rowNumberIndex = 4 - (int)(i / 4); // Calc current row based on index --> 4 intensities per row
    int rowNumberIndex = (int)(i / 4);
    int brightness = brightnessGradient[i % 4];

    for (int j = 0; j < rowSizes[rowNumberIndex]; j++) // Fill leds for current row
    {
      leds[rows[rowNumberIndex][j]].setHSV(colorGradient[rowNumberIndex], 0xff, brightness);
    }
  }

  FastLED.show();

  for (int dot = 0; dot < NUM_LEDS; dot++)
  {
    leds[dot].setHSV(hue, 0xff, 0x10);
  }
}

void intensityVUMeter(int hue)
{
  int numIntensities = 20;
  int value = getMicValue();

  int intensity = (int)(micScalingFactor * log10((float)value / micBaseLevel));

  if (intensity > numIntensities) // Don't need a "- 1" here because it's not being indexed
  {
    intensity = numIntensities;
  }
  else if (intensity < 0)
  {
    intensity = 0;
  }

  // int intensity = (int)map(value, 0, 1024, 0, numIntensities - 1);

  int brightness = 0;
  brightness = (MAX_BRIGHTNESS - 0x10) * intensity / numIntensities + 0x10;
  // int hue = 0;
  // hue = (85 - 160) * intensity / numIntensities + 160;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i].setHSV(hue % 0xff, 0xff, brightness);
  }

  FastLED.show();
}
