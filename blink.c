#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "indicator.c"
//#include "left_two.c"
//#include "left_three.c"
//#include "left_four.c"
#include "right.c"


#define LCD_HOST        HSPI_HOST
#define DMA_CHAN        2

#define PIN_NUM_MOSI   23
#define PIN_NUM_MISO   25
#define PIN_NUM_CLK    18
#define PIN_NUM_CS      5
#define PIN_NUM_DC     17
#define PIN_NUM_RST    16
#define PARALLEL_LINES 16

#define LOW_BEAM       13
#define HIGH_BEAM      12
#define LEFT_TURN      14
#define RIGHT_TURN     27
#define BATTERY        26
#define OIL            25
#define NEUTRAL        33
#define TEST            2

#define BLACK          0x0000
//#define BLUE           0xf800
//#define RED            //0x07e0
#define GREEN          0x001f

extern uint16_t BLUE[];
//extern uint16_t left_data[];
extern uint16_t INDICATOR[];
//extern uint16_t INDICATOR_TWO[];
//extern uint16_t INDICATOR_THREE[];
//extern uint16_t INDICATOR_FOUR[];

typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;




void gpio_init()
{

    gpio_pad_select_gpio(LOW_BEAM);
    gpio_pad_select_gpio(HIGH_BEAM);
    gpio_pad_select_gpio(LEFT_TURN);
    gpio_pad_select_gpio(RIGHT_TURN);
    gpio_pad_select_gpio(BATTERY);
    gpio_pad_select_gpio(OIL);
    gpio_pad_select_gpio(NEUTRAL);
    gpio_pad_select_gpio(TEST);
    gpio_pad_select_gpio(PIN_NUM_DC);
    gpio_pad_select_gpio(PIN_NUM_RST);

    gpio_set_direction(LOW_BEAM, GPIO_MODE_INPUT);
    gpio_set_direction(HIGH_BEAM, GPIO_MODE_INPUT);
    gpio_set_direction(LEFT_TURN, GPIO_MODE_INPUT);
    gpio_set_direction(RIGHT_TURN, GPIO_MODE_INPUT);
    gpio_set_direction(BATTERY, GPIO_MODE_INPUT);
    gpio_set_direction(OIL, GPIO_MODE_INPUT);
    gpio_set_direction(NEUTRAL, GPIO_MODE_INPUT);
    gpio_set_direction(TEST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
}


DRAM_ATTR static const lcd_init_cmd_t ssd1351[]={
    {0xfd, {0x12}, 1},
    {0xfd, {0xb1}, 1},
    {0xae, {0x00}, 0},
    {0xb2, {0xa4, 0x00, 0x00}, 3},
    {0xb3, {0xf0}, 1},
    {0xca, {0x7f}, 1},
    {0xa0, {0x74}, 1},
    {0xa1, {0x00}, 1},
    {0xa2, {0x00}, 1},
    {0xb5, {0x00}, 1},
    {0xab, {0x01}, 1},
    {0xb1, {0x32}, 1},
    {0xbb, {0x1f}, 1},
    {0xbe, {0x05}, 1},
    {0xa6, {0x00}, 0},
    {0xc7, {0x0a}, 1},
    {0xc1, {0xff, 0xff, 0xff}, 3},
    {0xb4, {0xa0, 0xb5, 0x55}, 3},
    {0xb6, {0x01}, 1},
    {0xaf, {0}, 0},

};
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
     spi_transaction_t t;
     memset(&t, 0, sizeof(t));
     t.length=8;
     t.tx_buffer=&cmd;
     t.user=(void*)0;
     spi_device_polling_transmit(spi, &t);
}

void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
     spi_transaction_t t;
     if (len == 0)return;
     memset(&t, 0, sizeof(t));
     t.length=len*8;
     t.tx_buffer=data;
     t.user=(void*)1;
     spi_device_polling_transmit(spi, &t);
}

void lcd_rgb(spi_device_handle_t spi,  uint16_t rgb)
{
     spi_transaction_t t;
     memset(&t, 0, sizeof(t));
     t.length=16;
     t.tx_buffer=&rgb;
     t.user=(void*)1;
     spi_device_polling_transmit(spi, &t);
}


void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

void lcd_init(spi_device_handle_t spi)
{
     int cmd=0;

     gpio_set_level(PIN_NUM_RST, 0);
     vTaskDelay(400 / portTICK_RATE_MS);
     gpio_set_level(PIN_NUM_RST, 1);
     vTaskDelay(400 / portTICK_RATE_MS);


      while (ssd1351[cmd].databytes!=0xff) {
        lcd_cmd(spi, ssd1351[cmd].cmd);
        lcd_data(spi, ssd1351[cmd].data, ssd1351[cmd].databytes&0x1F);
        cmd++;
    }
}

void sort (spi_device_handle_t spi, uint16_t *c, uint16_t col, uint16_t row, uint16_t mxp)
{

            uint16_t k = 0;
            uint16_t *p;
            //uint16_t x=0;

            lcd_cmd(spi, 0x15);
            lcd_rgb(spi, col);
            lcd_cmd(spi, 0x75);
            lcd_rgb(spi, row);
            lcd_cmd(spi, 0x5C);

        while(k <= mxp)
              {
                 p = &c[k];
                 lcd_rgb(spi, *p);
                 k++;
              }

}
void app_main(void)
{



        gpio_init();    //initalise gpio
       // esp_err_t ret;
        spi_device_handle_t spi;
        spi_bus_config_t buscfg={.miso_io_num=-1,
                                 .mosi_io_num=PIN_NUM_MOSI,
                                 .sclk_io_num=PIN_NUM_CLK,
                                 .quadwp_io_num=-1,
                                 .quadhd_io_num=-1,
                                 .max_transfer_sz=4096};

        spi_device_interface_config_t devcfg={.clock_speed_hz=14500000,
                                              .mode=0,
                                              .spics_io_num=PIN_NUM_CS,
                                              .queue_size=7,
                                              .pre_cb = lcd_spi_pre_transfer_callback};

        spi_bus_initialize(LCD_HOST, &buscfg, DMA_CHAN);
        spi_bus_add_device(LCD_HOST, &devcfg, &spi);


        lcd_init(spi);


while(1) {

       //  tegn(spi);
        /* Blink off (output low) */
	//printf("Turning off the LED\n");
      //  gpio_set_level(TEST, 1);
      //  }
       if(gpio_get_level(LOW_BEAM) == 1)
          {
           sort(spi, INDICATOR, 0x7f00, 0x7f00, 16389);

                      }
       if (gpio_get_level(LOW_BEAM) == 0)
          {
                   //PICTURE = extern RED;
          
                   sort(spi, BLUE, 0x3f00, 0x3f00, 4096);
                   //sort(spi, INDICATOR_TWO, 0x7f00, 0x3f20, 4096);
                   //sort(spi, INDICATOR_THREE, 0x7f00, 0x5f40, 4096);
                   //sort(spi, INDICATOR_FOUR, 0x7f00, 0x7f60, 4096);


                   //sort(spi, GREEN, 0x7f40, 0x3f00);
                   //sort(spi, BLUE, 0x7f40, 0x7f40);
                   //sort(spi, RED,0x3f00, 0x3f00);
            }

   }
}
