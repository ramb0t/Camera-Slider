#ifndef PINDEFINES_H_
#define PINDEFINES_H_

#define LEDR A1 //AVR 24 PC1
#define LEDG A2 //AVR 25 PC2
#define LEDB A3 //AVR 26 PC3

#define ENCA 2  //AVR 32 PD2
#define ENCB 3  //AVR 1  PD3
#define ENCS 4  //AVR 2  PD4

#define BTN1 5  //AVR 9  PD5

#define EMAX 6  //AVR 10   PD6
#define EMIN 7  //AVR 11 PD7

#define SDIR 8  //AVR 12 PB0
#define SSTP 9  //AVR 13 PB1
#define SEN 10  //AVR 14 PB2

#define OLED_RESET A0 //AVR 23 PC0
#define SDA A4  //AVR 27 PC4
#define SCL A5  //AVR 28 PC5

// All the wires needed for full functionality
#define DIR SDIR
#define STEP SSTP

#endif /* PINDEFINES_H_ */
