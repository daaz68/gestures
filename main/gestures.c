#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "gestures.h"

#if 0
   GND: Ground (0V)
   VCC: Power Supply (3.3V or 5V, check if your module has a regulator)
19 SCL (or CLK/SCK): SPI Clock Signal
18 SDA (or MOSI/SDI): SPI Serial Data Input (MOSI)
 2 RST (or RES): Reset Signal (Active LOW)
 1 DC (or D/C, RS, A0): Data/Command Selection (Low = Command, High = Data)
21 CS: Chip Select Signal (Active LOW)
17 BL (or BLK/LED): Backlight Control (3.3V/5V) 
#endif



static const char *TAG = "AMTR";

/* I2C Configuration (configurable via menuconfig) */
#define I2C_MASTER_NUM    I2C_NUM_0                        /*!< I2C master port number */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA            /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL            /*!< GPIO number used for I2C master clock */
#define I2C_FREQ_HZ       CONFIG_I2C_MASTER_FREQUENCY      /*!< I2C master clock frequency */
#define I2C_TIMEOUT_MS    1000                             /*!< I2C master timeout */

/* Display Configuration (configurable via menuconfig) */
#define I2C_DISPLAY_ADDRESS  CONFIG_I2C_DISPLAY_ADDRESS    /*!< Display I2C address */

VL53L8CX_Configuration 	Dev;			/* Sensor configuration */
VL53L8CX_ResultsData 	Results;		/* Results data from VL53L8CX */

static i2c_master_bus_handle_t i2c_bus_handle = NULL;   /*!< I2C master bus handle */
static i2c_master_dev_handle_t display_dev_handle = NULL;  /*!< Display device handle */
static u8g2_t u8g2;
static char buff[35];

i2c_master_bus_config_t i2c_bus_config = {
	.i2c_port = I2C_MASTER_NUM,
	.sda_io_num = I2C_MASTER_SDA_IO,
	.scl_io_num = I2C_MASTER_SCL_IO,
	.clk_source = I2C_CLK_SRC_DEFAULT,
	.glitch_ignore_cnt = 7,
	.intr_priority = 0,
	.trans_queue_depth = 0,  /* 0 = synchronous mode */
	.flags.enable_internal_pullup = true,
};

/**
 * @brief U8X8 I2C communication callback function
 *
 * This function handles all I2C communication between U8G2 library and display controller.
 * Please refer to https://github.com/olikraus/u8g2/wiki/Porting-to-new-MCU-platform for more details.
 *
 * @param[in] u8x8 Pointer to u8x8 structure (not used)
 * @param[in] msg Message type from U8G2 library
 * @param[in] arg_int Integer argument (varies by message type)
 * @param[in] arg_ptr Pointer argument (varies by message type)
 *
 * @return
 *     - 1: Success
 *     - 0: Failure
 */
static uint8_t u8x8_byte_i2c_cb(u8x8_t *u8x8, uint8_t msg,
                                uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[132];  /*!< Enhanced buffer: control byte + 128 data bytes + margin */
    static uint8_t buf_idx;      /*!< Current buffer index */

    switch (msg) {
    case U8X8_MSG_BYTE_INIT:
        /* Add display device to the I2C bus */
        i2c_device_config_t dev_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = I2C_DISPLAY_ADDRESS,
            .scl_speed_hz = I2C_FREQ_HZ,
            .scl_wait_us = 0,  /* Use default value */
            .flags.disable_ack_check = false,
        };

        esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &display_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C master driver initialized failed");
            return 0;
        }

        ESP_LOGI(TAG, "I2C master driver initialized successfully");
        break;

    case U8X8_MSG_BYTE_START_TRANSFER:
        /* Start transfer, reset buffer */
        buf_idx = 0;
        break;

    case U8X8_MSG_BYTE_SET_DC:
        /* DC (Data/Command) control - handled by SSD1306 protocol */
        break;

    case U8X8_MSG_BYTE_SEND:
        /* Add data bytes to buffer */
        for (size_t i = 0; i < arg_int; ++i) {
            buffer[buf_idx++] = *((uint8_t*)arg_ptr + i);
        }
        break;

    case U8X8_MSG_BYTE_END_TRANSFER:
        /* Transmit data using new I2C driver */
        if (buf_idx > 0 && display_dev_handle != NULL) {
            esp_err_t ret = i2c_master_transmit(display_dev_handle, buffer, buf_idx, I2C_TIMEOUT_MS);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "I2C master transmission failed");
                return 0;
            }

            /* Debug output: show transmitted data */
            ESP_LOGD(TAG, "Sent %d bytes to 0x%02X: control_byte=0x%02X",
                     buf_idx, I2C_DISPLAY_ADDRESS, buffer[0]);
        }
        break;

    default:
        return 0;
    }
    return 1;
}

/**
 * @brief U8X8 GPIO control and delay callback function for ESP32
 *
 * This function handles GPIO operations and timing delays required by U8G2 library.
 * Please refer to https://github.com/olikraus/u8g2/wiki/Porting-to-new-MCU-platform for more details.
 *
 * @param[in] u8x8 Pointer to u8x8 structure (not used)
 * @param[in] msg Message type from U8G2 library
 * @param[in] arg_int Integer argument (varies by message type)
 * @param[in] arg_ptr Pointer argument (not used)
 *
 * @return
 *     - 1: Success
 *     - 0: Failure
 */
static uint8_t u8x8_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg,
                                  uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        ESP_LOGI(TAG, "GPIO and delay initialization completed");
        break;

    case U8X8_MSG_DELAY_MILLI:
        /* Millisecond delay */
        vTaskDelay(pdMS_TO_TICKS(arg_int));
        break;

    case U8X8_MSG_DELAY_10MICRO:
        /* 10 microsecond delay */
        esp_rom_delay_us(arg_int * 10);
        break;

    case U8X8_MSG_DELAY_100NANO:
        /* 100 nanosecond delay - use minimal delay on ESP32 */
        __asm__ __volatile__("nop");
        break;

    case U8X8_MSG_DELAY_I2C:
        /* I2C timing delay: 5us for 100KHz, 1.25us for 400KHz */
        esp_rom_delay_us(5 / arg_int);
        break;

    case U8X8_MSG_GPIO_RESET:
        /* GPIO reset control (optional for most display controllers) */
        break;

    default:
        /* Other GPIO messages not handled */
        return 0;
    }
    return 1;
}

/**
 * @brief Update the display content based on the float parameter
 *
 */
void update_display(void){
	char val[35];

	u8g2_ClearBuffer(&u8g2);

	u8g2_SetFont(&u8g2, u8g_font_helvB14);
	sprintf(buff, "%4.2fV   mA", 15.0);
	u8g2_DrawStr(&u8g2, 26, 27, buff);

	/* Large title font */
	sprintf(buff, "%+07.2f", 20 * 10.0);
	sprintf(val, "%8s", buff);
	u8g2_SetFont(&u8g2, u8g_font_helvR24);
	u8g2_DrawStr(&u8g2, 4, 60, val);
	u8g2_SendBuffer(&u8g2);
}

/**
 * @brief Main application entry point
 *
 * This function initializes the U8G2 library, configures the display controller,
 */
void app_main(void)
{
	vTaskPrioritySet(NULL, 24);

    /*********************************/
    /*   VL53L8CX ranging variables  */
    /*********************************/

    uint8_t status, isAlive, isReady, freq = 15;

	ESP_LOGI(TAG,"SDA GPIO: %d, SCL GPIO: %d",I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    /* Create I2C master bus */
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
#if 0
    /* Initialize U8G2 for display type */
	u8g2_Setup_sh1107_i2c_128x128_f(
        &u8g2, U8G2_R0,
        u8x8_byte_i2c_cb,   /* I2C communication callback */
        u8x8_gpio_delay_cb  /* GPIO and delay callback */
    );

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);  /* Wake up display */
	u8g2_SetDisplayRotation(&u8g2, U8G2_R1);
	u8g2_ClearBuffer(&u8g2);
#endif

    //Define the i2c device configuration
    i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = VL53L8CX_DEFAULT_I2C_ADDRESS >> 1,
            .scl_speed_hz = VL53L8CX_MAX_CLK_SPEED,
    };


    /*********************************/
    /*      Customer platform        */
    /*********************************/

    Dev.platform.bus_config = i2c_bus_config;

    //Register the device
    i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &Dev.platform.handle);

    /* (Optional) Reset sensor */
    Dev.platform.reset_gpio = GPIO_NUM_16;

    Reset_Sensor(&(Dev.platform));

    /*********************************/
    /*   Power on sensor and init    */
    /*********************************/

    /* (Optional) Check if there is a VL53L8CX sensor connected */
    status = vl53l8cx_is_alive(&Dev, &isAlive);

    if(!isAlive || status) {
        ESP_LOGE(TAG, "VL53L8CX not detected at requested address\n");
        return;
    }

    /* (Mandatory) Init VL53L8CX sensor */
    status = vl53l8cx_init(&Dev);

    if(status) {
        ESP_LOGE(TAG, "VL53L8CX ULD Loading failed\n");
        return;
    }

	status = vl53l8cx_set_resolution(&Dev, VL53L8CX_RESOLUTION_8X8);
	status = vl53l8cx_set_ranging_frequency_hz(&Dev, freq);

    ESP_LOGI(TAG, "VL53L8CX ULD ready ! (Version : %s)\n", VL53L8CX_API_REVISION);


    /*********************************/
    /*   Reduce RAM & I2C access	 */
    /*********************************/

    /* Results can be tuned in order to reduce I2C access and RAM footprints.
     * The 'platform.h' file contains macros used to disable output. If user declare
     * one of these macros, the results will not be sent through I2C, and the array will not
     * be created into the VL53L8CX_ResultsData structure.
     * For the minimum size, ST recommends 1 targets per zone, and only keep distance_mm,
     * target_status, and nb_target_detected. The following macros can be defined into file 'platform.h':
     *
     * #define VL53L8CX_DISABLE_AMBIENT_PER_SPAD
     * #define VL53L8CX_DISABLE_NB_SPADS_ENABLED
     * #define VL53L8CX_DISABLE_SIGNAL_PER_SPAD
     * #define VL53L8CX_DISABLE_RANGE_SIGMA_MM
     * #define VL53L8CX_DISABLE_REFLECTANCE_PERCENT
     * #define VL53L8CX_DISABLE_MOTION_INDICATOR
     */

    status = vl53l8cx_start_ranging(&Dev);

	mat_setup(&u8g2, 128, 8);

	while (1) {

		do {
			status = vl53l8cx_check_data_ready(&Dev, &isReady);
		} while (!isReady);

		vl53l8cx_get_ranging_data(&Dev, &Results);
		ESP_LOGI(TAG, "working...");

//		mat_plot(Results);

		vTaskDelay(pdMS_TO_TICKS(5000));    /* delay for showing static display */
	}
}

