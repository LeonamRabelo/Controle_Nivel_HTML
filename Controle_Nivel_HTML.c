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

#define WIFI_SSID "A35 de Lucas"             //Alterar para o SSID da rede
#define WIFI_PASSWORD "lucaslucas"    //Alterar para a senha da rede

#define ADC_MIN 800     //Valor do potenciômetro quando tanque vazio (FAZER TESTE NO TANQUE VAZIO PARA ACHAR O VALOR IDEAL)
#define ADC_MAX 4096    //Valor do potenciômetro quando tanque cheio
#define CAPACIDADE_LITROS 100   //Capacidade do tanque em litros
//Definição de GPIOs
#define RELE_PIN 18     //Pino do rele
#define ADC_NIVEL 28     //ADC do potenciometro
#define I2C_SDA 14      //Pino SDA - Dados
#define I2C_SCL 15      //Pino SCL - Clock
#define WS2812_PIN 7    //Pino do WS2812
#define BUZZER_PIN 21   //Pino do buzzer
#define BOTAO_A 5       //Pino do botao A
#define LED_RED 13      //Pino do LED vermelho
#define LED_GREEN 12    //Pino do LED verde'
#define IS_RGBW false   //Maquina PIO para RGBW

uint16_t adc_nivel = 0;  //Variáveis para armazenar os valores do nivel lido no ADC do potenciometro
uint16_t volume_litros = 0;
uint volatile numero = 0;      //Variável para inicializar o numero com 0 (WS2812B)
volatile bool acionar_bomba = false;    //Variável para indicar o modo de monitoramento
bool bomba_ligada = false;    //Variável para indicar se a bomba esta ligada
uint buzzer_slice;  //Slice para o buzzer
const char HTML_BODY[] =
"<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'/>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'/>"
"<title>Water Level Monitor</title>"
"<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:sans-serif;background:#e0f7fa;color:#01579b;min-height:100vh}"
"header{background:#1744cb;padding:10px 20px;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap}"
"header img{height:50px}.hdr-txt{display:flex;gap:20px;font-weight:bold;font-size:1.3rem;color:#fff}"
"h1{font-size:2rem;margin:20px auto;text-align:center}.main{display:flex;gap:50px;flex-wrap:wrap;justify-content:center;padding:20px;max-width:1000px;margin:auto}"
".tank,.pump{display:flex;flex-direction:column;align-items:center;max-width:320px;width:100%}"
".tank-box{width:100%;aspect-ratio:1;border:4px solid #0288d1;border-radius:16px;background:#b3e5fc;position:relative;margin-bottom:15px;overflow:hidden}"
".wave-wrapper{position:absolute;bottom:0;left:0;width:100%;height:100%;overflow:hidden}"
".wave-container{width:100%;height:0;position:absolute;bottom:0;left:0;overflow:hidden;transition:height 0.5s ease-in-out}"
".wave-container svg{width:100%;height:100%;position:absolute;bottom:0}"
".w1{fill:#0288d1;opacity:1}.w2{fill:#4fc3f7;opacity:0.6}"
".waves1,.waves2{display:flex;width:2400px;height:100%}"
".waves1{animation:moveWaves 3s linear infinite}.waves2{animation:moveWaves 6s linear infinite}"
"@keyframes moveWaves{0%{transform:translateX(0)}100%{transform:translateX(-1200px)}}"
".tank p,.pump p{margin:5px 0;font-size:1.1rem;text-align:center}"
".pump-box{width:100%;aspect-ratio:1;border-radius:16px;background:white;border:6px solid red;display:flex;justify-content:center;align-items:center;margin-bottom:10px;transition:border-color 0.3s}"
".pump-box.on{border-color:green}.pump-box.on img{animation:shake 0.2s infinite}"
".pump-box img{width:90%;object-fit:contain}.status.on{color:green;font-weight:bold}.status.off{color:red;font-weight:bold}"
"button{padding:10px 20px;background:#0288d1;border:none;color:white;border-radius:8px;font-size:1rem;cursor:pointer;margin-top:10px}"
"button:hover{background:#0277bd}#last-update{margin-top:30px;font-size:1.2rem;font-weight:bold;text-align:center}"
"@keyframes shake{0%,100%{transform:translate(1px,-2px)}25%{transform:translate(-2px,1px)}50%{transform:translate(2px,-1px)}75%{transform:translate(-1px,2px)}}"
"@media(max-width:768px){.main{flex-direction:column;align-items:center}.hdr-txt{justify-content:center;width:100%;font-size:1.1rem;margin-top:10px}header{flex-direction:column;align-items:center}}"
"</style></head><body>"
"<header><img src='https://i.imgur.com/wVCmCfn.png' alt='RESTIC Logo'/>"
"<div class='hdr-txt'><span>RESTIC 37</span><span>CEPEDI</span></div></header>"
"<h1>Water Level Monitoring System</h1><div class='main'>"
"<div class='tank'><div class='tank-box'>"
"<div class='wave-wrapper'><div class='wave-container' id='wave'>"
"<svg viewBox='0 0 1200 200' preserveAspectRatio='xMidYMax slice'>"
"<g class='waves1'><path class='w1' d='M0,100 C300,50 900,150 1200,100 V200 H0 Z'/>"
"<path class='w1' d='M1200,100 C1500,50 2100,150 2400,100 V200 H1200 Z'/></g>"
"<g class='waves2'><path class='w2' d='M0,100 C300,50 900,150 1200,100 V200 H0 Z'/>"
"<path class='w2' d='M1200,100 C1500,50 2100,150 2400,100 V200 H1200 Z'/></g>"
"</svg></div></div></div>"
"<p><strong>Level:</strong> <span id='level'>--%</span></p>"
"<p><strong>Volume:</strong> <span id='volume'>--L</span></p></div>"
"<div class='pump'><div id='pump-box' class='pump-box'><img src='https://i.imgur.com/sucmfnd.png' alt='Water Pump'></div>"
"<p><strong>Pump:</strong> <span id='pump-status' class='status'>--</span></p>"
"<button id='toggle-pump'>Toggle Pump</button></div></div>"
"<p id='last-update'>Last update: --:--</p>"
"<script>"
"function togglePump(){fetch('/toggle',{method:'POST'})}"
"function atualizar(){fetch('/data').then(res=>res.json()).then(data=>{"
"const level = data.lvl;"
"document.getElementById('wave').style.height = `${level*2}%`;"
"document.getElementById('level').textContent=`${level}%`;"
"document.getElementById('volume').textContent=`${(level*10).toFixed(0)}L`;"
"const pump=document.getElementById('pump-status'),box=document.getElementById('pump-box'),isOn=data.pump;"
"pump.textContent=isOn?'On':'Off';pump.className='status '+(isOn?'on':'off');"
"box.className='pump-box '+(isOn?'on':'');"
"document.getElementById('last-update').textContent='Last update: '+new Date().toLocaleTimeString();"
"}).catch(e=>console.error('Erro ao atualizar:',e))}"
"setInterval(atualizar,1000);atualizar();"
"document.getElementById('toggle-pump').addEventListener('click',togglePump);"
"</script></body></html>";

//Prototipagem
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
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
    tcp_recv(newpcb, http_recv);  //Recebe dados da conexao
    return ERR_OK;
}

//Requisição HTTP
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;

    const char *header_html =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n";

    const char *header_json =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (p->len >= 6 && strncmp(req, "GET / ", 6) == 0) {
        tcp_write(tpcb, header_html, strlen(header_html), TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, HTML_BODY, strlen(HTML_BODY), TCP_WRITE_FLAG_COPY);
    } else if (p->len >= 9 && strncmp(req, "GET /data", 9) == 0) {
        char json[64];
        snprintf(json, sizeof(json), "{\"lvl\":%d,\"pump\":%s}", volume_litros, bomba_ligada ? "true" : "false");
        tcp_write(tpcb, header_json, strlen(header_json), TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, json, strlen(json), TCP_WRITE_FLAG_COPY);
    } else if (strstr(req, "POST /toggle") != NULL) {
        bomba_ligada = !bomba_ligada;
        char json[64];
        snprintf(json, sizeof(json), "{\"lvl\":%d,\"pump\":%s}", volume_litros, bomba_ligada ? "true" : "false");
        tcp_write(tpcb, header_json, strlen(header_json), TCP_WRITE_FLAG_COPY);
        tcp_write(tpcb, json, strlen(json), TCP_WRITE_FLAG_COPY);
    } else {
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Recurso não encontrado.";
        tcp_write(tpcb, not_found, strlen(not_found), TCP_WRITE_FLAG_COPY);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    tcp_close(tpcb);  // Fecha a conexão após envio da resposta
    return ERR_OK;
}

//Trata a requisição HTTP
void tratar_requisicao_http(char *request){
    if (strstr(request, "GET /topico")){ //Verifica se é o request "area_prev"

    }else if(strstr(request, "GET /topico2")){    //Verifica se é o request "area_next"

        }
}

//Função que realiza a leitura e conversão do ADC para nível %
uint8_t ler_nivel_percentual(){
    adc_select_input(2);    //Seleciona o potenciometro, canal 2 da GPIO 28
    adc_nivel = adc_read(); //Le o ADC e armazena na variavel global adc_nivel

    //Garante que o ADC nao esteja fora dos limites
    if(adc_nivel <= ADC_MIN) return 0;
    if(adc_nivel >= ADC_MAX) return 100;

    return((adc_nivel - ADC_MIN) * 100) / (ADC_MAX - ADC_MIN);  //Converte para porcentagem
}

//Função que atualiza o RGB
void atualizar_rgb(uint8_t nivel, uint8_t lim_min){
    if(nivel <= lim_min){   //Se o nivel for menor que o limite minimo
        gpio_put(LED_RED, 1);   //Liga o LED vermelho
        gpio_put(LED_GREEN, 0);
    }else if(nivel < 50){   //Se o nivel for menor que 50
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);  //Liga a cor amarela (vermelho + verde)
    }else{  //Se o nivel for maior que 50
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);  //Liga o LED verde
    }
}


//Função que atualiza o display
void atualizar_display(uint8_t nivel, uint8_t lim_min, uint8_t lim_max, bool bomba_ligada, uint16_t volume_litros){
    char buffer[32];    //Buffer para armazenar as informacoes a serem exibidas no display
    ssd1306_fill(&ssd, false);  //Limpa o display
    
    //Borda
    ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);
    ssd1306_rect(&ssd, 1, 1, 128 - 2, 64 - 2, true, false);
    ssd1306_rect(&ssd, 2, 2, 128 - 4, 64 - 4, true, false);
    ssd1306_rect(&ssd, 3, 3, 128 - 6, 64 - 6, true, false);

    //Desenha as informacoes
    ssd1306_draw_string(&ssd, "Controle de Nivel", 20, 0);
    sprintf(buffer, "Nivel: %d%%", nivel);
    ssd1306_draw_string(&ssd, buffer, 10, 10);
    sprintf(buffer, "Volume: %dL", volume_litros);
    ssd1306_draw_string(&ssd, buffer, 10, 25);
    sprintf(buffer, "Min:%d%% Max:%d%%", lim_min, lim_max);
    ssd1306_draw_string(&ssd, buffer, 10, 35);
    sprintf(buffer, "Bomba: %s", bomba_ligada ? "Ligada" : "Deslig.");
    ssd1306_draw_string(&ssd, buffer, 10, 45);

    ssd1306_send_data(&ssd);    //Envia os dados para o display
}

//Função que verifica o buzzer
void verificar_buzzer(uint8_t nivel, uint8_t lim_min, uint8_t lim_max){
    static absolute_time_t ultimo_alarme = {0}; //Variavel para armazenar o ultimo alarme

    if(nivel <= lim_min){   //Se o nivel for menor que o limite minimo
        for(int i = 0; i < 3; i++){ //Repete 3 vezes
            pwm_set_enabled(buzzer_slice, true);
            sleep_ms(200);
            pwm_set_enabled(buzzer_slice, false);
            sleep_ms(200);
        }
    }else if(nivel >= lim_max && absolute_time_diff_us(get_absolute_time(), ultimo_alarme) > 5000000){  //Se o nivel for maior que o limite maximo
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
    uint32_t current_time = to_ms_since_boot(get_absolute_time());      //Variavel para armazenar o tempo atual
    //Caso o botão A seja pressionado
    if(gpio == BOTAO_A && debounce_botao(BOTAO_A)){
        acionar_bomba = true;     //Variavel de verificação para ligar a bomba
    }
}

//Função que atualiza a matriz de leds
void atualizar_matriz_leds(uint8_t nivel){
    int faixa = nivel / 20;     //Divide o nivel por 20, verficando em qual faixa ele pertence, a cada 20% de nivel tem uma faixa
    if(faixa > 4) faixa = 4;    //Limita o numero de faixas

    uint8_t r = 0, g = 0, b = 0;    //Definindo variaveis para cores RGB

    //Definindo cor da barra com base na faixa
    if (nivel <= 20)        r = 25;  //Vermelho
    else if (nivel <= 40)   r = 13, g = 2;  //Laranja
    else if (nivel <= 60)   r = 25, g = 25;  //Amarelo
    else if (nivel <= 100)   g = 25;  //Verde

    set_one_led(r, g, b, faixa);    //Envia os dados para a matriz
}

int converter_em_litros(int nivel_porcentagem){
    volume_litros = (nivel_porcentagem * 100.0f) * CAPACIDADE_LITROS;
    return volume_litros;
}

int main(){
    inicializar_componentes();  //Inicia os componentes
    iniciar_webserver();        //Inicia o webserver
    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    printf("IP: %s\n", ip_str);  //Exibe o IP no serial monitor

    //Inicia a interrupção do botão A
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    //Definindo os limites de nivel padrao
    const uint8_t limite_minimo = 20;
    const uint8_t limite_maximo = 80;

    while(true){
        uint8_t nivel = ler_nivel_percentual(); //Le o ADC e armazena na variavel
        uint8_t volume = converter_em_litros(nivel);
        atualizar_display(nivel, limite_minimo, limite_maximo, bomba_ligada, volume);   //Atualiza o display com as informacoes atualizadas
        atualizar_rgb(nivel, limite_minimo);                                    //Atualiza o RGB
        atualizar_matriz_leds(adc_nivel);                                       //Atualiza a matriz de leds
        verificar_buzzer(nivel, limite_minimo, limite_maximo);                  //Verifica o buzzer

        //Controle da bomba
        if(acionar_bomba && nivel < limite_maximo){ //Se o botão A for pressionado e o nivel for menor que o limite maximo
            gpio_put(RELE_PIN, 1);                  //Liga a bomba
            bomba_ligada = true;                    //Indica que a bomba esta ligada
            while(ler_nivel_percentual() < limite_maximo){  //Enquanto o nivel for menor que o limite maximo
                sleep_ms(200);
            }
            gpio_put(RELE_PIN, 0);                          //Desliga a bomba
            bomba_ligada = false;                           //Indica que a bomba esta desligada
            acionar_bomba = false;                          //Indica que o botão A foi liberado
        }
        sleep_ms(1000);
    }
    return 0;
}