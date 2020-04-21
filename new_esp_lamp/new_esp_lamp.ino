#include "SPI.h"
#include "BlynkSimpleEsp8266.h"
#include "ESP8266WiFi.h"
#include "EEPROM.h"
#include "Adafruit_NeoPixel.h"

#define PIN                     D3
#define PIXELS_NUM              91
Adafruit_NeoPixel pxls = Adafruit_NeoPixel(PIXELS_NUM, PIN, NEO_GRB + NEO_KHZ800);
//==================================================================================================//
#define BLYNK_PRINT             Serial

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
#define R_MAX                   255
#define G_MAX                   255
#define B_MAX                   255
#define R_OFF                   0
#define G_OFF                   0
#define B_OFF                   0
#define NUMBER_OF_LED_COLORS    3

#define LED_EFFECT_NUM          CUSTOM_MODE
#define STROBE_COUNT            5

#define CHECKPOINT_EEPROM_BYTE  50
#define EEPROM_TIME_DELAY       1000
#define EEPROM_UPDATE_TIME      30000
#define FULL_EEPROM_ARRAY_SIZE  512
#define USER_VAL                1
//==================================================================================================//
byte DefaultSettings[] = {R_MAX, G_MAX, B_MAX, BRIGHTNESS, LED_EFFECT_NUM};
byte Settings[] = {0, 0, 0, 0, 0};
byte ColorWheelArr[PIXELS_NUM][NUMBER_OF_LED_COLORS];

bool EepromUpdateFlag = true;
uint32_t EepromSaveTimer = 0;

uint32_t LedEffectTimer;
uint8_t TemporaryEffectNumber;
uint16_t LedEffectTimeDelay = 25;
uint8_t j, k;
uint8_t HsvAngle = 0;

bool PowerButtonState = false;
bool AnimationReverseFlag = false;
bool RandomColorsState = true;

char auth[] = "***********************";
char ssid[] = "***********************";
char pass[] = "***********************";
//==================================================================================================//
void eeprom_starting_settings(){
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
}
//==================================================================================================//
void set_each_pixel_color_func(byte WorkingMode, byte Brightness, byte R = Settings[RED_VAL], byte G = Settings[GREEN_VAL], byte B = Settings[BLUE_VAL]){
  for (int i = 0; i < PIXELS_NUM; i++) {
    if(WorkingMode == RAINBOW_MODE){
      k=((uint16_t)(i*256 / PIXELS_NUM) + j);
      if(k < 85) {           R = k*3;     G = 255 - R; B = 0;      } else
      if(k < 170){ k -= 85;  R = 255 - B; G = 0;       B = k*3;    } else
                 { k -= 170; R = 0;       G = k*3;     B = 255 - G;}
    }
    if(WorkingMode == RANDOM_COLORS_MODE){
      int8_t n = random(3);
      if(n == 0){R = 0;} else {R = random(255);}
      if(n == 1){G = 0;} else {G = random(255);}
      if(n == 2){B = 0;} else {B = random(255);}
    }
    if(WorkingMode == SLOW_MOTION_MODE){
      R = ColorWheelArr[i][RED_VAL];
      G = ColorWheelArr[i][GREEN_VAL];
      B = ColorWheelArr[i][BLUE_VAL];
    }
    pxls.setPixelColor(i,
                       pxls.Color(map(R, 0, 255, 0, Brightness),
                                  map(G, 0, 255, 0, Brightness),
                                  map(B, 0, 255, 0, Brightness)));
  }
}
//==================================================================================================//
void set_all_pixels_color_func(byte R = Settings[RED_VAL], byte G = Settings[GREEN_VAL], byte B = Settings[BLUE_VAL], byte Brightness = Settings[BRIGHTNESS_VAL]){
  pxls.fill(pxls.Color(map(R, 0, 255, 0, Brightness),
                       map(G, 0, 255, 0, Brightness),
                       map(B, 0, 255, 0, Brightness)));
  pxls.show();
}

void pixels_off(){
  set_each_pixel_color_func(R_OFF, G_OFF, B_OFF, CUSTOM_MODE);
  pxls.show();
}
//==================================================================================================//
void rainbow_mode_func(){
  if(millis() - LedEffectTimer > LedEffectTimeDelay){
    LedEffectTimer = millis();
    j++;
    set_each_pixel_color_func(RAINBOW_MODE, Settings[BRIGHTNESS_VAL]);
    pxls.show();
  }
}
//==================================================================================================//
void random_colors_mode_func(){
  if(millis() - LedEffectTimer > LedEffectTimeDelay){
    LedEffectTimer = millis();
    set_each_pixel_color_func(RANDOM_COLORS_MODE, Settings[BRIGHTNESS_VAL]);
    pxls.show();
  }
}
//==================================================================================================//
void color_wheel_mode_func(){
  if(millis() - LedEffectTimer > LedEffectTimeDelay*2){
    LedEffectTimer = millis();
    if(HsvAngle == 0  ){HsvAngle = 42; } else
    if(HsvAngle == 42 ){HsvAngle = 85; } else
    if(HsvAngle == 85 ){HsvAngle = 127;} else
    if(HsvAngle == 127){HsvAngle = 170;} else
    if(HsvAngle == 170){HsvAngle = 212;} else
    if(HsvAngle == 212){HsvAngle = 0;  }
    if(HsvAngle < 85)  {Settings[BLUE_VAL]  = 0; Settings[RED_VAL]   = HsvAngle*3; Settings[GREEN_VAL] = 255 - Settings[RED_VAL];  } else
    if(HsvAngle < 170) {Settings[GREEN_VAL] = 0; Settings[BLUE_VAL]  = HsvAngle*3; Settings[RED_VAL]   = 255 - Settings[BLUE_VAL]; } else
                       {Settings[RED_VAL]   = 0; Settings[GREEN_VAL] = HsvAngle*3; Settings[BLUE_VAL]  = 255 - Settings[GREEN_VAL];}
    set_all_pixels_color_func();
  }
}
//==================================================================================================//
void strobe_mode_func(){
  if(millis() - LedEffectTimer > LedEffectTimeDelay){
    LedEffectTimer = millis();
    for(int j = 0; j < STROBE_COUNT; j++) {
      set_all_pixels_color_func(R_MAX, G_MAX, B_MAX);
      delay(LedEffectTimeDelay / 2);
      set_all_pixels_color_func(R_OFF, G_OFF, B_OFF);
      delay(LedEffectTimeDelay / 2);
    }
  }
}
//==================================================================================================//
void slow_motion_mode_func(){
  if(millis() - LedEffectTimer > LedEffectTimeDelay){
    LedEffectTimer = millis();
    if(RandomColorsState){
      RandomColorsState = false;
      for (int i = 0; i < PIXELS_NUM; i++) {
        int8_t RandomNumber = random(3);
        if(RandomNumber == 0){ColorWheelArr[i][RED_VAL]   = 0;} else {ColorWheelArr[i][RED_VAL]   = random(255);}
        if(RandomNumber == 1){ColorWheelArr[i][GREEN_VAL] = 0;} else {ColorWheelArr[i][GREEN_VAL] = random(255);}
        if(RandomNumber == 2){ColorWheelArr[i][BLUE_VAL]  = 0;} else {ColorWheelArr[i][BLUE_VAL]  = random(255);}
      }
    }
    if(!AnimationReverseFlag){
      for(int j = 0; j <= Settings[BRIGHTNESS_VAL]; j++){
       if (j == Settings[BRIGHTNESS_VAL]) {AnimationReverseFlag = true;}
       set_each_pixel_color_func(SLOW_MOTION_MODE, j);
       pxls.show();
       delay(LedEffectTimeDelay / 2);
      }
    }
    else {
      for(int j = Settings[BRIGHTNESS_VAL]; j >= 0; j--){
       if (j == 0) {AnimationReverseFlag = false; RandomColorsState = true;}
       set_each_pixel_color_func(SLOW_MOTION_MODE, j);
       pxls.show();
       delay(LedEffectTimeDelay / 2);
      }
    }
  }
}
//==================================================================================================//
//==================================================================================================//
void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  pxls.begin();
  randomSeed(analogRead(A0));
  EEPROM.begin(FULL_EEPROM_ARRAY_SIZE);
  eeprom_starting_settings();
  TemporaryEffectNumber = Settings[LED_EFFECT_VAL];
  pixels_off();
}

void loop() {
  Blynk.run();
  if(Settings[LED_EFFECT_VAL] != CUSTOM_MODE && PowerButtonState){
    if(Settings[LED_EFFECT_VAL] == RAINBOW_MODE)       rainbow_mode_func();       else
    if(Settings[LED_EFFECT_VAL] == RANDOM_COLORS_MODE) random_colors_mode_func(); else
    if(Settings[LED_EFFECT_VAL] == COLOR_WHEEL_MODE)   color_wheel_mode_func();   else
    if(Settings[LED_EFFECT_VAL] == STROBE_MODE)        strobe_mode_func();        else
    if(Settings[LED_EFFECT_VAL] == SLOW_MOTION_MODE)   slow_motion_mode_func();
  }
}
//==================================================================================================//
//==================================================================================================//
BLYNK_WRITE(V1) { // BRIGHTNESS HORIZONTAL SLIDER
  Settings[BRIGHTNESS_VAL] = param.asInt();
  if (PowerButtonState) {
    set_each_pixel_color_func(CUSTOM_MODE, Settings[BRIGHTNESS_VAL]);
    pxls.show();
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
    if(Settings[LED_EFFECT_VAL] == CUSTOM_MODE){
      set_each_pixel_color_func(CUSTOM_MODE, Settings[BRIGHTNESS_VAL]);
      pxls.show();
    }
    else {
      LedEffectTimer = millis() - LedEffectTimeDelay;
    }
  }
  else {
    TemporaryEffectNumber = Settings[LED_EFFECT_VAL];
    Settings[LED_EFFECT_VAL] = CUSTOM_MODE;
    pixels_off();
  }
}
//==================================================================================================//
BLYNK_WRITE(V4) { // ZeRGBa
  Settings[RED_VAL]   = param[0].asInt();
  Settings[GREEN_VAL] = param[1].asInt();
  Settings[BLUE_VAL]  = param[2].asInt();
  if (PowerButtonState) {
    set_each_pixel_color_func(CUSTOM_MODE, Settings[BRIGHTNESS_VAL]);
    pxls.show();
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
