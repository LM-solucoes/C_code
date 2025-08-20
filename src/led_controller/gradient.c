#include "gradient.h"

void calculateGradient(RGB_Color* strip, int numLeds, RGB_Color startColor, RGB_Color endColor) {
    for(int i = 0; i < numLeds; i++) {
        float ratio = (float)i / (numLeds - 1);
        strip[i].r = startColor.r + (endColor.r - startColor.r) * ratio;
        strip[i].g = startColor.g + (endColor.g - startColor.g) * ratio;
        strip[i].b = startColor.b + (endColor.b - startColor.b) * ratio;
    }
}

void setAllLeds(RGB_Color* strip, int numLeds, RGB_Color color) {
    for(int i = 0; i < numLeds; i++) {
        strip[i] = color;
    }
}

void fadeToColor(RGB_Color* strip, int numLeds, RGB_Color targetColor, int steps) {
    RGB_Color startColors[numLeds];
    for(int i = 0; i < numLeds; i++) {
        startColors[i] = strip[i];
    }
    
    for(int step = 0; step < steps; step++) {
        float ratio = (float)step / (steps - 1);
        for(int i = 0; i < numLeds; i++) {
            strip[i].r = startColors[i].r + (targetColor.r - startColors[i].r) * ratio;
            strip[i].g = startColors[i].g + (targetColor.g - startColors[i].g) * ratio;
            strip[i].b = startColors[i].b + (targetColor.b - startColors[i].b) * ratio;
        }
        // Aqui você deve adicionar um pequeno delay entre os passos
        // e atualizar os LEDs físicos
    }
}