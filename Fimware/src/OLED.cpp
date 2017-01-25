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
  display.println("Hello, world!");
  display.display();

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


  // if(menuSelect == true && menu == SPEEDMENU){ // draw filled rect if selected
  //   display.fillRect(74,0,35,16,WHITE);
  //   display.setTextColor(BLACK);
  //   display.println(actual_speed);
  //   display.setTextColor(WHITE);
  // }else if(menu == SPEEDMENU){ // draw rect if in correct menu
  //   display.drawRect(74,0,35,16,WHITE);
  //   display.println(actual_speed);
  // }else{ // just draw speed
  //   display.println(actual_speed);
  // }
  //
  // // print second line:
  // // Direction: <-->
  // display.setCursor(0,21);
  // display.print("Direction: ");
  //
  // if(menuSelect == true && menu == DIRMENU){ // draw filled rect if selected
  //   display.fillRect(5,36,45,16,WHITE);
  //   display.setTextColor(BLACK);
  //   if(actual_direction == FORWARD) display.print("-->");
  //   else display.print("<--");
  //   display.setTextColor(WHITE);
  // }else if(menu == DIRMENU){ // draw rect if in correct menu
  //   display.drawRect(5,36,45,16,WHITE);
  //   if(actual_direction == FORWARD) display.print("-->");
  //   else display.print("<--");
  // }else{ // just draw direction
  //   if(actual_direction == FORWARD) display.print("-->");
  //   else display.print("<--");
  // }
  //
  // // print third line:
  // // Start
  // display.setCursor(60,40);
  // if(running){ // draw filled rect if running
  //   display.fillRect(59,39,61,17,WHITE);
  //   display.setTextColor(BLACK);
  //   display.print("Start");
  //   display.setTextColor(WHITE);
  // }else{
  //   display.print("Start");
  // }
  // if(menu == STARTMENU){ // draw rect if in correct menu
  //   display.drawRect(58,38,62,18,WHITE);
  // }

  // print second line:
  // progress bar [#####         ]
  // 15 speed steps: 0 - 5 - 10 - ... - 70
//  lcd.setCursor(0,1);
//  lcd.print("[");
//
//  for(int i = 1; i <= 14; i++) {
//
//    if(actual_speed > (5 * i) - 1) lcd.write(byte(0));
//    else lcd.print(" ");
//  }
//
//  lcd.print("]");

  //DEBUG code
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

  // write out to the display
  display.display();
}
