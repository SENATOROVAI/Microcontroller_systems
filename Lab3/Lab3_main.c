#include "stm32f4xx.h"
#include <stdint.h>

#ifndef LAB_VARIANT
#define LAB_VARIANT 5u
#endif

#define BTN_SEND_PIN     (0u)   // PA0
#define LED_CONTROL_PIN  (0u)   // PC0

#define USART_TX_PIN     (2u)   // PA2 (USART2_TX, AF7)
#define USART_RX_PIN     (3u)   // PA3 (USART2_RX, AF7)

static volatile const uint8_t *g_tx_ptr = 0;
static volatile uint16_t g_tx_len = 0u;
static volatile uint16_t g_tx_pos = 0u;

static uint32_t target_freq_mhz_from_variant(uint32_t variant)
{
    if (variant < 1u) {
        return 84u;
    }
    if (variant > 41u) {
        return 44u;
    }
    return 85u - variant;
}

static void delay_cycles(volatile uint32_t n)
{
    while (n--) {
        __NOP();
    }
}

static void set_flash_latency(uint32_t sysclk_mhz)
{
    uint32_t latency;

    if (sysclk_mhz <= 30u) {
        latency = FLASH_ACR_LATENCY_0WS;
    } else if (sysclk_mhz <= 60u) {
        latency = FLASH_ACR_LATENCY_1WS;
    } else {
        latency = FLASH_ACR_LATENCY_2WS;
    }

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | latency;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != latency) {
    }
}

static void clock_init_pll_hsi(uint32_t target_mhz)
{
    uint32_t pllm = 16u;
    uint32_t pllp = 4u;
    uint32_t plln;

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0u) {
    }

    RCC->CFGR &= ~RCC_CFGR_SW;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
    }

    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0u) {
    }

    if ((target_mhz * 4u) >= 192u) {
        pllp = 4u;
    } else {
        pllp = 6u;
    }
    plln = target_mhz * pllp;

    set_flash_latency(target_mhz);

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    RCC->PLLCFGR = (pllm & RCC_PLLCFGR_PLLM)
                 | ((plln << RCC_PLLCFGR_PLLN_Pos) & RCC_PLLCFGR_PLLN)
                 | ((((pllp / 2u) - 1u) << RCC_PLLCFGR_PLLP_Pos) & RCC_PLLCFGR_PLLP)
                 | RCC_PLLCFGR_PLLSRC_HSI;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0u) {
    }

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }
}

static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    // PA0 input with pull-up (button to start TX sending).
    GPIOA->MODER &= ~(3u << (BTN_SEND_PIN * 2u));
    GPIOA->PUPDR &= ~(3u << (BTN_SEND_PIN * 2u));
    GPIOA->PUPDR |=  (1u << (BTN_SEND_PIN * 2u));

    // PC0 output (LED controlled by RX byte values).
    GPIOC->MODER &= ~(3u << (LED_CONTROL_PIN * 2u));
    GPIOC->MODER |=  (1u << (LED_CONTROL_PIN * 2u));
    GPIOC->OTYPER &= ~(1u << LED_CONTROL_PIN);
    GPIOC->PUPDR  &= ~(3u << (LED_CONTROL_PIN * 2u));
}

static void usart2_gpio_init(void)
{
    // PA2, PA3 -> AF7 for USART2.
    GPIOA->MODER &= ~((3u << (USART_TX_PIN * 2u)) | (3u << (USART_RX_PIN * 2u)));
    GPIOA->MODER |=  ((2u << (USART_TX_PIN * 2u)) | (2u << (USART_RX_PIN * 2u)));

    GPIOA->AFR[0] &= ~((0xFu << (USART_TX_PIN * 4u)) | (0xFu << (USART_RX_PIN * 4u)));
    GPIOA->AFR[0] |=  ((7u   << (USART_TX_PIN * 4u)) | (7u   << (USART_RX_PIN * 4u)));

    GPIOA->OSPEEDR |= ((2u << (USART_TX_PIN * 2u)) | (2u << (USART_RX_PIN * 2u)));
    GPIOA->PUPDR   &= ~((3u << (USART_TX_PIN * 2u)) | (3u << (USART_RX_PIN * 2u)));
    GPIOA->PUPDR   |=  ((1u << (USART_TX_PIN * 2u)) | (1u << (USART_RX_PIN * 2u)));
}

static void usart2_init(uint32_t pclk1_hz, uint32_t baud)
{
    // Enable USART2 clock.
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    // 16x oversampling, BRR = pclk/baud (integer part enough for 9600 demo).
    USART2->CR1 = 0u;
    USART2->CR2 = 0u;
    USART2->CR3 = 0u;
    USART2->BRR = (pclk1_hz + (baud / 2u)) / baud;

    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    NVIC_SetPriority(USART2_IRQn, 2u);
    NVIC_EnableIRQ(USART2_IRQn);
}

static uint8_t variant_off_value(void)
{
    switch (LAB_VARIANT) {
        case 1:  return '0';
        case 2:  return 'a';
        case 3:  return '2';
        case 4:  return 'c';
        case 5:  return '4';
        case 6:  return 'e';
        case 7:  return '6';
        case 8:  return 'g';
        case 9:  return '8';
        case 10: return 'i';
        case 11: return 10u;
        case 12: return 'k';
        case 13: return 12u;
        case 14: return 'm';
        case 15: return 14u;
        case 16: return 'o';
        case 17: return 'q';
        case 18: return 's';
        case 19: return 'u';
        case 20: return 'w';
        case 21: return 'y';
        case 22: return 'A';
        case 23: return 'C';
        case 24: return 'E';
        case 25: return 'G';
        case 26: return 'I';
        case 27: return 'K';
        case 28: return 'M';
        case 29: return 'O';
        case 30: return 'Q';
        case 31: return 'S';
        case 32: return 'U';
        case 33: return 'W';
        case 34: return 'Y';
        case 35: return ' ';
        case 36: return '(';
        case 37: return '.';
        case 38: return ':';
        case 39: return '$';
        case 40: return '&';
        case 41: return '+';
        default: return '0';
    }
}

static uint8_t variant_on_value(void)
{
    switch (LAB_VARIANT) {
        case 1:  return '1';
        case 2:  return 'b';
        case 3:  return '3';
        case 4:  return 'd';
        case 5:  return '5';
        case 6:  return 'f';
        case 7:  return '7';
        case 8:  return 'h';
        case 9:  return '9';
        case 10: return 'j';
        case 11: return 11u;
        case 12: return 'l';
        case 13: return 13u;
        case 14: return 'n';
        case 15: return 15u;
        case 16: return 'p';
        case 17: return 'r';
        case 18: return 't';
        case 19: return 'v';
        case 20: return 'x';
        case 21: return 'z';
        case 22: return 'B';
        case 23: return 'D';
        case 24: return 'F';
        case 25: return 'H';
        case 26: return 'J';
        case 27: return 'L';
        case 28: return 'N';
        case 29: return 'P';
        case 30: return 'R';
        case 31: return 'T';
        case 32: return 'V';
        case 33: return 'X';
        case 34: return 'Z';
        case 35: return '!';
        case 36: return ')';
        case 37: return ',';
        case 38: return ';';
        case 39: return '%';
        case 40: return '*';
        case 41: return '-';
        default: return '1';
    }
}

static void led_set(uint8_t on)
{
    if (on != 0u) {
        GPIOC->BSRR = (1u << LED_CONTROL_PIN);
    } else {
        GPIOC->BSRR = (1u << (LED_CONTROL_PIN + 16u));
    }
}

static void usart_send_packet_irq(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0u)) {
        return;
    }

    g_tx_ptr = data;
    g_tx_len = len;
    g_tx_pos = 0u;

    // Start TX by enabling TXE interrupt.
    USART2->CR1 |= USART_CR1_TXEIE;
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;

    if ((sr & USART_SR_RXNE) != 0u) {
        uint8_t rx = (uint8_t)USART2->DR;

        if (rx == variant_off_value()) {
            led_set(0u);
        } else if (rx == variant_on_value()) {
            led_set(1u);
        }
    }

    if (((sr & USART_SR_TXE) != 0u) && ((USART2->CR1 & USART_CR1_TXEIE) != 0u)) {
        if (g_tx_pos < g_tx_len) {
            USART2->DR = g_tx_ptr[g_tx_pos++];
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

static uint8_t send_button_pressed(void)
{
    return ((GPIOA->IDR & (1u << BTN_SEND_PIN)) == 0u) ? 1u : 0u;
}

int main(void)
{
    static const uint8_t demo_packet[] = "USART IRQ packet\r\n";
    const uint32_t sysclk_mhz = target_freq_mhz_from_variant(LAB_VARIANT);
    uint8_t btn_prev = 0u;

    clock_init_pll_hsi(sysclk_mhz);
    gpio_init();
    usart2_gpio_init();

    // APB1 = SYSCLK/2 in this setup.
    usart2_init((sysclk_mhz * 1000000u) / 2u, 9600u);

    led_set(0u);

    for (;;) {
        uint8_t btn_now = send_button_pressed();

        if ((btn_now != 0u) && (btn_prev == 0u)) {
            usart_send_packet_irq(demo_packet, (uint16_t)(sizeof(demo_packet) - 1u));
            delay_cycles(160000u);
        }

        btn_prev = btn_now;
    }
}
