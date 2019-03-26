#include <stdint.h>                         // Library of Standard Integer Types
#include <stdbool.h>                        // Library of Standard Boolean Types
#include <stdlib.h>                         // Library of Standard Datatype Conversions
#include "tm4c123gh6pm.h"               // Definitions for interrupt and register assignments on Tiva C
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

void SysTick_Init(void);
void SysTick_Wait(unsigned long delay);
void SysTick_Wait1micro_s(unsigned long delay);

volatile double dist;
volatile uint32_t debug;
volatile uint32_t counter;

double getRelDistance(){

    counter = 0;
    debug = 2;
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0XFF);
    debug =3;
    SysTick_Wait1micro_s(10);
    debug = 4;
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0);
    debug = 5;

    uint32_t maxDis = 100; //cm
    uint32_t maxCount = maxDis*2/34300;
    while(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) == 0 && counter < maxCount){
        counter++;
        SysTick_Wait1micro_s(1000);
        debug = 6;

    }
    debug = 7;

    return counter*34300/2;

}

int main(){
    debug = 0;
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    SysTick_Init();
        debug = 11;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    // Set the PA3 port as Output. Trigger Pin
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
    debug = 12;
    // Set the PA2 port as Input with a weak Pull-down. Echo Pin
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2);
    debug = 13;
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD);

    while(1){
        debug = 1;
        dist = getRelDistance();
    }

    return 0;
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

// 160000*62.5ns equals 10ms
void SysTick_Wait1micro_s(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(16);  // wait 10ms
  }
}
