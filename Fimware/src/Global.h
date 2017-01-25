#ifndef GLOBAL_H
#define GLOBAL_H

#include <Adafruit_SSD1306.h>

// Global Defines
/******************************************************************************/
// Menu states
#define SPEEDMENU 0
#define DIRMENU   1
#define STARTMENU 2
#define MENUEND   2

// Directions
#define FORWARD   HIGH
#define BACKWARD  LOW


// Global Variables
/******************************************************************************/

// Timer ISR vars
extern volatile long step_count;
extern volatile int actual_speed;
extern volatile int ticks;
extern volatile unsigned int tick_count;
extern volatile unsigned long DEBUG_TICKS;
extern volatile bool running;
extern volatile bool MAX_FLAG;
extern volatile bool MIN_FLAG;
extern volatile int status;
extern volatile int actual_direction;

//Encoder vars
extern volatile byte encoderPos;
extern volatile byte oldEncPos;

//Debug flags
extern bool DEBUG_SERIAL;
extern bool DEBUG_OLED;

//State Machine vars
extern bool menuSelect;
extern int menu;
extern volatile int status;


extern Adafruit_SSD1306 display;
// Global Functions
/******************************************************************************/
//void print_global_x();

#endif
