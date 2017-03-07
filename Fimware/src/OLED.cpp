#include "OLED.h"

// Class defines
/******************************************************************************/
Adafruit_SSD1306 display(OLED_RESET);

void OLED_Init(){
  // OLED Display setup
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done

  //Set the font
  //display.setFont(&FreeMono9pt7b);

  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,12);
  //display.println("Hello, world!");
  //display.display();

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  // Clear the buffer.
  display.clearDisplay();
  display.drawBitmap(0,0,logo,128,64,WHITE);
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

}

// Update OLED
void OLED_Update() {
  display.clearDisplay();
  display.setTextSize(2);

  //Line Origin vars
  int X;
  int Y;

  if(running){ // Running page
    // print first line:
    X = 0; Y = 0;
    // Time: H.M.S
    display.setCursor(X,Y);
    display.print("Running");

    // print second line:
    // H:M:S
    X = 0; Y = 20;
    display.setCursor(X,Y);
    display.print(hours);
    display.print(":");
    display.print(minutes);
    display.print(":");
    display.print(seconds);

    // print third line:
    // Stop
    X = 0; Y = 40;
    display.drawRect(X,Y,55,20,WHITE);
    display.setCursor(X+3,Y+3);
    display.print("STOP");

  }// end running page
  // Time Page:
  else if(item == HOUR_ITEM || item == MIN_ITEM || item == SEC_ITEM || item == STARTITEM){
    // print first line:
    X = 0; Y = 0;
    // Time: H.M.S
    display.setCursor(X,Y);
    display.print("Time:H.M.S");

    // print second line:
    // |x| |x| |x| |GO|
    // Hours:
    X = 0; Y = 20;
    if(itemSelect && item == HOUR_ITEM){    //Selected > Draw Filled Rect
      display.fillRect(X,Y,16,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.setTextColor(BLACK);
      display.print(hours);
      display.setTextColor(WHITE);
    }else if(item == HOUR_ITEM){            //Hover    > Draw Rect
      display.drawRect(X,Y,16,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.print(hours);
    }else{                                  // Just Print Value
      display.setCursor(X+3,Y+3);
      display.print(hours);
    }
    // Minutes:
    X = X + 20;
    if(itemSelect && item == MIN_ITEM){     //Selected > Draw Filled Rect
      display.fillRect(X,Y,28,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.setTextColor(BLACK);
      display.print(minutes);
      display.setTextColor(WHITE);
    }else if(item == MIN_ITEM){             //Hover    > Draw Rect
      display.drawRect(X,Y,28,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.print(minutes);
    }else{                                  // Just Print Value
      display.setCursor(X+3,Y+3);
      display.print(minutes);
    }
    // Seconds:
    X = X + 32;
    if(itemSelect && item == SEC_ITEM){     //Selected > Draw Filled Rect
      display.fillRect(X,Y,28,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.setTextColor(BLACK);
      display.print(seconds);
      display.setTextColor(WHITE);
    }else if(item == SEC_ITEM){             //Hover    > Draw Rect
      display.drawRect(X,Y,28,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.print(seconds);
    }else{                                  // Just Print Value
      display.setCursor(X+3,Y+3);
      display.print(seconds);
    }
    // Go!:
    X = X + 34;
    if(itemSelect && item == STARTITEM){    //Selected > Draw Filled Rect
      display.fillRect(X,Y,37,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.setTextColor(BLACK);
      display.print("GO!");
      display.setTextColor(WHITE);
    }else if(item == STARTITEM){            //Hover    > Draw Rect
      display.drawRect(X,Y,37,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.print("GO!");
    }else{                                  // Just Print Value
      display.setCursor(X+3,Y+3);
      display.print("GO!");
    }

    // print third line:
    // calibrate
    X = 0; Y = 44;
    display.setCursor(X, Y);
    display.setTextSize(1);
    display.print("Calibrate ");
    display.drawChar(X+60, Y, 0x19, WHITE, BLACK, 1);
  } // end first page
  // Second Page: Calibrate
  else if(item == CAL_ITEM){
    // print first line:
    display.setTextSize(2);
    X = 0; Y = 0;
    // |Calibrate|
    if(itemSelect && item == CAL_ITEM){    //Selected > Draw Filled Rect
      display.fillRect(X,Y,127,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.setTextColor(BLACK);
      display.print("Calibrate");
      display.setTextColor(WHITE);
    }else if(item == CAL_ITEM){            //Hover    > Draw Rect
      display.drawRect(X,Y,127,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.print("Calibrate");
    }else{                                  // Just Print Value
      display.setCursor(X+3,Y+3);
      display.print("Calibrate");
    }

    // print second line:
    X = 0; Y = 22;
    // Steps:
    display.setCursor(X, Y);
    display.print("Stps:");
    display.print(calibration_steps);

    // print third line:
    // time
    X = 0; Y = 42;
    display.setCursor(X, Y);
    display.setTextSize(1);
    display.print("Time ");
    display.drawChar(X+60, Y, 0x19, WHITE, BLACK, 1);
  } // end second page
  else if(item == FRUN_ITEM){
    // print first line:
    display.setTextSize(2);
    X = 0; Y = 0;
    // |Calibrate|
    if(itemSelect && item == CAL_ITEM){    //Selected > Draw Filled Rect
      display.fillRect(X,Y,127,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.setTextColor(BLACK);
      display.print("Free Run");
      display.setTextColor(WHITE);
    }else if(item == CAL_ITEM){            //Hover    > Draw Rect
      display.drawRect(X,Y,127,20,WHITE);
      display.setCursor(X+3,Y+3);
      display.print("Free Run");
    }else{                                  // Just Print Value
      display.setCursor(X+3,Y+3);
      display.print("Free Run");
    }

    // // print second line:
    // X = 0; Y = 22;
    // // Steps:
    // display.setCursor(X, Y);
    // display.print("Stps:");
    // display.print(calibration_steps);
    //
    // // print third line:
    // // time
    // X = 0; Y = 42;
    // display.setCursor(X, Y);
    // display.setTextSize(1);
    // display.print("Time ");
    // display.drawChar(X+60, Y, 0x19, WHITE, BLACK, 1);
  } // end third page


  //DEBUG code
  #ifdef DEBUG
    if(DEBUG_OLED){
      //small little debug icon
      if(digitalRead(ENCS) == 0){
        display.fillCircle(119,2,2,WHITE);
      }
      if(running){
        display.fillCircle(122,2,2,WHITE);
      }
      // debug encoder pos
      display.setTextSize(1);
      display.setCursor(0,55);
      display.print(encoderPos);
      display.print(" ");
      display.print(ticks);
      display.print(" ");
      display.print(tick_count);
      display.print(" ");
      display.print(actual_direction);
      display.print(" ");
      if (digitalRead(EMAX)) {
        display.print("1");
      }else{
        display.print("0");
      }
      display.print(" ");
      if (digitalRead(EMIN)) {
        display.print("1");
      }else{
        display.print("0");
      }
    }
  #endif

  // write out to the display
  display.display();
}
