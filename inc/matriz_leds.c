#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "matriz_leds.h"

//Função para ligar um LED
static inline void put_pixel(uint32_t pixel_grb){
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

//Função para converter cores RGB para um valor de 32 bits
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b){
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

bool alertas[5][NUM_PIXELS] = {
//0-20% Primeiro nivel
{
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 0,      
    0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0,   
    0, 0, 0, 0, 0
},
//21-40% Segundo nivel
{
    1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 0,      
    0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0
},
//41-60% Terceiro nivel
{
    1, 1, 1, 1, 1,   
    1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 0,      
    0, 0, 0, 0, 0
},
//61-80% Quarto nivel
{    
    1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1,   
    1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 0
},
//81-100% Quinto e ultimo nivel
{
    1, 1, 1, 1, 1,      
    1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1,   
    1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1
}
};

//Função para envio dos dados para a matriz de leds
void set_one_led(uint8_t r, uint8_t g, uint8_t b, int numero){
    uint32_t color = urgb_u32(r, g, b);
    for(int i = 0; i < NUM_PIXELS; i++){
        if(alertas[numero][i]){
            put_pixel(color);
        } else {
            put_pixel(0);  // Desliga
        }
    }
}