#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "ws2812.pio.h"
#include "inc/matriz_leds.h"

#define WIFI_SSID "PRF"             //Alterar para o SSID da rede
#define WIFI_PASSWORD "@hfs0800"    //Alterar para a senha da rede

#define ADC_MIN 800     //Valor do potenciômetro quando tanque vazio
#define ADC_MAX 4096    //Valor do potenciômetro quando tanque cheio
//Definição de GPIOs
#define RELE_PIN 0     //Pino do rele
#define ADC_NIVEL 0     //ADC do potenciometro
#define I2C_SDA 14      //Pino SDA - Dados
#define I2C_SCL 15      //Pino SCL - Clock
#define WS2812_PIN 7    //Pino do WS2812
#define BUZZER_PIN 21   //Pino do buzzer
#define BOTAO_A 5       //Pino do botao A
#define LED_RED 13      //Pino do LED vermelho
#define LED_GREEN 12    //Pino do LED verde
#define IS_RGBW false   //Maquina PIO para RGBW

uint16_t adc_nivel = 0;  //Variáveis para armazenar os valores do nivel lido no ADC do potenciometro
uint volatile numero = 0;      //Variável para inicializar o numero com 0 (WS2812B)
volatile bool acionar_bomba = false;    //Variável para indicar o modo de monitoramento
uint buzzer_slice;  //Slice para o buzzer

//Prototipagem
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void tratar_requisicao_http(char *request);

//////////////////////////////////////////BASE PRONTA//////////////////////////////////////////////////////////////////////
//Display SSD1306
ssd1306_t ssd;

//Função para modularizar a inicialização do hardware
void inicializar_componentes(){
    stdio_init_all();
    
    //Inicializa LED Vermelho
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, 0);

    //Inicializa LED Verde
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);

    //Inicializa rele
    gpio_init(RELE_PIN);
    gpio_set_dir(RELE_PIN, GPIO_OUT);
    gpio_put(RELE_PIN, 0);

    //Inicializa o pio
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    //Inicializa botao A
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    //Inicializa ADC para leitura do ADC do potenciometro
    adc_init();
    adc_gpio_init(ADC_NIVEL);

    //Inicializa I2C para o display SSD1306
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  //Dados
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  //Clock
    //Define como resistor de pull-up interno
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicializa display
    ssd1306_init(&ssd, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    //Inicializa buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN);   //Slice para o buzzer
    float clkdiv = 125.0f; // Clock divisor
    uint16_t wrap = (uint16_t)((125000000 / (clkdiv * 1000)) - 1);      //Valor do Wrap
    pwm_set_clkdiv(buzzer_slice, clkdiv);       //Define o clock
    pwm_set_wrap(buzzer_slice, wrap);           //Define o wrap
    pwm_set_gpio_level(BUZZER_PIN, wrap * 0.3f); //Define duty
    pwm_set_enabled(buzzer_slice, false); //Começa desligado
}

//WebServer: Início no main()
void iniciar_webserver(){
    if(cyw43_arch_init()) return;   //Inicia o Wi-Fi
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n"); 
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)){    //Conecta ao Wi-Fi - loop
        printf("Falha ao conectar!\n");
        sleep_ms(3000);
    }
    printf("Conectado! IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));    //Conectado, e exibe o IP da rede no serial monitor

    struct tcp_pcb *server = tcp_new(); //Cria o servidor
    tcp_bind(server, IP_ADDR_ANY, 80);  //Binda na porta 80
    server = tcp_listen(server);        //Inicia o servidor
    tcp_accept(server, tcp_server_accept);  //Aceita conexoes
}

//Aceita conexão TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err){
    tcp_recv(newpcb, tcp_server_recv);  //Recebe dados da conexao
    return ERR_OK;
}

//Requisição HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
    if(!p) return tcp_close(tpcb);  // Se nao houver dados, fecha a conexao

    char *request = (char *)malloc(p->len + 1); // Aloca memória para o request
    memcpy(request, p->payload, p->len);        // Copia o request
    request[p->len] = '\0';                     // Terminador de string
    tratar_requisicao_http(request);            // Tratar comandos HTTP

    bool atividade = abs(adc_nivel - 2048) > 500;

    char html[2048];
    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'><title>TITULO</title>"
        "<style>"
        "body {background:#46dd73;font-family:sans-serif;text-align:center;margin-top:30px;}"
        "h1,h2,h3,h4 {margin:10px;}"
        ".btns {display:flex;justify-content:center;gap:10px;flex-wrap:wrap;margin-top:20px;}"
        "button {background:lightgray;font-size:20px;padding:10px 20px;border-radius:8px;border:none;}"
        "input[type=range] {width: 40%%;}"
        "</style></head><body>"
        "<h1>NIVEL TITULO</h1>"
        "</body></html>"
    );

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    free(request);
    pbuf_free(p);
    return ERR_OK;
}

//Trata a requisição HTTP
void tratar_requisicao_http(char *request){
    if (strstr(request, "GET /topico")){ //Verifica se é o request "area_prev"

    }else if(strstr(request, "GET /topico2")){    //Verifica se é o request "area_next"

        }
}

// Função que realiza a leitura e conversão do ADC para nível %
uint8_t ler_nivel_percentual(){
    adc_select_input(ADC_NIVEL);
    adc_nivel = adc_read();

    if(adc_nivel <= ADC_MIN) return 0;
    if(adc_nivel >= ADC_MAX) return 100;

    return((adc_nivel - ADC_MIN) * 100) / (ADC_MAX - ADC_MIN);
}

void atualizar_rgb(uint8_t nivel, uint8_t lim_min){
    if(nivel <= lim_min){
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
    }else if(nivel < 50){
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);  // Amarelo
    }else{
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);
    }
}


//Função que atualiza o display
void atualizar_display(uint8_t nivel, uint8_t lim_min, uint8_t lim_max, bool bomba_ligada){
    char linha1[32], linha2[32], linha3[32], linha4[32];
    ssd1306_fill(&ssd, false);

    sprintf(linha1, "Controle de Nivel");
    sprintf(linha2, "Nivel: %d%%", nivel);
    sprintf(linha3, "Min:%d%% Max:%d%%", lim_min, lim_max);
    sprintf(linha4, "Bomba: %s", bomba_ligada ? "Ligada" : "Deslig.");

    ssd1306_draw_string(&ssd, linha1, 10, 0);
    ssd1306_draw_string(&ssd, linha2, 10, 16);
    ssd1306_draw_string(&ssd, linha3, 10, 32);
    ssd1306_draw_string(&ssd, linha4, 10, 48);

    ssd1306_send_data(&ssd);
}

void verificar_buzzer(uint8_t nivel, uint8_t lim_min, uint8_t lim_max) {
    static absolute_time_t ultimo_alarme = {0};

    if(nivel <= lim_min){
        for(int i = 0; i < 3; i++){
            pwm_set_enabled(buzzer_slice, true);
            sleep_ms(200);
            pwm_set_enabled(buzzer_slice, false);
            sleep_ms(200);
        }
    }else if(nivel >= lim_max && absolute_time_diff_us(get_absolute_time(), ultimo_alarme) > 5000000){
        pwm_set_enabled(buzzer_slice, true);
        sleep_ms(100);
        pwm_set_enabled(buzzer_slice, false);
        ultimo_alarme = get_absolute_time();
    }
}
//Debounce do botão (evita leituras falsas)
bool debounce_botao(uint gpio){
    static uint32_t ultimo_tempo = 0;
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    //Verifica se o botão foi pressionado e se passaram 200ms
    if (gpio_get(gpio) == 0 && (tempo_atual - ultimo_tempo) > 200){ //200ms de debounce
        ultimo_tempo = tempo_atual;
        return true;
    }
    return false;
}

//Função de interrupção com Debouncing
void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    // Caso o botão A seja pressionado
    if(gpio == BOTAO_A && debounce_botao(BOTAO_A)){
        acionar_bomba = true;
    }
}

void atualizar_matriz_leds(uint8_t nivel){
    int faixa = nivel / 20;
    if(faixa > 4) faixa = 4;

    uint8_t r = 0, g = 0, b = 0;

    //Definindo cor da barra com base na faixa
    if (nivel <= 20)        r = 25;  // vermelho
    else if (nivel <= 40)   r = 13, g = 2;  // laranja
    else if (nivel <= 60)   r = 25, g = 25;  // amarelo
    else if (nivel <= 100)   g = 25;  // verde claro

    set_one_led(r, g, b, faixa);
}


int main(){
    inicializar_componentes();  //Inicia os componentes
    iniciar_webserver();        //Inicia o webserver

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    const uint8_t limite_minimo = 20;
    const uint8_t limite_maximo = 80;
    bool bomba_ligada = false;

    while(true){
        uint8_t nivel = ler_nivel_percentual();
        atualizar_display(nivel, limite_minimo, limite_maximo, bomba_ligada);
        atualizar_rgb(nivel, limite_minimo);
        atualizar_matriz_leds(adc_nivel);
        verificar_buzzer(nivel, limite_minimo, limite_maximo);

        if(acionar_bomba && nivel < limite_maximo){
            gpio_put(RELE_PIN, 1);
            bomba_ligada = true;
            while(ler_nivel_percentual() < limite_maximo){
                sleep_ms(200);
            }
            gpio_put(RELE_PIN, 0);
            bomba_ligada = false;
            acionar_bomba = false;
        }

        sleep_ms(1000);
    }
    return 0;
}