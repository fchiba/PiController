#ifndef GPIO_BUTTON_H
#define GPIO_BUTTON_H

#include <stdbool.h>

#define BUTTON_GPIO_10  10
#define BUTTON_GPIO_15  15

void gpio_button_init(void);
bool gpio_button_10_pressed(void);
bool gpio_button_15_pressed(void);

#endif
