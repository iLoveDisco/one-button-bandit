/********************************************************************
 * FileName:        IR_Sensor.c
 * Processor:       PIC18F4520
 * Compiler:        MPLAB C18 v.3.06
 *
 * File responsibilities:
 *  - Senses and tracks coin input (uses UART)
 *  - PWM buzzer
 *  - LCD Display
 *  - Spins DC Motor
 *
 *       Author               Date              Comment
 *       Eric Tu              2/4/2019          Init
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/ 

/**  Header Files **************************************************/
#include <p18f4520.h>
#include <stdio.h>
#include <adc.h>
#include <pwm.h>
#include <usart.h>
#include <delays.h>
#include <stdlib.h>
#include <timers.h>
/** Configuration Bits *********************************************/
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOREN = OFF
#pragma config XINST = OFF

/** Define Constants Here ******************************************/
#define THRESH 900
#define HIGH 1
#define LOW 0
#define DUTY_CYCLE 700

#define PRESSED 0
#define TRUE 1
#define FALSE 0

#define TIMER_TICKS 141
/** Local Function Prototypes **************************************/
void low_isr(void);
void high_isr(void);
void handleCoinSensing(void);
void handleMotorDriving(void);
void handlePushButton(void);
void handleReel(char, char, char);
void sendByte(char);
void handlePayout(char);
void dispenseCoins(char);
void handleCoinOutput(void);


/** Declare Interrupt Vector Sections ****************************/
#pragma code high_vector=0x08

void interrupt_at_high_vector(void) {
  _asm goto high_isr _endasm
}

#pragma code low_vector=0x18

void interrupt_at_low_vector(void) {
  _asm goto low_isr _endasm
}

/** Global Variables ***********************************************/
int coinInputReading = 0;
int coinOutputReading = 0;
int credit = 0;
int coinOutCount = 0;
int coinOutGoal = 2;
char recentInputState = LOW;
char recentOutputState = LOW;

char recentButtonState = !PRESSED;

char wheel1Index = 0;
char wheel2Index = 0;
char wheel3Index = 0;

char wheel1[14] = {'7','W','B','C','O','B','G','O','W','O','G','W','B','G'};
char wheel2[14] = {'7','O','W','G','W','C','G','B','O','B','W','O','G','B'};
char wheel3[14] = {'7','G','W','G','B','W','O','B','O','C','W','G','B','O'};

char slotsAreBusy = FALSE;
char recentButtonState2 = !PRESSED;
/*******************************************************************
 * Function:        void main(void)
 ********************************************************************/
#pragma code

void main(void) {
  // Set the clock to 8 MHz
  OSCCONbits.IRCF2 = 1;
  OSCCONbits.IRCF1 = 1;
  OSCCONbits.IRCF0 = 1;

  // Pin IO Setup
  OpenADC(ADC_FOSC_8 & ADC_RIGHT_JUST & ADC_12_TAD,
          ADC_CH0 & ADC_INT_OFF & ADC_REF_VDD_VSS,
          0x0B); // Four analog pins
  TRISA = 0xFF; // All of PORTA input
  TRISB = 0xFF; // All of PORTB input
  TRISC = 0x80; // RX needs to be input. TX needs to be output
  TRISD = 0x01; // All of PORTD output
  PORTC = 0x00; // Turn off all 8 Port C LEDs

  Delay10KTCYx(800);

  OpenUSART(USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE
          & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 25);
  OpenTimer2(TIMER_INT_OFF & T2_PS_1_16);
  OpenPWM2(TIMER_TICKS);
  SetDCPWM2(0);
  // Interrupt setup
  RCONbits.IPEN = 1; // Put the interrupts into Priority Mode
  INTCONbits.GIEH = 1; // Turn on high priority interrupts

  PIE1bits.RCIE = 1; // Turn on RX interrupts
  IPR1bits.RCIP = 1; // Set RX ints to high prio
  while (1) {
    handlePushButton();
    handleCoinSensing();
    handleCoinOutput();
  }
}

/*****************************************************************
 * Function:        void high_isr(void)
 ******************************************************************/
#pragma interrupt high_isr

void high_isr(void) {
  if (PIR1bits.RCIF) {
    char byteReceived = RCREG;
    PIR1bits.RCIF = 0;
    if (byteReceived = 'D')
      slotsAreBusy = FALSE;
  }
}

/******************************************************************
 * Function:        void low_isr(void)
 ********************************************************************/
#pragma interruptlow low_isr

void low_isr(void) {
  // Add code here for the low priority Interrupt Service Routine (ISR)
}

#pragma code

/*****************************************************************
 * Function:			void handleCoinSensing(void)
 * Input Variables:	none
 * Output Return:	none
 * Overview:			Use a comment block like this before functions
 ******************************************************************/
void handleCoinSensing() {
  // RA0 coin input code
  SetChanADC(ADC_CH0);
  ConvertADC();
  while (BusyADC());
  coinInputReading = ReadADC();

  if (coinInputReading > THRESH && recentInputState == LOW) {
    Delay1KTCYx(30); // Debounce
    recentInputState = HIGH;
    credit++;
  } else if (coinInputReading < THRESH && recentInputState == HIGH) {
    recentInputState = LOW;
  }

  // RA1 coin output code
  SetChanADC(ADC_CH1);
  ConvertADC();
  while (BusyADC());
  coinOutputReading = ReadADC();
  if (coinOutputReading > THRESH && recentOutputState == LOW) {
    Delay1KTCYx(30);// Debounce
    recentOutputState = HIGH;
    coinOutCount++;
  } else if (coinOutputReading < THRESH && recentOutputState == HIGH) {
    recentOutputState = LOW;
  }
}

void handlePushButton() {
  if (credit > 0) {
    PORTCbits.RC3 = 1;
  } else {
    PORTCbits.RC3 = 0;
  }

  if (PORTBbits.RB0 == PRESSED && recentButtonState == !PRESSED && credit > 0) {
    recentButtonState = PRESSED;
    credit--;
    Delay1KTCYx(30);// Debounce
    // send 3 random nums to other pic
    // handleReel(rand() % 14, rand() % 14, rand() % 14);
    handleReel(1, 2, 2);
    slotsAreBusy = TRUE;
  } else if (PORTBbits.RB0 == !PRESSED) {
    recentButtonState = !PRESSED;
  }

  if (PORTDbits.RD0 == PRESSED && recentButtonState == !PRESSED && credit > 0) {
    recentButtonState2 = PRESSED;
    credit--;
    Delay1KTCYx(30);
    handleReel(14 - wheel1Index, 14 - wheel2Index, 14 - wheel3Index);
    slotsAreBusy = TRUE;
  } else if (PORTDbits.RD0 == !PRESSED) {
    recentButtonState2 = !PRESSED;
  }
}

void handleReel(char rand1, char rand2, char rand3) {
  // increments wheel index to the correct position
  wheel1Index = (wheel1Index + rand1) % 14;
  wheel2Index = (wheel2Index + rand2) % 14;
  wheel3Index = (wheel3Index + rand3) % 14;

  // tells other pic to spin in a multiple of the num sent
  sendByte(rand1);
  sendByte(rand2);
  sendByte(rand3);
  Delay10KTCYx(200);// Delay 1 second for debounce
  while(slotsAreBusy); // Delay until slotsAreBusy = false. Will get triggered by RX int
  if (wheel1[wheel1Index] == wheel2[wheel2Index] && 
      wheel2[wheel2Index] == wheel3[wheel3Index]) {
    handlePayout(wheel1[wheel1Index]);
  }
}

void handlePayout (char sticker) {
  switch(sticker) {
    case 'C':
      dispenseCoins(5);
      break;
    case '7':
      dispenseCoins(20);
      break;
    case 'W':
      credit++;
      break;
    case 'B':
      dispenseCoins(2);
      break;
    case 'O':
      dispenseCoins(2);
      break;
    case 'G':
      dispenseCoins(2);
      break;
  }
}

void dispenseCoins (char amount) {
  Delay10KTCYx(200);
  if (amount == 20) {
    PORTCbits.RC0 = 1;
    Delay10KTCYx(200);
    PORTCbits.RC0 = 0;
  }
  SetDCPWM2(DUTY_CYCLE);
  coinOutGoal = amount;
}

void handleCoinOutput() {
  if (coinOutCount >= coinOutGoal) {
    SetDCPWM2(0);
    coinOutCount = 0;
    coinOutGoal = 2;
  }
}

void sendByte(char byte){
  printf("%c", byte);
}
