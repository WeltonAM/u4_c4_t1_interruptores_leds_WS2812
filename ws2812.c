#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define LED_RED 13
#define LED_GREEN 12
#define LED_BLUE 11
#define BUTTON_A 5
#define BUTTON_B 6
#define WS2812_PIN 7
#define NUM_PIXELS 25

static volatile uint32_t last_time_A = 0;
static volatile uint32_t last_time_B = 0;
static volatile int num_displayed = 0;

PIO pio = pio0;
int sm = 0;
uint32_t led_buffer[NUM_PIXELS] = {0};

// Representações dos números de 0 a 9 usando a matriz 5x5 de LEDs
int num_map[10][NUM_PIXELS] = {
    {0, 1, 1, 1, 0, // 0
     0, 1, 0, 1, 0,
     0, 1, 0, 1, 0,
     0, 1, 0, 1, 0,
     0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, // 1
     0, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     0, 1, 1, 0, 0,
     0, 0, 1, 0, 0},

    {0, 1, 1, 1, 0, // 2
     0, 1, 0, 0, 0,
     0, 1, 1, 1, 0,
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, // 3
     0, 0, 0, 1, 0,
     0, 1, 1, 0, 0,
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 0},

    {0, 1, 0, 0, 0, // 4
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 0,
     0, 1, 0, 1, 0,
     0, 1, 0, 1, 0},

    {0, 1, 1, 1, 0, // 5
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 0,
     0, 1, 0, 0, 0,
     0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, // 6
     0, 1, 0, 1, 0,
     0, 1, 1, 1, 0,
     0, 1, 0, 0, 0,
     0, 1, 1, 1, 0},

    {0, 1, 0, 0, 0, // 7
     0, 0, 0, 1, 0,
     0, 1, 0, 0, 0,
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, // 8
     0, 1, 0, 1, 0,
     0, 1, 1, 1, 0,
     0, 1, 0, 1, 0,
     0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, // 9
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 0,
     0, 1, 0, 1, 0,
     0, 1, 1, 1, 0}};

// Função para enviar um pixel para o WS2812
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

// Função que converte as cores para o formato GRB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Função que gera uma cor aleatória
static inline uint32_t random_color()
{
    uint8_t r = rand() % 20;
    uint8_t g = rand() % 20;
    uint8_t b = rand() % 20;
    return urgb_u32(r, g, b);
}

// Função que define os LEDs da matriz a partir do buffer
void set_leds_from_buffer()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        put_pixel(led_buffer[i]);
    }
}

// Função que exibe o número na matriz de LEDs e gera cores aleatórias
void display_number(int num)
{
    // Limpar todos os LEDs (para garantir que a matriz será limpa)
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        led_buffer[i] = urgb_u32(0, 0, 0); // Todos os LEDs apagados (off)
    }

    // Preencher os LEDs com base no número a ser exibido e gerar cores aleatórias para os LEDs que representam o número
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (num_map[num][i] == 1)
        {
            // Atribui uma cor aleatória ao LED que representa o número
            led_buffer[i] = random_color();
            // led_buffer[i] = urgb_u32(255, 0, 0);
        }
    }

    set_leds_from_buffer(); // Atualiza os LEDs da matriz
}

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Debouncing: se o tempo desde o último evento for suficiente, processa o evento
    if (gpio == BUTTON_A && current_time - last_time_A > 200000)
    {
        last_time_A = current_time;
        num_displayed = (num_displayed + 1) % 10; // Incrementa o número exibido
    }
    if (gpio == BUTTON_B && current_time - last_time_B > 200000)
    {
        last_time_B = current_time;
        num_displayed = (num_displayed - 1 + 10) % 10; // Decrementa o número exibido
    }

    display_number(num_displayed); // Exibe o número na matriz de LEDs
}

int main()
{
    stdio_init_all();

    // Inicializa o LED RGB
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    // Inicializa os botões com pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Inicializa o WS2812
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    // Configuração das interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Loop principal: LED vermelho pisca a 5 Hz
    while (1)
    {
        gpio_put(LED_RED, true);  // LED vermelho ligado
        sleep_ms(100);            // Atraso de 100 ms (5 Hz)
        gpio_put(LED_RED, false); // LED vermelho desligado
        sleep_ms(100);            // Atraso de 100 ms (5 Hz)
    }

    return 0;
}
