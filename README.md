# Sistema de Controle de Nível com Interface Web – RESTIC 37

## Visão Geral

Este projeto consiste em um sistema embarcado que realiza o monitoramento e controle automático de um reservatório de água, utilizando uma boia acoplada a um potenciômetro, interface web via Wi-Fi, e os recursos da plataforma BitDogLab (display OLED, buzzer, matriz de LEDs, LEDs RGB, botão físico). A bomba de água é acionada automaticamente ou manualmente conforme o nível lido e os limites definidos.

---

## Objetivos

- Medir continuamente o nível de água com sensor analógico (potenciômetro).
- Exibir informações locais no display OLED e interface web.
- Permitir controle da bomba automaticamente ou por botão.
- Indicar alertas com buzzer, LEDs e matriz WS2812.
- Permitir reconfiguração dos limites mínimos e máximos remotamente.

---

## Hardware Utilizado

- Raspberry Pi Pico W
- Display OLED SSD1306 (I2C)
- Módulo WS2812 (matriz de LEDs RGB)
- Potenciômetro com haste e boia de isopor
- Módulo Relé para bomba d'água
- LEDs RGB (vermelho e verde)
- Buzzer (PWM)
- Botão físico (pino 5)
- Conexão Wi-Fi

---

## Funcionalidades

### 1. Medição do Nível

- A boia conectada a um potenciômetro converte o nível de água em um valor analógico.
- O valor lido é convertido em porcentagem (0–100%) e em volume (litros).
- Função: `ler_nivel_percentual()` + `converter_em_litros()`.

### 2. Controle da Bomba

- **Modo automático**: a bomba liga se o nível estiver abaixo do limite mínimo e desliga ao atingir o máximo.
- **Modo manual**: botão físico permite acionamento da bomba, desde que abaixo do limite máximo.

### 3. Interface Web

- A página HTML é servida diretamente pela Pico W.
- Permite:
  - Visualizar nível atual (porcentagem e volume em litros)
  - Estado da bomba
  - Definir novos limites mínimo/máximo (em tempo real)
  - Acionar bomba manualmente

### 4. Interface Local

- **Display OLED**:
  - Nome do sistema
  - Nível (%)
  - Volume (litros)
  - Limites configurados
  - Estado da bomba

- **Matriz de LEDs WS2812**:
  - Acende linhas conforme a faixa de nível (5 faixas: 0–20%, ..., 81–100%)

- **LEDs RGB**:
  - Vermelho: nível abaixo do limite mínimo
  - Verde: nível cheio
  - Amarelo: entre 20% e 50%

- **Buzzer**:
  - 3 toques curtos: nível crítico
  - 1 toque longo periódico: nível cheio

---

## Mapeamento de GPIOs

| Função            | GPIO Pico |
|-------------------|-----------|
| Potenciômetro     | 28 (ADC2) |
| Relé da Bomba     | 18        |
| Buzzer            | 21 (PWM)  |
| LED Verde         | 12        |
| LED Vermelho      | 13        |
| WS2812            | 7         |
| Botão (A)         | 5         |
| I2C SDA (Display) | 14        |
| I2C SCL (Display) | 15        |

---

## Estrutura de Código

- `main()`: inicializa periféricos, Wi-Fi, servidor HTTP e loop principal.
- `http_recv()`: processa requisições HTML e JSON da interface web.
- `gpio_irq_handler()`: trata o botão com interrupção e debounce.
- `atualizar_display()`, `atualizar_rgb()`, `atualizar_matriz_leds()`, `verificar_buzzer()`: atualizam os periféricos.

---

## Interface Web

### Requisições

- `GET /`: retorna a interface HTML principal
- `GET /data`: retorna nível atual e estado da bomba em JSON
- `POST /toggle`: alterna o estado da bomba manualmente
- `POST /set-limits`: atualiza os valores de limite mínimo/máximo

---

## Instruções de Uso

1. **Edite os dados da sua rede** no código:
   ```c
   #define WIFI_SSID "SEU_SSID"
   #define WIFI_PASSWORD "SUA_SENHA"

2. Compile e grave na Pico W com o SDK C da Raspberry Pi.

3. Abra o navegador e acesse o IP exibido no serial monitor.

4. Visualize e interaja com o sistema pela interface web.

---

## Autor

Desenvolvido pela equipe composta por Gabriel Marques, Leonam Rabelo e Lucas Ferreira, como parte do programa de residência em sistemas embarcados da CEPEDI — RESTIC 37 (2025).