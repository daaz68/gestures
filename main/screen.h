
#pragma once

#include "u8g2.h"
#include "vl53l8cx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct circle_t {
	uint16_t x,y;
} circle_t;

void mat_idx_to_2d(uint16_t idx, uint16_t *i, uint16_t *j);
void mat_populate(void);
void mat_setup(u8g2_t *u8g2_in, uint16_t resolution, uint16_t divisions);
void mat_plot(VL53L8CX_ResultsData 	results);

#ifdef __cplusplus
}
#endif
