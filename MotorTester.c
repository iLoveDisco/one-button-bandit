/********************************************************************
 * FileName:        (change filename of template).c
 * Processor:       PIC18F4520
 * Compiler:        MPLAB C18 v.3.06
 *
 * This file does the following....
 *
 *
 *       Author               Date              Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**  Header Files **************************************************/
#include <p18f4520.h>
#include <stdio.h>
#include <adc.h>
#include <timers.h>
#include <usart.h>


/** Configuration Bits *********************************************/
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOREN = OFF
#pragma config XINST = OFF

/** Define Constants Here ******************************************/
#define STEP1 0b00001010
#define STEP2 0b00001001
#define STEP3 0b00000101
#define STEP4 0b00000110

#define PRESSED 0
#define UNPRESSED 1
#define TRUE 1
#define FALSE 0
#define ONE_REV 332

/** Local Function Prototypes **************************************/
void low_isr(void);
void high_isr(void);

void stepMotor1(void);
void stepMotor2(void);
void stepMotor3(void);

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
char slotsAreFree = TRUE;
int byteReceived;

int motor1Count = 0;
int motor2Count = 0;
int motor3Count = 0;

// every count is 1.08 deg on big
int motor1CountGoal = ONE_REV;
int motor2CountGoal = ONE_REV;
int motor3CountGoal = ONE_REV;

char motor1RecentState = STEP1;
char motor2RecentState = STEP1;
char motor3RecentState = STEP1;

char rxCount = 1;

char debugCount = 0;
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

  TRISA = 0x00; // All of PORTA output
  TRISB = 0xFF; // All of PORTB input
  TRISC = 0x80; // All of PORTC output. RX needs to be an input
  TRISD = 0x00; // All of PORTD output. D2 needs to be an input
  
  PORTC = 0x00;
  PORTD = 0x00;
  PORTA = 0x00;

  OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_1);
  WriteTimer0(65530);
  OpenUSART(USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 25);
  // Interrupt setup
  RCONbits.IPEN = 1; // Put the interrupts into Priority Mode
  INTCONbits.GIEH = 1; // Turn on high priority interrupts
  INTCONbits.GIEL = 1; // Turn on low prio interrupts

  PIE1bits.RCIE = 1; // Turn on RX interrupts
  IPR1bits.RCIP = 0; // Set RX ints to low prio

  INTCONbits.TMR0IE = 1; // Turn on Timer0 interupts
  INTCON2bits.TMR0IP = 1; // Set Timer0 ints to high prio

  while (1) {
  }
}

/*****************************************************************
 * High prio interrupt is used for 
 ******************************************************************/
#pragma interrupt high_isr

void high_isr(void) {
  if (INTCONbits.TMR0IF && !slotsAreFree) {
    // Count goal has to be a multiple of 4
    char m1IsBusy = motor1Count < motor1CountGoal;
    char m2IsBusy = motor2Count < motor2CountGoal;
    char m3IsBusy = motor3Count < motor3CountGoal;
    INTCONbits.TMR0IF = 0; // Clear interrupt flag for timer 0
    // Each motorCount is eqv to a 1.08 degree step on BIG
    if (m1IsBusy)
      stepMotor1();
    if (m2IsBusy)
      stepMotor2();
    if (m3IsBusy)
      stepMotor3();

    if (!m1IsBusy && !m2IsBusy && !m3IsBusy) {
      slotsAreFree = TRUE;
      Delay10KTCYx(400);
      
      motor1Count = 0;
      motor2Count = 0;
      motor3Count = 0;
      printf("%c", 'D');
      debugCount++;
      PORTA = 0;
      PORTD = 0;
      PORTC &= 0b11110000;
    }

  }
  INTCONbits.TMR0IF = 0; // Clear interrupt flag for timer 0
  // INT_MAX = 65536
  // WriteTimer0(33333);
  // WriteTimer0(55536);   // 1 rotation  every 1 seconds
  WriteTimer0(59512); // 1 wheel rotations every 1 second
  // WriteTimer0(60536);   // 2 rotations every 1 seconds
  // WriteTimer0(62459); // 3.25 rotations every 1 seconds
}

/******************************************************************
 * Function:        void low_isr(void)
 ********************************************************************/
#pragma interruptlow low_isr

void low_isr(void) {
  // UART
  if (PIR1bits.RCIF) {
    byteReceived = RCREG;// always a num from 0 - 13
    PIR1bits.RCIF = 0;
    switch (rxCount) {
      case 1:
        motor1CountGoal = ONE_REV - byteReceived * 23 - byteReceived / 2;
        rxCount = 2;
        break;
      case 2:
        motor2CountGoal = ONE_REV * 2 - byteReceived * 23 - byteReceived / 2 + 4;
        rxCount = 3;
        break;
      case 3:
        motor3CountGoal = ONE_REV * 3 - byteReceived * 23 - byteReceived / 2 - 2;
        rxCount = 1;
        slotsAreFree = FALSE;
        break;
    }
  }
}

#pragma code
/**
 * Each call causes motor to step 4 times 
 * (4.32 deg on big) 4 * 1.8 * 0.6
 * */
void stepMotor1() {
  switch (motor1RecentState) {
    case STEP1:
      motor1RecentState = STEP2;
      break;
    case STEP2:
      motor1RecentState = STEP3;
      break;
    case STEP3:
      motor1RecentState = STEP4;
      break;
    case STEP4:
      motor1RecentState = STEP1;
      break;
    default:
      motor1RecentState = STEP1;
      break;
  }
  PORTD = motor1RecentState;
  motor1Count++;
}
void stepMotor2() {
  switch (motor2RecentState) {
    case STEP1:
      motor2RecentState = STEP2;
      break;
    case STEP2:
      motor2RecentState = STEP3;
      break;
    case STEP3:
      motor2RecentState = STEP4;
      break;
    case STEP4:
      motor2RecentState = STEP1;
      break;
    default:
      motor2RecentState = STEP1;
      break;
  }
  PORTC = motor2RecentState;
  motor2Count++;
}
void stepMotor3() {
  switch (motor3RecentState) {
    case STEP1:
      motor3RecentState = STEP2;
      break;
    case STEP2:
      motor3RecentState = STEP3;
      break;
    case STEP3:
      motor3RecentState = STEP4;
      break;
    case STEP4:
      motor3RecentState = STEP1;
      break;
    default:
      motor3RecentState = STEP1;
      break;
  }
  PORTA = motor3RecentState;
  motor3Count++;
}

