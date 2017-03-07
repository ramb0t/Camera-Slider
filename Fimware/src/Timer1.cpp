#include "Timer1.h"

// Variable declarations
/*****************************************************************************/
#ifdef DEBUG
  volatile unsigned long debug_ticker;
#endif
volatile unsigned int tick_count;

// Functions:
/*****************************************************************************/
// ISR to do stepper moves
void timerIsr() {

  #ifdef DEBUG
    if(DEBUG_SERIAL) debug_ticker++;
  #endif

  // inc the step counter
  tick_count++;
  ints_step_count ++; // counts the ints for the next step

  if((ints_step_count >= ints_step) && running) {

    // make a step
    digitalWrite(SSTP, HIGH);
    digitalWrite(SSTP, LOW);

    // inc the step_counter, so we know where we are
    step_count++;

    // reset int counter
    ints_step_count = 0;
  }
}
