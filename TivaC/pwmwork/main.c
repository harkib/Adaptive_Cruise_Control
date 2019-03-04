/*
 * Author: Tom Liang
 * Date: Feb 23, 2019
 * Objective: produce a 100Hz PWM waveform with adjustable duty cycles.
 * start with a zero duty cycle which is incremented by 10% each time when
 * SW1 is pressed. When the duty cycle reaches 100% it should be reset to 0%.
 * Use Module1 PWM0 (M1PWM2) and Module1 PWM1 (M1PWM3), where the IO pins to
 * be assigned for these PWM outputs are PE4 and PE5, respectively.
 */

#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

void disable_interrupts(void);
void enable_interrupts(void);
void wait_for_interrupts(void);

void PWM_Init(void);
void PF4_Init(void);
void PF4_Handler(void);

void SysTick_Init(void);
void SysTick_Wait(unsigned long delay);
void SysTick_Wait10ms(unsigned long delay);

volatile signed long ComparatorValue = 10000;

/* main */
int main(void){
    SysTick_Init();
    PWM_Init();
    PF4_Init();

    while(1){
        wait_for_interrupts();
    }
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
    enable_interrupts();                    // Enable global Interrupt flag (I)
}

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

void PF4_Handler(void) {
    GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
    SysTick_Wait10ms(1);          // debounce the switch: delay 10ms and then recheck the switch status
    if ((GPIO_PORTF_DATA_R & 0x10) == 0) {
        ComparatorValue -= 1000;
        if (ComparatorValue < 0) ComparatorValue = 10000; // reload to 10000 if it's less than 0
        PWM1_1_CMPA_R = abs(ComparatorValue - 1); // update comparatorA value
        PWM1_1_CMPB_R = abs(ComparatorValue - 1); // update comparatorB value
    }
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
void SysTick_Wait10ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(160000);  // wait 10ms
  }
}
