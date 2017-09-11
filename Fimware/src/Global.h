#ifndef GLOBAL_H
#define GLOBAL_H

#include <Adafruit_SSD1306.h>

// Global Defines
/******************************************************************************/
//#define DEBUG_OLED
//define DEBUG
#define DEBUG_TICKS 50000
#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif

// Menu states
#define SPEEDITEM 4
#define DIRITEM   5
#define STARTITEM 3
#define HOUR_ITEM 0
#define MIN_ITEM  1
#define SEC_ITEM  2
#define CAL_ITEM  6
#define FRUN_ITEM 7
#define ITEMEND   6

// Directions
#define FORWARD   HIGH
#define BACKWARD  LOW


// Global Variables
/******************************************************************************/

// Timer ISR vars
extern volatile long step_count;
//extern volatile int actual_speed;
extern volatile unsigned int tick_count;
extern volatile bool running;
extern volatile bool MAX_FLAG;
extern volatile bool MIN_FLAG;
//extern volatile int actual_direction;

//Encoder vars
extern volatile byte encoderPos;
extern volatile byte oldEncPos;

//Debug Vars
#ifdef DEBUG
  extern bool DEBUG_SERIAL;
  extern volatile unsigned long debug_ticker;
#endif

//State Machine vars
extern bool itemSelect;
extern int item;
extern volatile int status;

//Time Variables
extern byte hours;
extern byte minutes;
extern byte seconds;

// step calc variables
extern long steps_sec;
extern long ints_step;
extern long ints_step_count;
extern byte speed;
extern int actual_direction;

// Calibration Variables
extern long calibration_steps;


extern Adafruit_SSD1306 display;
// Global Functions
/******************************************************************************/
//void print_global_x();

#endif
