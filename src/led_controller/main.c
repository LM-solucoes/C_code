/*
 * Projeto: Controlador LED WS2812 Duplo para Honda CBR650R
 * Arquivo: main.c
 * Autor: LM-solucoes
 * Data: 20/08/2025
 * Versão: 1.0
 * 
 * Descrição: Sistema de controle de duas fitas LED WS2812 (150 LEDs cada)
 * com múltiplos modos de operação (RPM, Stops, Piscas)
 * Microcontrolador: PIC18F2680 @ 32MHz (8MHz + PLL)
 */


/// Configura o oscilador interno a 8 MHz
#define _XTAL_FREQ 32000000  // Frequência do oscilador (exemplo 16MHz interno ajustado)


#include <xc.h>
#include <stdint.h>


// CONFIG1H
#pragma config OSC = IRCIO67     // Oscilador interno, pinos RA6/RA7 como I/O digitais
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor OFF
#pragma config IESO = OFF
// Oscilador interno/externo OFF
// CONFIG2L
#pragma config PWRT = ON        // Power-up Timer ON
#pragma config BOREN = OFF      // Brown-out Reset OFF
// CONFIG2H
#pragma config WDT = OFF      // Watchdog Timer OFF
// CONFIG3H
#pragma config PBADEN = OFF     // PORTB<5:0> como digital no reset
#pragma config LPT1OSC = OFF
#pragma config MCLRE = OFF       // MCLR como reset

// CONFIG4L
#pragma config STVREN = OFF
#pragma config LVP = OFF        // Low-voltage programming OFF
#pragma config XINST = OFF      // Instruções estendidas OFF

#define LED_PIN LATCbits.LATC0
#define LED_TRIS TRISCbits.TRISC0
#define HB_LED LATCbits.LATC1
#define HB_TRIS TRISCbits.TRISC1

#define NUM_LEDS 150UL//300
#define RPM_MAX 12000UL

// Buffer de LEDs: 300 LEDs, 3 bytes cada (R, G, B)
uint8_t led_buffer[NUM_LEDS][3];

// Estrutura de cor para gradiente

#define NUM_COLORS 6

typedef struct {
    uint8_t r, g, b;
    uint8_t position; // De 0 a 100
} ColorPoint;

const ColorPoint gradient[NUM_COLORS] = {
    {0, 0, 255, 0}, // Azul
    {0, 255, 255, 20}, // Ciano
    {0, 255, 0, 40}, // Verde
    {255, 255, 0, 60}, // Amarelo
    {255, 165, 0, 80}, // Laranja
    {255, 0, 0, 100} // Vermelho
};

// ==========================
// WS2812 LOW LEVEL
// ==========================

void ws2812_sendBit(uint8_t bitVal) {
    if (bitVal) {
        // Bit 1: HIGH por ~0.8µs, LOW por ~0.45µs
        LED_PIN = 1;
        __delay_us(0.8);
        LED_PIN = 0;
        __delay_us(0.45);
    } else {
        // Bit 0: HIGH por ~0.4µs, LOW por ~0.85µs
        LED_PIN = 1;
        __delay_us(0.4);
        LED_PIN = 0;
        __delay_us(0.85);
    }
}

void ws2812_sendByte(uint8_t byte) {
    for (int8_t i = 7; i >= 0; i--) {
        ws2812_sendBit((byte >> i) & 0x01);
    }
}

void ws2812_sendPixel(uint8_t r, uint8_t g, uint8_t b) {
    // Ordem GRB exigida pelos WS2812
    ws2812_sendByte(g);
    ws2812_sendByte(r);
    ws2812_sendByte(b);
}


// Função para enviar reset (pausa longa) - VERSÃO CORRIGIDA

void ws2812_reset(void) {
    LED_PIN = 0;
    // Substituir __delay_us() por loop manual
    for (volatile int i = 0; i < 400; i++) {
        NOP(); // ~50µs com 8MHz
    }
}
// ==========================
// Cores do gradiente por %
// ==========================

void getGradientColor(unsigned int percent, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (percent > 100) percent = 100;

    for (int i = 0; i < NUM_COLORS - 1; i++) {
        if (percent >= gradient[i].position && percent <= gradient[i + 1].position) {
            unsigned int range = gradient[i + 1].position - gradient[i].position;
            unsigned int pos = percent - gradient[i].position;

            *r = gradient[i].r + (((int32_t) (gradient[i + 1].r - gradient[i].r) * pos) / range);
            *g = gradient[i].g + (((int32_t) (gradient[i + 1].g - gradient[i].g) * pos) / range);
            *b = gradient[i].b + (((int32_t) (gradient[i + 1].b - gradient[i].b) * pos) / range);
            return;
        }
    }

    *r = gradient[NUM_COLORS - 1].r;
    *g = gradient[NUM_COLORS - 1].g;
    *b = gradient[NUM_COLORS - 1].b;
}

// ==========================
// Atualiza o array de LEDs baseado no RPM
// ==========================

void fillStripByRPM(unsigned int rpm) {
    unsigned int rpm_percent = ((unsigned long) rpm * 100UL) / RPM_MAX;
    unsigned int leds_on = ((unsigned long) rpm * NUM_LEDS) / RPM_MAX;

    if (leds_on > NUM_LEDS) leds_on = NUM_LEDS;

    uint8_t r = 0, g = 0, b = 0;
    getGradientColor(rpm_percent, &r, &g, &b);

    for (int i = 0; i < NUM_LEDS; i++) {
        if (i < leds_on) {
            led_buffer[i][0] = r;
            led_buffer[i][1] = g;
            led_buffer[i][2] = b;
        } else {
            led_buffer[i][0] = 0;
            led_buffer[i][1] = 0;
            led_buffer[i][2] = 0;
        }
    }
}

// ==========================
// Envia todo o buffer de LEDs
// ==========================

void ws2812_sendStrip(uint8_t(*buffer)[3]) {
    for (int i = 0; i < NUM_LEDS; i++) {
        ws2812_sendPixel(buffer[i][0], buffer[i][1], buffer[i][2]);
    }
    __delay_us(300); // Latch
}

// Simula as RPM (para teste)

unsigned int getSimulatedRPM(void) {
    static unsigned int rpm = 1400;
    static int fase = 0;
    static int contador = 0;
    
    switch (fase) {
        case 0:  // Marcha lenta (ralenti)
            rpm = 1350 + (contador % 5) * 20;  // oscila entre 1350?1450 rpm
            contador++;
            if (contador > 40) {
                fase = 1;
                contador = 0;
            }
            break;

        case 1:  // Aceleração suave inicial
            if (rpm < 5000) {
                rpm += 120 + (contador % 3) * 10;  // subida variável
                contador++;
            } else {
                fase = 2;
                contador = 0;
            }
            break;

        case 2:  // Aceleração com flutuações
            if (rpm < 10500) {
                rpm += 180 + ((contador % 4) - 2) * 30;  // +/- pequenas flutuações
                contador++;
            } else {
                fase = 3;
                contador = 0;
            }
            break;

        case 3:  // Corte (limitador)
            if (contador < 12) {
                int osc = (contador % 2) ? -300 : 250;
                int nova_rpm = (int)rpm + osc;
                if (nova_rpm < 0) nova_rpm = 0;
                if (nova_rpm > 12000) nova_rpm = 12000;
                rpm = (unsigned int)nova_rpm;
                contador++;
            } else {
                fase = 4;
                contador = 0;
            }
            break;

        case 4:  // Desaceleração
            if (rpm > 1700) {
                rpm -= 250 + (contador % 3) * 20;
                contador++;
            } else {
                fase = 0;
                contador = 0;
            }
            break;
    }

    return rpm;
}

// ==========================
// MAIN
// ==========================

void main(void) {

    OSCCONbits.SCS = 0b00; // usa clock defualt
    OSCCONbits.IRCF = 0b111; // 8 MHz interno
    PLLEN = 1; // ativa PLL x4 = 32 MHz
    GIE = 0;

    LED_TRIS = 0;
    HB_TRIS = 0;
    HB_LED = 0;

    for (int i = 0; i < NUM_LEDS; i++) {
        led_buffer[i][0] = 0; // R
        led_buffer[i][1] = 0; // G
        led_buffer[i][2] = 0; // B
    }

    unsigned int rpm = 0;
    unsigned int heartbeat_counter = 0;

    while (1) {

        // Modo 3: Funcionamento normal (descomente)
        rpm =14000; //getSimulatedRPM();
        fillStripByRPM(rpm);
        ws2812_sendStrip(led_buffer);

        // Heartbeat LED 
        heartbeat_counter++;
        if (heartbeat_counter > 100) {
            HB_LED = !HB_LED;
            heartbeat_counter = 0;
        }

        // Pequeno delay para não sobrecarregar
        __delay_ms(10);
    }
}