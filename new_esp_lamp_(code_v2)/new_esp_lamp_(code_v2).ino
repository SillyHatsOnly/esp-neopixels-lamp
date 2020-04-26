#include "SPI.h"
#include "BlynkSimpleEsp8266.h"
#include "ESP8266WiFi.h"
#include "EEPROM.h"
#include "Adafruit_NeoPixel.h"
//==================================================================================================//
#define PIN                     D3
#define PIXELS_NUM              91
Adafruit_NeoPixel pxls = Adafruit_NeoPixel(PIXELS_NUM, PIN, NEO_GRB + NEO_KHZ800);
//==================================================================================================//
#define RAINBOW_MODE            1
#define RANDOM_COLORS_MODE      2
#define COLOR_WHEEL_MODE        3
#define STROBE_MODE             4
#define SLOW_MOTION_MODE        5
#define CUSTOM_MODE             6

#define RED_VAL                 0
#define GREEN_VAL               1
#define BLUE_VAL                2
#define BRIGHTNESS_VAL          3
#define LED_EFFECT_VAL          4
#define BRIGHTNESS              127
#define R                       255
#define G                       255
#define B                       255
#define NUMBER_OF_LED_COLORS    3
#define LED_EFFECT_NUM          CUSTOM_MODE

#define STROBE_COUNT            5

#define CHECKPOINT_EEPROM_BYTE  50
#define EEPROM_TIME_DELAY       1000
#define EEPROM_UPDATE_TIME      30000
#define FULL_EEPROM_ARRAY_SIZE  512
#define USER_VAL                1

#define BLYNK_PRINT             Serial
//==================================================================================================//
byte DefaultSettings[] = {R, G, B, BRIGHTNESS, LED_EFFECT_NUM};
byte Settings[] = {0, 0, 0, 0, 0};
byte ColorWheelArr[PIXELS_NUM][NUMBER_OF_LED_COLORS];

bool EepromUpdateFlag = true;
uint32_t EepromSaveTimer = 0;

uint32_t LedEffectTimer;
uint8_t TemporaryEffectNumber;
uint16_t LedEffectTimeDelay = 25;
uint8_t RainbowCycle, WheelPos;
uint8_t HsvDegree = 0;

bool PowerButtonState = false;
bool AnimationReverseFlag = false;
bool RandomColorsState = true;

char auth[] = "***********************";
char ssid[] = "***********************";
char pass[] = "***********************";
//==================================================================================================//
void all_pixels_off() {
  for (int i = 0; i < PIXELS_NUM; i++) {
    pxls.setPixelColor(i,
                       pxls.Color(0,
                                  0,
                                  0));
  }
  pxls.show();
}
//==================================================================================================//
void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  pxls.begin();
  randomSeed(analogRead(A0));
  
  EEPROM.begin(FULL_EEPROM_ARRAY_SIZE);
  if (EEPROM.read(CHECKPOINT_EEPROM_BYTE) == USER_VAL) {
    for (int i = 0; i < (sizeof(Settings) / sizeof(Settings[0])); i++) {
      byte EepromCurrentValue = EEPROM.read(i);
      Settings[i] = EepromCurrentValue;
    }
  }
  else {
    for (int i = 0; i < (sizeof(Settings) / sizeof(Settings[0])); i++) {
      Settings[i] = DefaultSettings[i];
    }
  }
  
  TemporaryEffectNumber = Settings[LED_EFFECT_VAL];
  all_pixels_off();
}
//==================================================================================================//
void loop() {
  Blynk.run();
  if (Settings[LED_EFFECT_VAL] != CUSTOM_MODE && PowerButtonState) {
    //==============================================================================================//
    if (Settings[LED_EFFECT_VAL] == RAINBOW_MODE) {
      if (millis() - LedEffectTimer > LedEffectTimeDelay) {
        LedEffectTimer = millis();
        RainbowCycle++;
        for (int i = 0; i < PIXELS_NUM; i++) {
          WheelPos = ((uint16_t)(i * 256 / PIXELS_NUM) + RainbowCycle);
          if (WheelPos < 85) {
            Settings[RED_VAL]   = WheelPos * 3;
            Settings[GREEN_VAL] = 255 - Settings[RED_VAL];
            Settings[BLUE_VAL]  = 0;
          } else if (WheelPos < 170) {
            WheelPos -= 85;
            Settings[RED_VAL]   = 255 - Settings[BLUE_VAL];
            Settings[GREEN_VAL] = 0;
            Settings[BLUE_VAL]  = WheelPos * 3;
          } else {
            WheelPos -= 170;
            Settings[RED_VAL]   = 0;
            Settings[GREEN_VAL] = WheelPos * 3;
            Settings[BLUE_VAL]  = 255 - Settings[GREEN_VAL];
          }
          pxls.setPixelColor(i,
                             pxls.Color(map(Settings[RED_VAL],   0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                        map(Settings[GREEN_VAL], 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                        map(Settings[BLUE_VAL],  0, 255, 0, Settings[BRIGHTNESS_VAL])));
        }
        pxls.show();
      }
    }
    //==============================================================================================//
    else if (Settings[LED_EFFECT_VAL] == RANDOM_COLORS_MODE) {
      if (millis() - LedEffectTimer > LedEffectTimeDelay) {
        LedEffectTimer = millis();
        for (int i = 0; i < PIXELS_NUM; i++) {
          int8_t n = random(3);
          if (n == 0) Settings[RED_VAL]   = 0; else
                      Settings[RED_VAL]   = random(255);
          if (n == 1) Settings[GREEN_VAL] = 0; else
                      Settings[GREEN_VAL] = random(255);
          if (n == 2) Settings[BLUE_VAL]  = 0; else
                      Settings[BLUE_VAL]  = random(255);
          pxls.setPixelColor(i,
                             pxls.Color(map(Settings[RED_VAL],   0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                        map(Settings[GREEN_VAL], 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                        map(Settings[BLUE_VAL],  0, 255, 0, Settings[BRIGHTNESS_VAL])));
        }
        pxls.show();
      }
    }
    //==============================================================================================//
    else if (Settings[LED_EFFECT_VAL] == COLOR_WHEEL_MODE) {
      if (millis() - LedEffectTimer > LedEffectTimeDelay * 2) {
        LedEffectTimer = millis();
        if (HsvDegree == 0  ) HsvDegree = 42;
        else if (HsvDegree == 42 ) HsvDegree = 85;
        else if (HsvDegree == 85 ) HsvDegree = 127;
        else if (HsvDegree == 127) HsvDegree = 170;
        else if (HsvDegree == 170) HsvDegree = 212;
        else if (HsvDegree == 212) HsvDegree = 0;

        if (HsvDegree < 85) {
          Settings[BLUE_VAL]  = 0;
          Settings[RED_VAL]   = HsvDegree * 3;
          Settings[GREEN_VAL] = 255 - Settings[RED_VAL];
        }
        else if (HsvDegree < 170) {
          Settings[GREEN_VAL] = 0;
          Settings[BLUE_VAL]  = HsvDegree * 3;
          Settings[RED_VAL]   = 255 - Settings[BLUE_VAL];
        }
        else {
          Settings[RED_VAL]   = 0;
          Settings[GREEN_VAL] = HsvDegree * 3;
          Settings[BLUE_VAL]  = 255 - Settings[GREEN_VAL];
        }
        pxls.fill(pxls.Color(map(Settings[RED_VAL],   0, 255, 0, Settings[BRIGHTNESS_VAL]),
                             map(Settings[GREEN_VAL], 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                             map(Settings[BLUE_VAL],  0, 255, 0, Settings[BRIGHTNESS_VAL])));
        pxls.show();
      }
    }
    //==============================================================================================//
    else if (Settings[LED_EFFECT_VAL] == STROBE_MODE) {
      if (millis() - LedEffectTimer > LedEffectTimeDelay) {
        LedEffectTimer = millis();
        for (int j = 0; j < STROBE_COUNT; j++) {
          pxls.fill(pxls.Color(map(R, 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                               map(G, 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                               map(B, 0, 255, 0, Settings[BRIGHTNESS_VAL])));
          pxls.show();
          delay(LedEffectTimeDelay / 2);
          pxls.fill(0, 0, 0);
          pxls.show();
          delay(LedEffectTimeDelay / 2);
        }
      }
    }
    //==============================================================================================//
    else if (Settings[LED_EFFECT_VAL] == SLOW_MOTION_MODE) {
      if (millis() - LedEffectTimer > LedEffectTimeDelay) {
        LedEffectTimer = millis();
        if (RandomColorsState) {
          RandomColorsState = false;
          for (int i = 0; i < PIXELS_NUM; i++) {
            int8_t RandomNumber = random(3);
            if (RandomNumber == 0) ColorWheelArr[i][RED_VAL]   = 0; else
                                   ColorWheelArr[i][RED_VAL]   = random(255);
            if (RandomNumber == 1) ColorWheelArr[i][GREEN_VAL] = 0; else
                                   ColorWheelArr[i][GREEN_VAL] = random(255);
            if (RandomNumber == 2) ColorWheelArr[i][BLUE_VAL]  = 0; else
                                   ColorWheelArr[i][BLUE_VAL]  = random(255);
          }
        }
        if (!AnimationReverseFlag) {
          for (int j = 0; j <= Settings[BRIGHTNESS_VAL]; j++) {
            if (j == Settings[BRIGHTNESS_VAL]) AnimationReverseFlag = true;
            for (int i = 0; i < PIXELS_NUM; i++) {
              pxls.setPixelColor(i,
                                 pxls.Color(map(ColorWheelArr[i][RED_VAL],   0, 255, 0, j),
                                            map(ColorWheelArr[i][GREEN_VAL], 0, 255, 0, j),
                                            map(ColorWheelArr[i][BLUE_VAL],  0, 255, 0, j)));
            }
            pxls.show();
            delay(LedEffectTimeDelay / 2);
          }
        }
        else {
          for (int j = Settings[BRIGHTNESS_VAL]; j >= 0; j--) {
            if (j == 0) {
              AnimationReverseFlag = false;
              RandomColorsState = true;
            }
            for (int i = 0; i < PIXELS_NUM; i++) {
              pxls.setPixelColor(i,
                                 pxls.Color(map(ColorWheelArr[i][RED_VAL],   0, 255, 0, j),
                                            map(ColorWheelArr[i][GREEN_VAL], 0, 255, 0, j),
                                            map(ColorWheelArr[i][BLUE_VAL],  0, 255, 0, j)));
            }
            pxls.show();
            delay(LedEffectTimeDelay / 2);
          }
        }
      }
    }
  }
}
//==================================================================================================//
BLYNK_WRITE(V1) { // BRIGHTNESS HORIZONTAL SLIDER
  Settings[BRIGHTNESS_VAL] = param.asInt();
  if (PowerButtonState) {
    for (int i = 0; i < PIXELS_NUM; i++) {
      pxls.setPixelColor(i,
                         pxls.Color(map(Settings[RED_VAL],   0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                    map(Settings[GREEN_VAL], 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                    map(Settings[BLUE_VAL],  0, 255, 0, Settings[BRIGHTNESS_VAL])));
      pxls.show();
    }
  }
}
//==================================================================================================//
BLYNK_WRITE(V2) { // ANIMATION SPEED HORIZONTAL SLIDER
  LedEffectTimeDelay = param.asInt();
}
//==================================================================================================//
BLYNK_WRITE(V3) { // POWER BUTTON
  PowerButtonState = param.asInt();
  if (PowerButtonState) {
    Settings[LED_EFFECT_VAL] = TemporaryEffectNumber;
    if (Settings[LED_EFFECT_VAL] == CUSTOM_MODE) {
      for (int i = 0; i < PIXELS_NUM; i++) {
        pxls.setPixelColor(i,
                           pxls.Color(map(Settings[RED_VAL],   0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                      map(Settings[GREEN_VAL], 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                      map(Settings[BLUE_VAL],  0, 255, 0, Settings[BRIGHTNESS_VAL])));
      }
      pxls.show();
    }
    else {
      LedEffectTimer = millis() - LedEffectTimeDelay;
    }
  }
  else {
    TemporaryEffectNumber = Settings[LED_EFFECT_VAL];
    Settings[LED_EFFECT_VAL] = CUSTOM_MODE;
    all_pixels_off();
  }
}
//==================================================================================================//
BLYNK_WRITE(V4) { // ZeRGBa
  Settings[RED_VAL]   = param[0].asInt();
  Settings[GREEN_VAL] = param[1].asInt();
  Settings[BLUE_VAL]  = param[2].asInt();
  if (PowerButtonState) {
    for (int i = 0; i < PIXELS_NUM; i++) {
      pxls.setPixelColor(i,
                         pxls.Color(map(Settings[RED_VAL],   0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                    map(Settings[GREEN_VAL], 0, 255, 0, Settings[BRIGHTNESS_VAL]),
                                    map(Settings[BLUE_VAL],  0, 255, 0, Settings[BRIGHTNESS_VAL])));
      pxls.show();
    }
  }
}
//==================================================================================================//
BLYNK_WRITE(V5) { // LED EFFECTS MENU
  switch (param.asInt()) {
    case 1: Settings[LED_EFFECT_VAL] = RAINBOW_MODE;       LedEffectTimer = millis() - LedEffectTimeDelay; break;
    case 2: Settings[LED_EFFECT_VAL] = RANDOM_COLORS_MODE; LedEffectTimer = millis() - LedEffectTimeDelay; break;
    case 3: Settings[LED_EFFECT_VAL] = COLOR_WHEEL_MODE;   LedEffectTimer = millis() - LedEffectTimeDelay; break;
    case 4: Settings[LED_EFFECT_VAL] = STROBE_MODE;        LedEffectTimer = millis() - LedEffectTimeDelay; break;
    case 5: Settings[LED_EFFECT_VAL] = SLOW_MOTION_MODE;   LedEffectTimer = millis() - LedEffectTimeDelay; break;
    case 6: Settings[LED_EFFECT_VAL] = CUSTOM_MODE;        LedEffectTimer = millis() - LedEffectTimeDelay; break;
  }
}
//==================================================================================================//
BLYNK_WRITE(V6) { // EEPROM WRITE
  int pinValue = param.asInt();
  if (pinValue && PowerButtonState && (millis() - EepromSaveTimer > EEPROM_TIME_DELAY)) {
    EepromSaveTimer = millis();
    EEPROM.write(CHECKPOINT_EEPROM_BYTE, 1);
    for (int i = 0; i < (sizeof(Settings) / sizeof(Settings[0])); i++) {
      byte EepromCurrentValue = Settings[i];
      EEPROM.write(i, EepromCurrentValue);
    }
    EEPROM.commit();
  }
}
//==================================================================================================//
