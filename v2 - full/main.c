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
void PF4_Init(void);
void SysTick_Init(void);

void SysTick_Wait(unsigned long delay);
void SysTick_Waitmicro(unsigned long delay);

void disable_interrupts(void);
void enable_interrupts(void);
void wait_for_interrupts(void);
void timerInterupt(void);
void buttonInterrtupt(void);

double getRelDistance(void);
void setRelVelocity(void);
void setLeadVelocity(void);
void setX_des(void);
void setX_pid(void);
void XtoVelocity(void);
void VtoVoltage(void);
void PID (void);
void Break (void);


//Global Variables
volatile double relDis;
volatile double relVel;
volatile double leadVel;
volatile double hostVel;
volatile double X_des;
volatile double X_pid;
volatile double duty;
volatile int freq = 50; // deltaT = 1/freq
volatile uint32_t debug;

int main(){
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    debug = 1;
    SysTick_Init();
    debug =2;
    PF4_Init();
    debug = -1;
    PWM_Init();
    debug = 2;
    IOSenosr_Init();
    debug = 4;
    TimerInterupt_Init();
    debug = 5;

    debug = 6;
    hostVel = 0;
    enable_interrupts();
    //disable_interrupts();
    debug = 7;


    while(1){
        debug = 88;
//        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7);
//        SysTick_Waitmicro(105000);
//        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0);
//        SysTick_Waitmicro(105000);
        wait_for_interrupts();
    }

    return 0;
}

void timerInterupt(void){
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    relDis = getRelDistance();
    PID ();

//    setRelVelocity();
//    setLeadVelocity();
//    setX_des();
//    setX_pid();
//    XtoVelocity();
    Break();
    VtoVoltage();
}

void Break (void){
    if(relDis < 5){
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7);
    } else {
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0);
    }
}


void PID (void){
    double goal = 15;
    double error = relDis - 15;
    double P = 2.5;
    hostVel = error*P;
    if(hostVel < 0){
        hostVel = 0;
    }
}

void VtoVoltage(void){

    if(hostVel > 65){
        hostVel = 65;
    }
    if(hostVel < 25 ){
      duty = 0;
        //  duty = 1.6*hostVel;
    } else if(hostVel < 75){
        duty = .0249*hostVel*hostVel - 1.4834*hostVel + 64.824;
    } else  {
        duty = 100;
    }


    PWM1_1_CMPA_R = 10000 - 100*duty - 1;

}

void XtoVelocity(void){
    double t = (double)1/freq;
    hostVel = (double)(2/t)*(leadVel*t + relDis - X_pid) - hostVel;
    if (hostVel > 75){
        hostVel = 75;
    }

    if (hostVel < 0){
        hostVel = 0;
    }
}

void setX_pid(void){
    double P = 1;
    X_pid = P*(X_des - relDis);
    if (X_pid < 0){
        X_pid = 0;
    }
}

void setX_des(void){
    double alpha = 93.7;//max deceleration cm/s^2

    X_des = leadVel*leadVel/alpha;
    if (X_des < 5 ){
        X_des = 5;
    }
}

void setLeadVelocity(void){
    leadVel = hostVel + relVel;
}

void setRelVelocity(){
    int waitTime = 100000; //micro_s
    double dist1 = getRelDistance();
    SysTick_Waitmicro(waitTime);
    double dist2 = getRelDistance();
    relDis = (dist1 + dist2)/2;

    if(dist2 > 99){
        relVel = 75; //cm/s, -> repsent imaginary car at max distance
    } else {
        relVel = (dist2 - dist1)/((double)waitTime/1000000); //cm/s
    }
    relVel = 0;
}


double getRelDistance(){
	
    uint32_t counter = 0;
    uint32_t maxDis = 100; //cm
    uint32_t maxCount = 1000000*maxDis*2/34300; // maxDis converted to micro_s, 5.83 ms at 100 cm

    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3); //write high to trigger
    SysTick_Waitmicro(10); // wait 10 micro_s
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0); //write low to trigger

    while(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) == 0){} //wait for echo to go 
    //count while echo is high
    while(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) != 0 && counter < maxCount){
        counter+=10;
        SysTick_Waitmicro(10);
    }

    return (double)(counter*34300)/2000000;

}


void IOSenosr_Init(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    // Set the PA3 port as Output. Trigger Pi
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
    // Set the PA2 port as Input with a weak Pull-down. Echo Pin
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD);
    //for break
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_7);
}

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
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

/* Initialize PF4 (SW1) */
void PF4_Init(void) {
    SYSCTL_RCGC2_R |= 0x00000020;           // activate clock for PortF
    while ((SYSCTL_PRGPIO_R & 0x00000020) == 0)
    {};                          // wait until PortF is ready
    GPIO_PORTF_DIR_R &= ~0x10;              // make PF4 input
    GPIO_PORTF_AFSEL_R &= ~0x10;            // disable alt function on PF4
    GPIO_PORTF_DEN_R |= 0x10;               // enable digital I/O on PF4
    GPIO_PORTF_PCTL_R &= ~0x000F0000;       // use PF4 as GPIO
    GPIO_PORTF_AMSEL_R &= ~0x10;            // disable analog on PF4
    GPIO_PORTF_PUR_R = 0x10;                // enable pull-up on PF4
    GPIO_PORTF_IS_R &= ~0x10;               // PF4 is edge-sensitive
    GPIO_PORTF_IBE_R &= ~0x10;              // PF4 is not both edges
    GPIO_PORTF_IEV_R &= ~0x10;              // PF4 falling edge event
    GPIO_PORTF_ICR_R = 0x10;                // clear flag4
    GPIO_PORTF_IM_R |= 0x10;                // arm interrupt on PF4
    NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // priority 5
    NVIC_EN0_R = 0x40000000;                // enable interrupt 30 in NVIC
}

void TimerInterupt_Init(void){
    uint32_t ticks = SysCtlClockGet()/freq;
    //send clock to peripheral
    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );
    // Configure Timer0 to run in periodic mode
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC );
    //set period
    TimerLoadSet( TIMER0_BASE, TIMER_A, ticks -1 );
    // Enable the Interrupt specific vector associated with Timer0A
    IntEnable(INT_TIMER0A);
    // Enables a specific event within the timer to generate an interrupt
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable( TIMER0_BASE, TIMER_A );
}

void buttonInterrtupt(void){
    disable_interrupts();
    PWM1_1_CMPA_R = 9999;
    while (1){}
}

// The delay parameter is in units of the 16 MHz core clock. (62.5 ns)
void SysTick_Wait(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){ // wait for count flag
  }
}


void SysTick_Waitmicro(unsigned long delay){
  delay = (delay-10)*(1.05) + 10;
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(25);
  }
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
