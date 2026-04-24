idf_component_register(
	SRCS
		"gestures.c"
		"screen.c"
	PRIV_REQUIRES esp_driver_i2c esp_driver_gpio u8g2 vl53l8cx
    INCLUDE_DIRS "."
)

