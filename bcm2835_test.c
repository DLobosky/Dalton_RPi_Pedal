#include <bcm2835.h>
#include <stdio.h>

//Define input pins
#define PUSH1          RPI_GPIO_P1_08     //GPIO14 (right button)
#define PUSH2          RPI_V2_GPIO_P1_38  //GPIO20 (left button)
#define TOGGLE_SWITCH  RPI_V2_GPIO_P1_32  //GPIO12
#define FOOT_SWITCH    RPI_GPIO_P1_10     //GPIO15
#define LED            RPI_V2_GPIO_P1_36  //GPIO16

uint32_t read_timer = 0;
uint32_t input_signal = 0;
uint32_t output_signal = 0;
uint32_t booster_value = 2047;

uint8_t FOOT_SWITCH_val;
uint8_t TOGGLE_SWITCH_val;
uint8_t PUSH1_val;
uint8_t PUSH2_val;

int main(int argc, char **argv) {
    printf("\nThis is the start of the program\n\n");
    //Start the BCM2835 library to access GPIO
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root ?\n");
        return 1;
    }
    //Start the SPI BUS
    if (!bcm2835_spi_begin()) {
        printf("bcm2835_spi_begin failed. Are you running as root ?\n");
        return 1;
    }

    //Define PWM
    bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_ALT5 );  //PWM0 signal on GPIO18
    bcm2835_gpio_fsel(13, BCM2835_GPIO_FSEL_ALT0 );  //PWM1 signal on GPIO13
    bcm2835_pwm_set_clock(2);       //Max clk frequency (19.2MHz/2 = 9.6MHz)
    bcm2835_pwm_set_mode(0, 1, 1);  //Channel 0, markspace mode, PWM enabled
    bcm2835_pwm_set_range(0, 64);   //Channel 0, 64 is max range (6bits): 9.6MHz/64 = 150KHz switching PWM freq
    bcm2835_pwm_set_mode(1, 1, 1);  //Channel 1, markspace mode, PWM enabled
    bcm2835_pwm_set_range(1, 64);   //Channel 0, 64 is max range (6bits): 9.6MHz/64 = 150KHz switching PWM freq

    //Define SPI bus configuration
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);    //The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                 //The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);  //4MHz clock with _64
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                    //The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);    //The default

    uint8_t mosi[10] = {0x01, 0x00, 0x00};  //12 bit ADC read 0x08 ch0,-0c for ch1
    uint8_t miso[10] = {0};

    //Define GPIO pins configuration
    bcm2835_gpio_fsel(PUSH1, BCM2835_GPIO_FSEL_INPT);          //PUSH1 button as input
    bcm2835_gpio_fsel(PUSH2, BCM2835_GPIO_FSEL_INPT);          //PUSH2 button as input
    bcm2835_gpio_fsel(TOGGLE_SWITCH, BCM2835_GPIO_FSEL_INPT);  //TOGGLE_SWITCH as input
    bcm2835_gpio_fsel(FOOT_SWITCH, BCM2835_GPIO_FSEL_INPT);    //FOOT_SWITCH as input
    bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);            //LED as output

    bcm2835_gpio_set_pud(PUSH1, BCM2835_GPIO_PUD_UP);          //PUSH1 pull-up enabled
    bcm2835_gpio_set_pud(PUSH2, BCM2835_GPIO_PUD_UP);          //PUSH2 pull-up enabled
    bcm2835_gpio_set_pud(TOGGLE_SWITCH, BCM2835_GPIO_PUD_UP);  //TOGGLE_SWITCH pull-up enabled
    bcm2835_gpio_set_pud(FOOT_SWITCH, BCM2835_GPIO_PUD_UP);    //FOOT_SWITCH pull-up enabled

    while(1) {  //Main Loop

        printf("\nThis is the start of the while loop\n\n");

        //Read 12 bits ADC
        bcm2835_spi_transfernb(mosi, miso, 3);
        input_signal = miso[2] + ((miso[1] & 0x0F) << 8);

        //Read the PUSH buttons every 50000 times (0.25s) to save resources
        read_timer++;
        if (read_timer == 5000) {
            printf("\nThis is the start of the if statement\n\n");
            read_timer = 0;
            PUSH1_val = bcm2835_gpio_lev(PUSH1);
            PUSH2_val = bcm2835_gpio_lev(PUSH2);
            TOGGLE_SWITCH_val = bcm2835_gpio_lev(TOGGLE_SWITCH);
            FOOT_SWITCH_val = bcm2835_gpio_lev(FOOT_SWITCH);
            //Light the effect when the footswitch is activated
            //bcm2835_gpio_write(LED, !FOOT_SWITCH_val);
            //bcm2835_gpio_write(LED, !PUSH1_val);
            bcm2835_gpio_write(LED, !PUSH2_val);
            //bcm2835_gpio_write(LED, !TOGGLE_SWITCH_val);

            printf("\nThis is the end of the if statement\n\n");
        }

        //*** BOOSTER EFFECT ***//
        //The input_signal is attenuated depending on the value of booster_value
        //I am using a simplified version of the Arduino "map" function, more info in: https://www.arduino.cc/en/reference/map
        output_signal = (int)((float)(input_signal) * (float)((float) booster_value / (float) 4095.0));

        //Generate output PWM signal 6 bits
        bcm2835_pwm_set_data(1, output_signal & 0x3F);
        bcm2835_pwm_set_data(0, output_signal >> 6);

        printf("\nThis is the end of the while loop\n\n");
    }

    printf("\nThis is where all the bcm stuff closes and exits.\n\n");
    //Close all and exit
    bcm2835_spi_end();
    bcm2835_close();
    printf("\nThis is the end of the program\n\n");
    return 0;
}