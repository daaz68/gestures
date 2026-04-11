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
 * @brief Rotates a given line left/right
 * @param(uint16_t) dir: 0 = rotate left, 1 = rotate right
 */
void mat_rotate_line(uint16_t line, uint16_t dir, uint16_t count) {
	circle_t tmp;

	if ( dir == 0 ) {
		tmp = mat_circle[line * mat_dim];
		for( int cnt=0; cnt < count; cnt++){
			for( int i=0; i < (mat_dim - 1); i++)
				mat_circle[line * mat_dim + i] = mat_circle[line * mat_dim + i + 1];
			mat_circle[(line + 1) * mat_dim - 1] = tmp;
		}
	} else {
		tmp = mat_circle[(line + 1) * mat_dim - 1];
		for( int cnt=0; cnt < count; cnt++){
			for( int i=(mat_dim - 1); i > 0; i--)
				mat_circle[line * mat_dim + i] = mat_circle[line * mat_dim + i - 1];
			mat_circle[line * mat_dim] = tmp;
		}
	}
}

/**
 * @brief Rotates a given column up/down
 */
void mat_rotate_col(uint16_t col, uint16_t dir, uint16_t count) {
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
			// ESP_LOGI("TAG","%6u: %6u %6u", i*mat_dim + j, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim + j].y);
		}
	}
	for( i=0; i< mat_dim; i++)
		mat_rotate_line(i, 1, 2);
}

/**
 * @brief  Setup the display layout
 */
void mat_setup(u8g2_t *u8g2_in, uint16_t resolution, uint16_t divisions) {
	u8g2 = u8g2_in;

	scr_sq_size = resolution;
	mat_dim = divisions;

	mat_init = true;

	mat_circle = malloc(sizeof(circle_t) * mat_dim * mat_dim);

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
	float af = 8.0/300.0;
	float rf;

	if(!mat_init)
		return;

	u8g2_ClearBuffer(u8g2);

	for( i=0; i < mat_dim; i++) {
		for( j=0; j < mat_dim; j++) {
			r = results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * (i * mat_dim + j)];
			rf = (float)r * af;
			r = (uint16_t)rf;
			if (r > 8) r = 8;
			r = 8 - r;
			u8g2_DrawDisc(u8g2, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim +j].y, r, U8G2_DRAW_ALL);
			//u8g2_DrawCircle(u8g2, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim +j].y, r, U8G2_DRAW_ALL);
		}
	}

	u8g2_SendBuffer(u8g2);
}
