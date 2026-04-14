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
 * Helper function to reverse an array in place
 */
void mat_reverse_array(circle_t *arr, int n) {
    for (int i = 0; i < n / 2; i++) {
        circle_t temp = arr[i];
        arr[i] = arr[n - 1 - i];
        arr[n - 1 - i] = temp;
    }
}

/**
 * Rotates a row in a 2D matrix left or right.
 *
 * @param row Pointer to the array (row) to rotate
 * @param len Length of the row
 * @param shift Number of positions to rotate (positive = right, negative = left)
 */
void mat_rotate_row(circle_t *row, int len, int shift) {
    if (len <= 1) return;

    // Normalize shift to be within [0, len-1]
    shift %= len;
    if (shift < 0) {
        shift += len;  // Convert negative shift to equivalent positive right shift
    }

    // If shift is 0, no rotation needed
    if (shift == 0) return;

    // Use the reversal algorithm for efficient O(n) time, O(1) space rotation
    mat_reverse_array(row, len);
    mat_reverse_array(row, shift);
    mat_reverse_array(row + shift, len - shift);
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
		mat_rotate_row(&mat_circle[i*mat_dim],mat_dim,2);
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


#define MIN_RADIUS 14
#define MAX_DISTANCE 300.0
/**
 * @brief Plot the data
 */
void mat_plot(VL53L8CX_ResultsData 	results) {
	uint16_t i, j;
	uint16_t r;
	float af = MIN_RADIUS/MAX_DISTANCE;
	float rf;

	if(!mat_init)
		return;

	u8g2_ClearBuffer(u8g2);

	for( i=0; i < mat_dim; i++) {
		for( j=0; j < mat_dim; j++) {
			r = results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * (i * mat_dim + j)];
			rf = (float)r * af;
			r = (uint16_t)rf;
			if (r > MIN_RADIUS) r = MIN_RADIUS;
			r = MIN_RADIUS - r;
			u8g2_DrawDisc(u8g2, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim +j].y, r, U8G2_DRAW_ALL);
			//u8g2_DrawCircle(u8g2, mat_circle[i*mat_dim + j].x, mat_circle[i*mat_dim +j].y, r, U8G2_DRAW_ALL);
		}
	}

	u8g2_SendBuffer(u8g2);
}
