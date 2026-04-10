
#pragma once

#include "u8g2.h"
#include "vl53l8cx_api.h"

#define SCR_SQ_SIZE 128

#define MAT_DIM_8X8  8
#define MAT_DIM_4X4  4
#define MAT_DIM MAT_DIM_4X4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct circle_t {
	uint16_t x,y;
	uint16_t radius;
	uint16_t status;
} circle_t;

typedef circle_t mat_circle_t[MAT_DIM][MAT_DIM];

void mat_idx_to_2d(uint16_t idx, uint16_t *i, uint16_t *j);
void mat_populate(void);
void mat_setup(u8g2_t *u8g2_in, uint16_t resolution, uint16_t divisions);
void mat_plot(VL53L8CX_ResultsData 	results);

#ifdef __cplusplus
}
#endif
