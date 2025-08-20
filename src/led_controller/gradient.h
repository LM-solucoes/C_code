#ifndef GRADIENT_H
#define GRADIENT_H

#include <stdint.h>

// Estrutura para cor RGB
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_Color;

// Funções de gradiente
void calculateGradient(RGB_Color* strip, int numLeds, RGB_Color startColor, RGB_Color endColor);
void setAllLeds(RGB_Color* strip, int numLeds, RGB_Color color);
void fadeToColor(RGB_Color* strip, int numLeds, RGB_Color targetColor, int steps);

#endif // GRADIENT_H
