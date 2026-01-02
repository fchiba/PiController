#include "gpio_button.h"
#include "pico/stdlib.h"

void gpio_button_init(void) {
    gpio_init(BUTTON_GPIO_10);
    gpio_set_dir(BUTTON_GPIO_10, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO_10);

    gpio_init(BUTTON_GPIO_15);
    gpio_set_dir(BUTTON_GPIO_15, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO_15);
}

bool gpio_button_10_pressed(void) {
    return !gpio_get(BUTTON_GPIO_10);
}

bool gpio_button_15_pressed(void) {
    return !gpio_get(BUTTON_GPIO_15);
}
