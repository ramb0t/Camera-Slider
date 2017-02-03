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
#include <EEPROM.h>
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

// interrupts per second
#define INTS_PSEC   1000000/INT_PERIOD

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
#define CALIB_SPEED 15

#define HOURS_MAX   3

// ID of the settings block
#define CONFIG_VERSION "ls1"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

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
bool itemSelect = false;
int item = 0;
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

// time variables
byte hours;
byte minutes;
byte seconds;
int  totalRunSecs;
long oldMillis;

// step calc variables
long steps_sec;
long ints_step;
long ints_step_count;

// Structs
/******************************************************************************/
// Example settings structure
struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  //int a, b;
  //char c;
  long c_steps;
  //float e[6];
} storage = {
  CONFIG_VERSION,
  // The default values
  //220, 1884,
  //'c',
  10000
  //{4.5, 5.5, 7, 8.5, 10, 12}
};

// Function prototypes
/******************************************************************************/
int read_buttons();
void calibrate();
void init_run();
void increase_speed();
void decrease_speed();
void set_speed(int speed);
void change_direction(int new_direction);
void emergency_stop();
void updateLCD();
void pciSetup(byte pin);
void inc_hours();
void dec_hours();
void inc_mins();
void dec_mins();
void inc_secs();
void dec_secs();
void loadConfig();
void saveConfig();

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

  // Load vals from EEPROM
  loadConfig();
  calibration_steps = storage.c_steps;

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
  hours = 0;
  minutes = 0;
  seconds = 10;
  totalRunSecs = 0;
  steps_sec = 0;
  ints_step = 0;
  ints_step_count = 0;


  digitalWrite(SDIR, actual_direction);

  DEBUG_TICKS = 50000;
  Serial.begin(115200);
  Serial.println("Debug Started:");
  status = C_UDEFF;
  //calibrate();
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

  // switch item selected active mode
  if(button != btnNONE){
    if(itemSelect) itemSelect = false; // deselect item
    else           itemSelect = true;  // select item
    encoder_result = 0; // reset the encoder result to prevent value jumps
  }

  if(!itemSelect){// selecting item
    if(!running) item = item + encoder_result;
    // loop item around
    if(item > ITEMEND) item = 0;
    else if(item < 0) item = ITEMEND;
  }
  else{ // in item selected mode
    if(item == SPEEDITEM){ // in speed mode
      if(encoder_result == 1) increase_speed();
      else if(encoder_result == -1) decrease_speed();
    }else if(item == DIRITEM){ // in direction mode
      if(encoder_result == 1) change_direction(FORWARD);
      else if(encoder_result == -1) change_direction(BACKWARD);
    }else if(item == STARTITEM){ // start your engines!
      if(!running){ init_run(); running = true; }
      else running = false;
      itemSelect = false; // get out the item
    }else if(item == HOUR_ITEM){ // Adjust hours
      if(encoder_result == 1)       inc_hours();
      else if(encoder_result == -1) dec_hours();
    }else if(item == MIN_ITEM){ // Adjust minutes
      if(encoder_result == 1)       inc_mins();
      else if(encoder_result == -1) dec_mins();
    }else if(item == SEC_ITEM){ // Adjust seconds
      if(encoder_result == 1)       inc_secs();
      else if(encoder_result == -1) dec_secs();
    }else if(item == CAL_ITEM){ // Calibrate
      calibrate();
      itemSelect = false;
    }else if(item == FRUN_ITEM){ // Free Run


    }
  }

  // check seconds
  if(running && (millis() - oldMillis) > 1000){
    dec_secs();
    if(hours == 0 && minutes == 0 && seconds == 0){ // end run
      running = false;
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

  delay(1000);

  // store steps counted
  calibration_steps = step_count;
  storage.c_steps = calibration_steps;
  saveConfig(); // save to EEPROM
  status = C_DONE;
}

// inits the run based on time
void init_run(){
  // calculate ticks value for time
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

  while(!MIN_FLAG && !MAX_FLAG){
    // wait
  }
  if(MAX_FLAG){
    emergency_stop();
    //TODO: error checking
  }else{ // min endstop
    running = false;
  }
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

  // calculate the timer ticks..
  // Work out total seconds
  totalRunSecs = hours*3600;
  totalRunSecs += minutes*60;
  totalRunSecs += seconds;
  // work out steps / second using calibration steps
  steps_sec = calibration_steps/totalRunSecs;
  // work out num interrupt ticks per step
  ints_step = INTS_PSEC/steps_sec;
  if(DEBUG_SERIAL){
    Serial.print("Total Secs: ");
    Serial.println(totalRunSecs);
    Serial.print("Calibration Steps: ");
    Serial.println(calibration_steps);
    Serial.print("Steps per Sec: ");
    Serial.println(steps_sec);
    Serial.print("Interrupts per Step: ");
    Serial.println(ints_step);
  }

  oldMillis = millis();

  // start run
  //running = true;
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


// Time adjustments:
void inc_hours(){
  hours++;
  if(hours > HOURS_MAX) hours = 0;
}
void dec_hours(){
  if(hours > 0) hours--;
  else hours = HOURS_MAX;
}

void inc_mins(){
  minutes++;
  if(minutes > 59){ minutes = 0; inc_hours(); }
}
void dec_mins(){
  if(minutes > 0) minutes--;
  else { minutes = 59; dec_hours(); }
}

void inc_secs(){
  seconds++;
  if(seconds > 59){ seconds = 0; inc_mins(); }
}
void dec_secs(){
  if(seconds > 0) seconds--;
  else { seconds = 59; dec_mins(); }
}

// EEPROM stuffs
void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t=0; t<sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
}

// ISR for pin changes
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
