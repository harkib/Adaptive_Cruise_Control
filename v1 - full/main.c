#include <stdint.h>                         // Library of Standard Integer Types
#include <stdbool.h>                        // Library of Standard Boolean Types
#include <stdlib.h>                         // Library of Standard Datatype Conversions
#include "tm4c123gh6pm.h"                   // Definitions for interrupt and register assignments on Tiva C
#include "inc/hw_memmap.h"                  // Macros defining the memory map of the Tiva C Series device
#include "inc/hw_types.h"                   // Defines common types and macros
#include "inc/hw_timer.h"                   // Defines and macros used when accessing the timer
#include "inc/hw_gpio.h"                    // Defines Macros for GPIO hardware
#include "driverlib/debug.h"                // Macros for assisting debug of the driver library
#include "driverlib/sysctl.h"               // Defines and macros for System Control API of DriverLib
#include "driverlib/interrupt.h"            // Defines and macros for NVIC Controller API of DriverLib
#include "driverlib/timer.h"                // Defines and macros for Timer API of driverLib
#include "driverlib/gpio.h"                 // Defines and macros for GPIO API of DriverLib
#include "driverlib/pin_map.h"              // Mapping of peripherals to pins for all parts
#include "driverlib/uart.h"                 // Defines and Macros for the UART
#include "driverlib/rom.h"                  // Defines and macros for ROM API of driverLib

void TimerInterupt_Init(void);
void IOSenosr_Init(void);
void PWM_Init(void);
void SysTick_Init(void);

void SysTick_Wait(unsigned long delay);
void SysTick_Waitmicro(unsigned long delay);

void disable_interrupts(void);
void enable_interrupts(void);
void wait_for_interrupts(void);
void timerInterupt(void);

double getRelDistance(void);
void setRelVelocity(void);

//Global Variables
volatile double relDis;
volatile double relVel;
volatile uint32_t debug;

int main(){
    debug = 1;
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    debug = 2;
    SysTick_Init();
    debug = 3;
    PWM_Init();
    IOSenosr_Init();
    debug = 4;
    TimerInterupt_Init();
    debug = 5;

    while(1){
        //wait_for_interrupts();
    }

    return 0;
}

void timerInterupt(void){
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    setRelVelocity();
    debug = 6;

    //relDis = getRelDistance();
}

void IOSenosr_Init(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    // Set the PA3 port as Output. Trigger Pi
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
    // Set the PA2 port as Input with a weak Pull-down. Echo Pin
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD);
}

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
}

// The delay parameter is in units of the 16 MHz core clock. (62.5 ns)
void SysTick_Wait(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){ // wait for count flag
  }
}


void SysTick_Waitmicro(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(25);
  }
}

//initialize PWM
void PWM_Init(void) {
    SYSCTL_RCGCPWM_R |= 0x02;     // 1) enable PWM1 clock
    SYSCTL_RCGC2_R |= 0x10;          // 2.1) activate clock for PortE
    while ((SYSCTL_PRGPIO_R & 0x10) == 0) {}; // 2.2) wait until PortE is ready
    GPIO_PORTE_AFSEL_R |= 0x30;       // 3) enable alt function on PE5-4
    GPIO_PORTE_PCTL_R &= 0x00FF0000; // 4.1) clear PE5-4 GPIOPCTL PMCx fields
    GPIO_PORTE_PCTL_R |= 0x00550000; // 4.2) configure PE5-4 as PWM Module 1
    GPIO_PORTE_AMSEL_R &= ~0x30;     // 4.3) disable analog functionality on PE5-4
    GPIO_PORTE_DEN_R |= 0x30;        // 4.4) enable digital I/O on PE5-4
    SYSCTL_RCC_R |= 0x00100000;      // 5.1) configure PWM clock divider as the source for PWM clock
    SYSCTL_RCC_R &= ~0x000E0000;     // 5.2) clear PWMDIV field
    SYSCTL_RCC_R |= 0x000060000;     // 5.3) set divisor to 16 so PWM clock source is 1 MHz
    PWM1_1_CTL_R = 0;                // 6.1) disable PWM while initializing; also configure Count-Down mode
    PWM1_1_GENA_R = 0x08C;           // 6.2) drives pwmA HIGH when counter matches value in PWM1LOAD
                                          // drive pwmA LOW when counter matches comparator A
    PWM1_1_GENB_R = 0x80C;           // 6.3) drives pwmB HIGH when counter matches value in PWM1LOAD
                                          // drive pwmB LOW when counter matches comparator B
    PWM1_1_LOAD_R = 10001 -1;        // 7) since target period is 100Hz, there are 10,000 clock ticks per period
    PWM1_1_CMPA_R = 10000 -1;        // 8) set 0% duty cycle to PE4
    PWM1_1_CMPB_R = 10000 -1;        // 9) set 0% duty cycle to PE5
    PWM1_1_CTL_R |= 0x01;            // 10) start the timers in PWM generator 1 by enabling the PWM clock
    PWM1_ENABLE_R |= 0x0C;           // 11) Enable M1PWM2 and M1PWM3
}

void TimerInterupt_Init(void){
    uint32_t period = SysCtlClockGet()/20;
    //send clock to peripheral
    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );
    // Configure Timer0 to run in periodic mode
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC );
    //set period
    TimerLoadSet( TIMER0_BASE, TIMER_A, period -1 );
    // Enable the Interrupt specific vector associated with Timer0A
    IntEnable(INT_TIMER0A);
    // Enables a specific event within the timer to generate an interrupt
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable( TIMER0_BASE, TIMER_A );
}

double getRelDistance(){
    uint32_t counter = 0;
    uint32_t maxDis = 100; //cm
    uint32_t maxCount = 1000000*maxDis*2/34300; // maxDis converted to micro_s, 5.83 ms at 100 cm

    //debug = 20;
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3); //write high to trigger
    //debug = 21;
    SysTick_Waitmicro(10); // wait 10 micro_s
    //debug = 22;
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0); //write low to trigger

    //debug = 23;
    while(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) == 0){} //wait for echo to go high

    //debug = 24;
    //count while echo is high
    while(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) != 0 && counter < maxCount){
        counter+=10;
        SysTick_Waitmicro(10);
    }

    //debug = 25;
    return (double)(counter*34300)/2000000;

}

void setRelVelocity(){
    int waitTime = 2000; //micro_s
    double dist1 = getRelDistance();
    SysTick_Waitmicro(waitTime);  //need to tune a separate function for this
    double dist2 = getRelDistance();
    relDis = (dist1 + dist2)/2;
    relVel = (dist2 - dist1)/((double)waitTime/1000000); //cm/s
}



/* Disable interrupts by setting the I bit in the PRIMASK system register */
void disable_interrupts(void) {
    __asm("    CPSID  I\n"
          "    BX     LR");
}


/* Enable interrupts by clearing the I bit in the PRIMASK system register */
void enable_interrupts(void) {
    __asm("    CPSIE  I\n"
          "    BX     LR");
}


/* Enter low-power mode while waiting for interrupts */
void wait_for_interrupts(void) {
    __asm("    WFI\n"
          "    BX     LR");
}
