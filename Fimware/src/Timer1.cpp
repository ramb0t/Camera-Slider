#include "Timer1.h"

// Variable declarations
/*****************************************************************************/
unsigned long debug_ticker;
volatile unsigned int tick_count;

// Functions:
/*****************************************************************************/
// ISR to do stepper moves
void timerIsr() {

  if(DEBUG_SERIAL){
    debug_ticker++;
    if(debug_ticker >= DEBUG_TICKS){
      debug_ticker = 0;
      Serial.print("DEBUG: ");
      Serial.print("Run ");
      Serial.print(running);
      Serial.print(", Spd ");
      Serial.print(actual_speed);
      Serial.print(", Dir ");
      Serial.print(actual_direction);
      Serial.print(", MaxF ");
      Serial.print(MAX_FLAG);
      Serial.print(", MinF ");
      Serial.print(MIN_FLAG);
      Serial.print(", Stat ");
      Serial.println(status);

    }
  }

  //Don't even bother
  if(actual_speed == 0) return;

  // inc the step counter
  tick_count++;

  // if we have hit the limit and we should be running
  if((tick_count >= ticks) && running) {

    // make a step
    digitalWrite(SSTP, HIGH);
    digitalWrite(SSTP, LOW);

    // inc the step_counter
    step_count++;

    // reset tick counter
    tick_count = 0;
  }
}
