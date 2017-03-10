#ifndef OLED_H
#define OLED_H

// Includes:
/*****************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeMono9pt7b.h>
#include "logo.h"
#include "pinDefines.h"
#include "Global.h"

// Defines:
/*****************************************************************************/

// Variables:
/*****************************************************************************/

// Function Prototypes:
/*****************************************************************************/
void OLED_Init();
void OLED_DisplayLogo();
void OLED_Update();
void OLED_Clear();
void OLED_Print(String s, int x = 0, int y = 1, int z = 1); // prints a string in size z at x and y . 

#endif
