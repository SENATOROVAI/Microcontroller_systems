#include "stm32f4xx.h"
#include <stdint.h>

#define LED_MASK_8BIT   (0xFFu)
#define BTN_UP_PIN      (0u)   // PA0
#define BTN_DOWN_PIN    (1u)   // PA1

static void delay_cycles(volatile uint32_t n)
{
    while (n--) {
        __NOP();
    }
}

static void gpio_init(void)
{
    // Enable GPIOA and GPIOC clocks.
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    // PC0..PC7 as outputs (LEDs), push-pull, no pull.
    GPIOC->MODER &= ~(0xFFFFu);
    GPIOC->MODER |=  (0x5555u);
    GPIOC->OTYPER &= ~(LED_MASK_8BIT);
    GPIOC->PUPDR  &= ~(0xFFFFu);

    // PA0 (UP) and PA1 (DOWN) as input with pull-up.
    GPIOA->MODER &= ~((3u << (BTN_UP_PIN * 2u)) | (3u << (BTN_DOWN_PIN * 2u)));
    GPIOA->PUPDR &= ~((3u << (BTN_UP_PIN * 2u)) | (3u << (BTN_DOWN_PIN * 2u)));
    GPIOA->PUPDR |=  ((1u << (BTN_UP_PIN * 2u)) | (1u << (BTN_DOWN_PIN * 2u)));
}

static void leds_write(uint8_t value)
{
    uint32_t odr = GPIOC->ODR;
    odr &= ~LED_MASK_8BIT;
    odr |= value;
    GPIOC->ODR = odr;
}

static uint8_t button_pressed(uint32_t pin)
{
    // Active low because pull-up is enabled.
    return ((GPIOA->IDR & (1u << pin)) == 0u) ? 1u : 0u;
}

int main(void)
{
    uint8_t counter = 0u;
    uint8_t up_prev = 0u;
    uint8_t down_prev = 0u;

    gpio_init();
    leds_write(counter); // Initial state: all LEDs off.

    for (;;) {
        uint8_t up_now = button_pressed(BTN_UP_PIN);
        uint8_t down_now = button_pressed(BTN_DOWN_PIN);

        if ((up_now != 0u) && (up_prev == 0u)) {
            counter++;
            leds_write(counter);
            delay_cycles(120000u); // Debounce delay.
        }

        if ((down_now != 0u) && (down_prev == 0u)) {
            counter--;
            leds_write(counter);
            delay_cycles(120000u); // Debounce delay.
        }

        up_prev = up_now;
        down_prev = down_now;
    }
}
