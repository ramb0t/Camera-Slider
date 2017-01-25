/*

 */

#include <Arduino.h>
//#include "A4988.h"
#include <TimerOne.h>
// #include <SPI.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include <Fonts/FreeMono9pt7b.h>
#include "pinDefines.h"
#include "Timer1.h"
#include "Global.h"
#include "Encoder.h"
#include "OLED.h"


// Defines
/*****************************************************************************/

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200

// Since microstepping is set externally, make sure this matches the selected mode
// 1=full step, 2=half step etc.
#define MICROSTEPS 1

// steps / rev
#define STEPS_REV MOTOR_STEPS * MICROSTEPS

// mm movement per revolution, belt pitch * pulley teeth
#define MM_REV 2*20

// Motor max speed
#define MAX_SPEED  75

//interrupt period in uS
#define INT_PERIOD  10

// Calibration states
#define C_UDEFF   0
#define C_INIT    1
#define C_HMIN    2
#define C_GMAX    3
#define C_HMAX    4
#define C_GMIN    5
#define C_FIN     6
#define C_DONE    7


// buttons code
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// debounce time (milliseconds)
#define DEBOUNCE_TIME 200

#define tickerMax 125

#define STEP_DELAY  10
#define CALIB_SPEED 20

// Variable declarations
/******************************************************************************/
// debug enable
bool DEBUG_SERIAL;
bool DEBUG_OLED;

// motor speed -> ticks mapping
const int speed_ticks[] = {
-1, 375, 188, 125, 94, 75, 63, 54, 47, 42, 38, 34, 31, 29, 27, 6};

bool dirFlag;
bool ledFlag;

byte oldPos = 0;

// State machine vars
bool menuSelect = false;
int menu = 0;
volatile int status;

// motor parameters
volatile int actual_speed;
volatile int actual_direction;

// interrupt vars
volatile int ticks;
volatile unsigned long DEBUG_TICKS;
volatile bool running = false;

int button;
bool debounce;
unsigned long previous_time;

volatile bool MAX_FLAG;
volatile bool MIN_FLAG;
volatile long step_count;
long calibration_steps;

int ticker = 0;
int encoder_result = 0;

// Function prototypes
/******************************************************************************/
int read_buttons();
void calibrate();
void increase_speed();
void decrease_speed();
void set_speed(int speed);
void change_direction(int new_direction);
void emergency_stop();
void updateLCD();
void pciSetup(byte pin);

// Functions
/******************************************************************************/

// the setup function runs once when you press reset or power the board
void setup() {
  DEBUG_SERIAL = true;
  DEBUG_OLED   = true;
  // initialize LEDS
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  // Encoder setup
  Encoder_Init();

  // Initialize Button
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(ENCS, INPUT_PULLUP);

  // Initialize Endstops
  pinMode(EMAX, INPUT_PULLUP);
  pinMode(EMIN, INPUT_PULLUP);

  // Init Endstop interrupts http://playground.arduino.cc/Main/PinChangeInterrupt
  pciSetup(EMAX);
  pciSetup(EMIN);

  // Initialize Stepper
  pinMode(SDIR, OUTPUT);
  pinMode(SSTP, OUTPUT);
  pinMode(SEN, OUTPUT);

  digitalWrite(SEN, LOW); // motor on
  digitalWrite(SDIR, LOW);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDB, LOW);
  dirFlag = false;


  // OLED Display setup
  OLED_Init();

  // Timer stuffs http://www.lucadentella.it/en/2013/05/30/allegro-a4988-e-arduino-3/
  Timer1.initialize(INT_PERIOD); // setup for 10uS interrupts
  Timer1.attachInterrupt(timerIsr); // attach isr function

   // initial values
  actual_speed = 0;
  actual_direction = FORWARD;
  tick_count = 0;
  ticks = -1;
  debounce = false;
  running = false;
  MAX_FLAG = false;
  MIN_FLAG = false;

  digitalWrite(SDIR, actual_direction);

  DEBUG_TICKS = 50000;
  Serial.begin(115200);
  Serial.println("Debug Started:");
  status = C_UDEFF;
  calibrate();
  DEBUG_SERIAL = false;
}

// Install Pin change interrupt for a pin, can be called multiple times
void pciSetup(byte pin)
{
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// the loop function runs over and over again forever
void loop() {

  // calculate the current encoder move
  if(encoderPos - oldPos >0) encoder_result = 1;
  else if(encoderPos - oldPos <0) encoder_result = -1;
  else encoder_result = 0;
  oldPos = encoderPos;  // reset the relative pos value

  // check if debounce active
  if(debounce) {
    button = btnNONE;
    if(millis() > previous_time + DEBOUNCE_TIME) debounce = false;
  } else button = read_buttons();

  // if a button is pressed, start debounce time
  if(button != btnNONE) {
    previous_time = millis();
    debounce = true;
  }

  // switch menu selected active mode
  if(button != btnNONE){
    if(menuSelect) menuSelect = false;
    else{
      menuSelect = true;
      encoder_result = 0; // reset the encoder result
    }
  }

  if(!menuSelect){// selecting menu
    menu = menu + encoder_result;
    // loop menu around
    if(menu > MENUEND) menu = 0;
    else if(menu < 0) menu = MENUEND;
  }
  else{ // in menu selected mode
    if(menu == SPEEDMENU){ // in speed mode
      if(encoder_result == 1) increase_speed();
      else if(encoder_result == -1) decrease_speed();
    }else if(menu == DIRMENU){ // in direction mode
      if(encoder_result == 1) change_direction(FORWARD);
      else if(encoder_result == -1) change_direction(BACKWARD);
    }else if(menu == STARTMENU){ // start your engines!
      if(!running) running = true;
      else running = false;
      menuSelect = false; // get out the menu
    }
  }


  // finally update the OLED
  OLED_Update();
}

//calibrate the extents
void calibrate(){
  // Tell the user what we are doing
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,1);
  display.print("Calibrating... ");
  display.setTextSize(2);
  display.display();

  // Check that we are not on the endstops
  change_direction(FORWARD);
  while(digitalRead(EMIN)){
    // make a step
    digitalWrite(SSTP, HIGH);
    digitalWrite(SSTP, LOW);
    delay(STEP_DELAY);
  }
  change_direction(BACKWARD);
  while(digitalRead(EMAX)){
    // make a step
    digitalWrite(SSTP, HIGH);
    digitalWrite(SSTP, LOW);
    delay(STEP_DELAY);
  }

  MIN_FLAG = false;
  MAX_FLAG = false;

  // move to min endstop first
  change_direction(BACKWARD);
  set_speed(CALIB_SPEED);
  running = true;
  status = C_INIT;

  while(!MIN_FLAG && !MAX_FLAG){
    // wait
  }
  if(MAX_FLAG){
    emergency_stop();
    Serial.println("Error hit Max");
    //TODO: error checking
  }else{ // min endstop
    running = false;
  }
  status = C_HMIN;
  change_direction(FORWARD);
  while(digitalRead(EMIN)){
    // make a step
    digitalWrite(SSTP, HIGH);
    digitalWrite(SSTP, LOW);
    delay(STEP_DELAY);
  }

  // reset flags
  MAX_FLAG = false;
  MIN_FLAG = false;

  // move to max Endstop
  change_direction(FORWARD);
  set_speed(CALIB_SPEED);
  running = true;
  status = C_GMAX;

  while(!MIN_FLAG && !MAX_FLAG){
    // wait
  }
  if(MIN_FLAG){
    emergency_stop();
    Serial.println("Error hit Min");
    //TODO: error checking
  }else{ // min endstop
    running = false;
  }
  status = C_HMAX;
  display.setCursor(0,21);
  display.print("Max Hit... ");
  display.display();
  // endstop retract:
  change_direction(BACKWARD);
  while(digitalRead(EMAX)){
    // make a step
    digitalWrite(SSTP, HIGH);
    digitalWrite(SSTP, LOW);
    delay(STEP_DELAY);
  }

  MAX_FLAG = false;
  MIN_FLAG = false;
  status = C_GMIN;


  // reset step counter
  step_count = 0;

  // move to minimum Endstop
  change_direction(BACKWARD);
  set_speed(CALIB_SPEED);
  running = true;

  while(!MIN_FLAG && !MAX_FLAG){
    // wait
  }
  if(MAX_FLAG){
    emergency_stop();
    Serial.println("Error hit Max");
    //TODO: error checking
  }else{ // min endstop
    running = false;
  }
  MAX_FLAG = false;
  MIN_FLAG = false;
  change_direction(FORWARD);
  status = C_FIN;
  display.setCursor(0,41);
  display.print("Min Hit... ");
  display.display();

  delay(2000);

  // store steps counted
  calibration_steps = step_count;
  display.clearDisplay();
  display.setCursor(0,1);
  display.print("Steps: ");
  display.setCursor(0,21);
  display.print(calibration_steps);
  display.display();
  status = C_DONE;

  delay(10000);
}

// read buttons connected to a single analog pin
int read_buttons() {
 if(digitalRead(ENCS) == 1) return btnNONE;
 else return btnSELECT;
}

// increase speed if it's below the max (70)
void increase_speed() {

  if(actual_speed < MAX_SPEED) {
    actual_speed += 5;
    tick_count = 0;
    ticks = speed_ticks[actual_speed / 5];
  }
}

// decrease speed if it's above the min (0)
void decrease_speed() {

  if(actual_speed > 0) {
    actual_speed -= 5;
    tick_count = 0;
    ticks = speed_ticks[actual_speed / 5];
  }
}

// Sets speed to a particular value
void set_speed(int speed) {

  if(speed > 0 && speed < MAX_SPEED) {
    actual_speed = speed;
    tick_count = 0;
    ticks = speed_ticks[actual_speed / 5];
  }
}

// change direction if needed
void change_direction(int new_direction) {

  if(actual_direction != new_direction) {
    actual_direction = new_direction;
    digitalWrite(SDIR, actual_direction);
  }
}

// emergency stop: speed 0
void emergency_stop() {
  actual_speed = 0;
  tick_count = 0;
  ticks = speed_ticks[actual_speed / 5];
  running = false;
}




ISR (PCINT2_vect) // handle pin change interrupt for D0 to D7 here
 {
     if(digitalRead(EMAX)){ // MAX hit
       MAX_FLAG = true;
       // check that we are out of the calibration loop
       if(status == C_DONE) emergency_stop();
     }
     if(digitalRead(EMIN)){ // Min hit
       MIN_FLAG = true;
       // check that we are out of the calibration loop
       if(status == C_DONE) emergency_stop();
     }
 }
