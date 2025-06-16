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

#define WIFI_SSID "SSID"             //Alterar para o SSID da rede
#define WIFI_PASSWORD "SENHA"    //Alterar para a senha da rede

#define ADC_MIN 800     //Valor do potenciômetro quando tanque vazio
#define ADC_MAX 4096    //Valor do potenciômetro quando tanque cheio
//Definição de GPIOs
#define ADC_NIVEL 0  //ADC do potenciometro
#define I2C_SDA 14      //Pino SDA - Dados
#define I2C_SCL 15      //Pino SCL - Clock
#define IS_RGBW false   //Maquina PIO para RGBW

uint16_t adc_nivel = 0;  //Variáveis para armazenar os valores do nivel lido no ADC do potenciometro

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

int main(){
    inicializar_componentes();  //Inicia os componentes
    iniciar_webserver();        //Inicia o webserver

    while(true){
     
    }
    return 0;
}
