#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_random.h"
#include "sdkconfig.h"
#include "screen.h"

circle_t *mat_circle;
static u8g2_t *u8g2 = NULL;
bool mat_init = 0;
uint16_t scr_sq_size, mat_dim;

/**
 * @brief Convert 1D array to 2D array
 *
 * Return the i and j indexes in a bidimensional array from
 * a linear array index idx
 *
 * @param[idx] linear array index
 * @param[i]
 * @param[j] pointers to return values
 *
 * @return
 *		no return
 */
void mat_idx_to_2d(uint16_t idx, uint16_t *i, uint16_t *j){

	*i = idx % mat_dim;
	*j = idx / mat_dim;
}

/**
 * @brief Preset circles centers
 */
void mat_populate(void) {
	uint16_t i, j;
	uint16_t rmax;

	if(!mat_init) return;

	rmax = scr_sq_size / mat_dim;

	for( i=0; i< mat_dim; i++) {
		for( j=0; j< mat_dim; j++) {
			mat_circle[i*mat_dim + j].x = i * rmax + rmax/2 - 1;
			mat_circle[i*mat_dim + j].y = j * rmax + rmax/2 - 1;
		}
	}
}

/**
 * @brief  Setup the display layout
 */
void mat_setup(u8g2_t *u8g2_in, uint16_t resolution, uint16_t divisions) {
	u8g2 = u8g2_in;

	scr_sq_size = resolution;
	mat_dim = divisions;

	mat_init = true;

	mat_circle = malloc(sizeof(circle_t) * divisions * divisions);

	if(!mat_circle) {
		ESP_LOGE("TAG", "Error allocating memory");
		mat_init = false;
		return;
	}

	mat_populate();
}


/**
 * @brief Plot the data
 */
void mat_plot(VL53L8CX_ResultsData 	results) {
	uint16_t i, j;
	uint16_t r;
	float af = 7.0/1600.0;
	float rf;

	if(!mat_init)
		return;

	u8g2_ClearBuffer(u8g2);

#if 0
		/* As the sensor is set in 4x4 mode by default, we have a total
		 * of 16 zones to print. For this example, only the data of first zone are
		 * print */
		printf("Print data no : %3u\n", Dev.streamcount);
		for(i = 0; i < 16; i++)
		{
			printf("Zone : %3d, Status : %3u, Distance : %4d mm\n",
					i,
					Results.target_status[VL53L8CX_NB_TARGET_PER_ZONE*i],
					Results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE*i]);
		}
#endif

	for( i=0; i< mat_dim; i++) {
		for( j=0; j< mat_dim; j++) {
			r = results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * (i * mat_dim + j)];
			rf = (float)r * af;
			r = (uint16_t)rf;
			if (r > 7) r = 7;
			r = 7 - r;
			u8g2_DrawDisc(u8g2, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim +j].y, r, U8G2_DRAW_ALL);
			//u8g2_DrawCircle(u8g2, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim +j].y, r, U8G2_DRAW_ALL);
		}
	}

	u8g2_SendBuffer(u8g2);
}
