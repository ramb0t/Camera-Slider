#ifndef GLOBAL_H
#define GLOBAL_H

#include <Adafruit_SSD1306.h>

// Global Defines
/******************************************************************************/
#define DEBUG
#define DEBUG_TICKS 50000
#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif

// Menu states
#define SPEEDITEM -1
#define DIRITEM   -1
#define STARTITEM 3
#define HOUR_ITEM 0
#define MIN_ITEM  1
#define SEC_ITEM  2
#define CAL_ITEM  4
#define FRUN_ITEM 5
#define ITEMEND   4

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

// Calibration Variables
extern long calibration_steps;


extern Adafruit_SSD1306 display;
// Global Functions
/******************************************************************************/
//void print_global_x();

#endif
