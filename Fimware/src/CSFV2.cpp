/*

 */

#include <Arduino.h>
//#include "A4988.h"
#include <TimerOne.h>
// #include <SPI.h>
// #include <Wire.h>
#include <Adafruit_GFX.h>
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
#define MICROSTEPS 16

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
// Running states
#define S_SLEEP   8
#define S_RUN     9
#define S_ERROR   10

// buttons code
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// debounce time (milliseconds)
#define DEBOUNCE_TIME 150

#define tickerMax 125

#define STEP_DELAY  16/MICROSTEPS
#define CALIB_STEPS_SEC 0.75*STEPS_REV // Requested RPS * Steps per Rev
#define CALIB_SPEED INTS_PSEC/(100*MICROSTEPS)  //Ints per second / steps per second

#define HOME_SPEED INTS_PSEC/(700*MICROSTEPS)

#define STEPS_OFF_ENDSTOP 40*MICROSTEPS  // sets the number of steps to move off an endstop..

#define HOURS_MAX   3
#define MIN_RUN_SECS 10

// ID of the settings block
#define CONFIG_VERSION "ls1"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// Variable declarations
/******************************************************************************/
// debug enable
bool DEBUG_SERIAL;

int actual_direction;
int enc_dir = -1; // sets fwd or reverse for the encoder

byte oldPos = 0;

// State machine vars
bool itemSelect = false;
int item = 0;
volatile int status;

// interrupt vars
volatile int ticks;
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
byte o_hours;
byte o_minutes;
byte o_seconds;
int  totalRunSecs;
long oldMillis;

// step calc variables
long steps_sec;
long ints_step;
long ints_step_count;
byte speed;

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
void read_buttons();
void check_encoder();
void calibrate();
void init_run();
void end_run();
void home_min();
void home_max();
void disable_motor();
void enable_motor();
void change_speed(int new_speed);
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

  #ifdef DEBUG
    Serial.begin(115200);
    DEBUG_SERIAL = false;
  #else
  DEBUG_SERIAL = false;
  #endif
  DEBUG_PRINT("Debug Started:");
  status = C_UDEFF;
  //calibrate();

  DEBUG_PRINT("Setting up IO... ");
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

  digitalWrite(SEN, HIGH); // motor off
  digitalWrite(SDIR, LOW);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDB, LOW);

  DEBUG_PRINT("Getting vals from EEPROM... ");
  // Load vals from EEPROM
  loadConfig();
  calibration_steps = storage.c_steps;

  DEBUG_PRINT("Setting up OLED... ");
  // OLED Display setup
  OLED_Init();

  DEBUG_PRINT("Init Vars... ");
   // initial values
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
  actual_direction = FORWARD;
  speed = 100;


  digitalWrite(SDIR, actual_direction);

  DEBUG_PRINT("Activate Timer1 ISR... ");
  // Timer stuffs http://www.lucadentella.it/en/2013/05/30/allegro-a4988-e-arduino-3/
  Timer1.initialize(INT_PERIOD); // setup for 10uS interrupts
  Timer1.attachInterrupt(timerIsr); // attach isr function

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
  check_encoder();

  // Check if the button has been pressed, debounce etc.
  read_buttons();

  if(!itemSelect){// selecting item
    if(!running) item = item + encoder_result;
    // loop item around
    if(item > ITEMEND) item = 0;
    else if(item < 0) item = ITEMEND;
  }
  else{ // in item selected mode
    if(item == SPEEDITEM){ // in speed mode
      change_speed(encoder_result);
    }else if(item == DIRITEM){ // in direction mode
      if(encoder_result == 1) change_direction(FORWARD);
      else if(encoder_result == -1) change_direction(BACKWARD);
    }else if(item == STARTITEM){ // start your engines!
      if(!running){ init_run(); }
      else emergency_stop();
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

  // check seconds when running
  if((millis() - oldMillis) > 1000){
    oldMillis = millis();
    if(running) dec_secs();
    if(hours == 0 && minutes == 0 && seconds == 0){ // end run
      end_run();
    }
  }

  // check if there was an error
  if(status == S_ERROR){
    disable_motor();
    if(MIN_FLAG){
      OLED_Clear();
      OLED_Print("Min Hit!", 0, 1, 2);
      delay(1000);
    }
    if(MAX_FLAG){
      OLED_Clear();
      OLED_Print("MAX Hit!", 0, 1, 2);
      delay(3000);
    }
    // Reset flags
    MIN_FLAG = false;
    MAX_FLAG = false;

    // reset time
    seconds = o_seconds;
    minutes = o_minutes;
    hours = o_hours;

    status = S_SLEEP;

  }

  // Debug OUTPUT
  #ifdef DEBUG
    if(debug_ticker >= DEBUG_TICKS){
      debug_ticker = 0; //reset the ticker
      Serial.print("DEBUG: ");
      Serial.print("Time ");
      Serial.print(millis());
      Serial.print("Run ");
      Serial.print(running);
      Serial.print(", Spd ");
      Serial.print(ints_step);
      Serial.print(", Dir ");
      Serial.print(actual_direction);
      Serial.print(", MaxF ");
      Serial.print(MAX_FLAG);
      Serial.print(", MinF ");
      Serial.print(MIN_FLAG);
      Serial.print(", Stat ");
      Serial.println(status);
    }
  #endif

  // finally update the OLED
  OLED_Update();
}

//calibrate the extents
void calibrate(){
  status = C_INIT;

  // Tell the user what we are doing
  OLED_Clear();
  OLED_Print("Calibrating...");

  enable_motor();
  //Home min first
  OLED_Print("Home Min..", 0, 21, 2);
  status = C_HMIN;
  home_min();

  // Do the Calibrating

  // reset step counter
  step_count = 0;
  // set speed
  ints_step = CALIB_SPEED;
  // set direction
  change_direction(FORWARD);
  running = true;
  //reset the flags
  MIN_FLAG = false;
  MAX_FLAG = false;
  status = C_GMAX;

  // wait to hit an endstop
  while(!MIN_FLAG && !MAX_FLAG){
    // wait
    OLED_Clear(false);
    OLED_Print("Calibrating...", 0, 1, 1, false);
    OLED_Print("Counting..", 0, 21, 2, false);
    char buf[50];
    ltoa(step_count, buf, 10);  // 10 is the base value not the size - look up ltoa for avr
     // use arduino string functions because lazy..
    OLED_Print(buf, 0, 41);
  }
  running = false;
  if(MIN_FLAG){
    OLED_Print("Error hit Min!");
    DEBUG_PRINT("Error hit Min");
    status = C_UDEFF;
    delay(1000);
    return; // exit...
  }else{ // max endstop

  }

  // hit the max
  status = C_HMAX;
  OLED_Clear();
  OLED_Print("Calibrating...");
  OLED_Print("Max Hit... ", 0, 41, 2);

  status = C_FIN;

  // store steps counted
  calibration_steps = step_count;
  calibration_steps = calibration_steps/MICROSTEPS; // take out the MICROSTEPS factor
  storage.c_steps = calibration_steps;
  saveConfig(); // save to EEPROM

  //home min again
  status = C_GMIN;
  home_min();

  // Finally done!
  status = C_DONE;
  disable_motor();
  OLED_Clear();
  OLED_Print("Done!", 0 , 1 , 2);
  delay(1000);
}



// inits the run based on time
void init_run(){
  enable_motor();

  OLED_Clear();
  OLED_Print("Home Min..", 0, 21, 2);
  // go to the min endstop
  home_min();
  // calculate the timer ticks..
  // Work out total seconds
  totalRunSecs = hours*3600;
  totalRunSecs += minutes*60;
  totalRunSecs += seconds;
  if(totalRunSecs < MIN_RUN_SECS){ totalRunSecs = MIN_RUN_SECS;} // error checking
  // work out steps / second using calibration steps
  steps_sec = calibration_steps * MICROSTEPS; // cal steps * current MICROSTEPS
  steps_sec = steps_sec - (2*STEPS_OFF_ENDSTOP); // take endstop offsets off
  steps_sec = steps_sec/totalRunSecs; // divide by the total time

  // work out num interrupt ticks per step (SPEED)
  ints_step = INTS_PSEC/steps_sec;

  //debug
  #ifdef DEBUG
    Serial.print("Total Secs: ");
    Serial.println(totalRunSecs);
    Serial.print("Calibration Steps: ");
    Serial.println(calibration_steps);
    Serial.print("Steps per Sec: ");
    Serial.println(steps_sec);
    Serial.print("Interrupts per Step: ");
    Serial.println(ints_step);
  #endif

  oldMillis = millis();

  // store the original time vars
  o_seconds = seconds;
  o_minutes = minutes;
  o_hours = hours;

  status = S_RUN;
  // start run
  running = true;
}


// reinit the slider at the end of a run
void end_run(){
  running = false;
  status = S_SLEEP;
  OLED_Clear();
  OLED_Print("Done!", 0, 1, 2);
  OLED_Print("Home Min..", 0, 21, 2);
  // go to the min endstop
  home_min();

  running = false;
  seconds = o_seconds;
  minutes = o_minutes;
  hours = o_hours;
  item = STARTITEM; // default screen
  disable_motor();
}

// Home to the min Endstop
void home_min(){
  // reset flags
  MAX_FLAG = false;
  MIN_FLAG = false;

  // move to min endstop first
  change_direction(BACKWARD);
  ints_step = HOME_SPEED;
  running = true;

  while(!MIN_FLAG){
    // wait
    if(MAX_FLAG){
      //TODO: error checking
    }
  }

  // min endstop hit
  running = false;

  // move off endstop
  change_direction(FORWARD);
  step_count = 0;
  running = true;
  while(step_count < STEPS_OFF_ENDSTOP){
    //wait...
  }
  running = false;

  // reset flags
  MAX_FLAG = false;
  MIN_FLAG = false;

}

// Home to the max Endstop
void home_max(){
  // reset flags
  MAX_FLAG = false;
  MIN_FLAG = false;

  // move to min endstop first
  change_direction(FORWARD);
  ints_step = HOME_SPEED;
  running = true;

  while(!MAX_FLAG){
    // wait
    if(MIN_FLAG){
      //TODO: error checking
    }
  }

  // max endstop, move off:
  running = false;
  step_count = 0;
  running = true;
  while(step_count < STEPS_OFF_ENDSTOP){
    //wait...
  }
  running = false;

  // reset flags
  MAX_FLAG = false;
  MIN_FLAG = false;
}

void disable_motor(){
  digitalWrite(SEN, HIGH); // motor off
}

void enable_motor(){
  digitalWrite(SEN, LOW); // motor on
}

// read buttons and debounce
void read_buttons() {
  // check if debounce active
  if(debounce) {
    button = btnNONE;
    if(millis() > previous_time + DEBOUNCE_TIME) debounce = false;
  }
  else if(digitalRead(ENCS) == 1) button = btnNONE;
  else button = btnSELECT;

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
}

//check if anything has happened with the encoder
void check_encoder(){
  if(encoderPos - oldPos >0) encoder_result = 1*enc_dir;
  else if(encoderPos - oldPos <0) encoder_result = -1*enc_dir;
  else encoder_result = 0;
  oldPos = encoderPos;  // reset the relative pos value
}

// Change the speed
void change_speed(int spdChng){
  if (spdChng > 0 && speed < 100){ // inc speed, watch out for max 100%
    speed++;
  }else if(spdChng < 0 && speed > 0){ // dec speed, watch out for min 0%
    speed--;
  }
}

// change direction if needed
void change_direction(int new_direction) {

  if(actual_direction != new_direction) {
    actual_direction = new_direction;
    digitalWrite(SDIR, actual_direction);
  }
}

// emergency stop:
void emergency_stop() {
  disable_motor();
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
     }
     if(digitalRead(EMIN)){ // Min hit
       MIN_FLAG = true;
     }
     if(MIN_FLAG || MAX_FLAG){
       if(status == S_RUN || status == C_GMAX){ // we are running and hit an endstop
         running = false;
         status = S_ERROR;
       }
     }
 }
