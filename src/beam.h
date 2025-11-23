/*  src/viga.h
    Header VIGA para MECSOL - TI-84 Plus CE
    Autor: https://github.com/daniSoares08
*/

#ifndef BEAM_H
#define BEAM_H

#include <stddef.h>

typedef struct {
    float L;     // comprimento [m]
    float EI;    // rigidez flexional [N*m^2]
    // carga pontual única
    float P;     // intensidade [N] (positivo p/ baixo)
    float a;     // posição da carga [m], 0<=a<=L

    // reações e constantes de integração
    float RA, RB;
    float C1, C3, C4; // p/ y' / y nas duas regiões

} Beam;

void beam_init(Beam *b, float L, float EI, float P, float a);
void beam_set_pointload(Beam *b, float P, float a);
void beam_solve(Beam *b);

// grandezas por trechos (0<=x<a) e (a<=x<=L)
float V_of(const Beam *b, float x);
float M_of(const Beam *b, float x);
float theta_of(const Beam *b, float x);   // y' (rad), retorna (1/EI)*∫Mdx + constante
float y_of(const Beam *b, float x);       // deflexão [m]

#endif
