#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include <stdio.h>
#include"LcdDriver/Crystalfontz128x128_ST7735.h"

//*****************************************************************************
Graphics_Context g_sContext;
volatile uint8_t ADC_flag = 0;
static volatile int16_t resultsBuffer[3];

volatile int32_t px = 48;
int32_t py = 124;
int32_t paddle_length = 32;

volatile int32_t score = 0;

// Global flag
volatile uint32_t flag;
volatile int32_t x = 64, y = 64;
volatile int32_t vx = 2, vy = -1;
int32_t radius = 3;

void ADC_init(void) {
    ADC_flag = 0;
    Interrupt_disableMaster();
    // Configures Pin 4.0, 4.2, and 6.1 as ADC input
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2, GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN1, GPIO_TERTIARY_MODULE_FUNCTION);

    // Initializing ADC (ADCOSC/64/8)
    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM2 (A11, A13, A14)  with no repeat)
         * with internal 2.5v reference */
    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM2, true);
    ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A14, ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A13, ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_configureConversionMemory(ADC_MEM2,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A11, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 2 (end of sequence)
     *  is complete and enabling conversions */
    ADC14_enableInterrupt(ADC_INT2);

    // Enabling Interrupts
    Interrupt_enableInterrupt(INT_ADC14);
    Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    // Triggering the start of the sample
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
}

void ADC14_IRQHandler(void)
{
    uint64_t status;

    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);

    // ADC_MEM2 conversion completed
    if(status & ADC_INT2)
    {
        // Store ADC14 conversion results
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);
        resultsBuffer[2] = ADC14_getResult(ADC_MEM2);
        ADC_flag = 1;
    }
}

void erase_paddle(int32_t px, int32_t py, int32_t paddle_length) {
    // draw over old paddle with background color to erase it
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawLineH(&g_sContext, px, px + paddle_length-1, py);
    // restore the foreground color setting
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
}

void draw_paddle(int32_t px, int32_t py, int32_t paddle_length) {
    Graphics_drawLineH(&g_sContext, px, px + paddle_length-1, py);
}

void SysTick_Handler(void) {
   flag = 1;
}

void SysTick_init(int32_t period) {
   SysTick_setPeriod(period);
   SysTick->VAL = 0;
   SysTick_enableModule();
   SysTick_enableInterrupt();
}

//*****************************************************************************

void LCD_init() {
   Crystalfontz128x128_Init(); // LCD Screen Size 128x128
   Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
   Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,&g_sCrystalfontz128x128_funcs);
   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
   Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
   GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
   Graphics_clearDisplay( &g_sContext);
}

//*****************************************************************************

void erase(int32_t x, int32_t y, int32_t radius) {
   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);// paint dot white to "erase"
   Graphics_fillCircle(&g_sContext, x, y, radius);// set circle radius
   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE); // reset color after erasing
}

void draw(int32_t x, int32_t y, int32_t radius) {
   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);// set dot color again
   Graphics_fillCircle(&g_sContext, x, y, radius); // paint red dot
}

//*****************************************************************************

int main(void) {

   // Stop Watchdog
   WDT_A_holdTimer();

   // Initialize color LDC
   LCD_init();

   ADC_init();

   // Initialize GPIO pins
   GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN1);

   // Start message
   Graphics_drawStringCentered(&g_sContext, (int8_t*) "Press to Begin", AUTO_STRING_LENGTH, 64, 50, OPAQUE_TEXT);

   // Polling start switch 1.1
   while(GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN1)) {
       ;
   }

   // De-bounce switch 1.1
   __delay_cycles(60000);

   // Clear start message
   Graphics_clearDisplay(&g_sContext);

   // SysTick interrupts at 10 ~ 20Hz
   // Note: Clock frequency is 3 MHz
   SysTick_init(299999);

    while(1) {
        if (flag) {
            flag = 0;

            if ((y + vy + radius) >= py && x >= px && x <= px + paddle_length - 1) {
                vy = -vy;
                score++;
            } else if (( y + vy + radius) >= py) {
                Interrupt_disableMaster();

                Graphics_clearDisplay(&g_sContext);
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);

                char string[20];
                Graphics_drawStringCentered(&g_sContext, (int8_t *) "GAME OVER!", AUTO_STRING_LENGTH, 64, 50, OPAQUE_TEXT);
                sprintf(string, "Score: %d", (int)score);
                Graphics_drawStringCentered(&g_sContext, (int8_t*)string, AUTO_STRING_LENGTH, 64, 70, OPAQUE_TEXT);

                while(1);
            }

            // Save old positions
            int32_t x0 = x, y0 = y;

            // Checking x boundaries
            if ((x + vx + radius) > 127 || (x + vx - radius) < 0) {
                vx = -vx;
            }

            // Checking y boundaries
            if (( y  + vy - radius) < 0) {
                vy = -vy;
            }

            // Update position of ball
            x += vx;
            y += vy;

            // Erase old ball and draw new one
            erase(x0, y0, 3);
            draw(x, y, 3);
        }

        if (ADC_flag) {
            ADC_flag = 0;

            int32_t px0 = px;

            if (resultsBuffer[0] > 8800) {
                px += 1;
            } else if (resultsBuffer[0] < 8280) {
                px -= 1;
            }

            if (px < 0) {
                px = 0;
            }
            if ( px + paddle_length > 127) {
                px = 127 - paddle_length;
            }
            if (px != px0) {
                erase_paddle(px0, py, paddle_length);
                draw_paddle(px,py, paddle_length);
            }
        }
    }
}
