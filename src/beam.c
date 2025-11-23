/*  src/beam.c
    Aux de VIGA para MECSOL - TI-84 Plus CE
    Autor: https://github.com/daniSoares08
*/

#include "beam.h"
#include <math.h>

static inline float clampf(float v, float lo, float hi){
    return v<lo?lo:(v>hi?hi:v);
}

void beam_init(Beam *b, float L, float EI, float P, float a){
    b->L = (L>0?L:1.0f);
    b->EI = (EI>0?EI:1.0e4f);
    b->P = P;
    b->a = clampf(a, 0.0f, b->L);
    b->RA = b->RB = 0.0f;
    b->C1 = b->C3 = b->C4 = 0.0f;
    beam_solve(b);
}

void beam_set_pointload(Beam *b, float P, float a){
    b->P = P;
    b->a = clampf(a, 0.0f, b->L);
    beam_solve(b);
}

void beam_solve(Beam *b){
    const float L = b->L;
    const float a = b->a;
    const float P = b->P;
    const float EI = b->EI;

    // Reações (viga simplesmente apoiada)
    b->RA = P * (L - a) / L;
    b->RB = P * a / L;

    // Integração: EI y'' = M
    // Região 1 (0<=x<a):
    //   y1' = RA x^2/(2 EI) + C1
    //   y1  = RA x^3/(6 EI) + C1 x + C2, com y(0)=0 => C2=0
    // Região 2 (a<=x<=L):
    //   y2' = RA x^2/(2 EI) - P (x-a)^2/(2 EI) + C3
    //   y2  = RA x^3/(6 EI) - P (x-a)^3/(6 EI) + C3 x + C4
    //
    // Condições:
    //  1) Continuidade de y' em x=a  => y1'(a)=y2'(a)
    //  2) Continuidade de y  em x=a  => y1(a)=y2(a)
    //  3) y(L)=0 (apoiada em B)
    //
    // Resolve (sistema 3x3 em C1, C3, C4). Fazemos algébrico direto:

    float a2 = a*a, a3 = a2*a;
    float L2 = L*L, L3 = L2*L;

    (void)a2; (void)a3; // não usados diretamente, mas mantidos para clareza

    // Equações:
    // (1) RA a^2/(2EI) + C1 = RA a^2/(2EI) - P*0/(2EI) + C3  => C1 = C3
    // (2) RA a^3/(6EI) + C1 a = RA a^3/(6EI) - P*0/(6EI) + C3 a + C4  => C4 = C1 a - C3 a = 0
    // (3) y2(L) = RA L^3/(6EI) - P (L-a)^3/(6EI) + C3 L + C4 = 0

    // Das (1) -> C1=C3 ; Das (2) -> C4=0.
    // Então (3): C3 L = -[ RA L^3/(6EI) - P (L-a)^3/(6EI) ]
    // => C3 = -( RA L^3 - P (L-a)^3 ) / (6EI L)

    float num = (b->RA * L3) - (P * powf(L - a, 3.0f));
    b->C3 = b->C1 = -( num ) / (6.0f * EI * L);
    b->C4 = 0.0f;
}

float V_of(const Beam *b, float x){
    // cortante: RA - P*H(x-a)
    if (x < b->a) return b->RA;
    return b->RA - b->P;
}

float M_of(const Beam *b, float x){
    // momento: RA x - P (x-a)H(x-a)
    if (x < b->a) return b->RA * x;
    return b->RA * x - b->P * (x - b->a);
}

float theta_of(const Beam *b, float x){
    const float EI = b->EI;
    if (x < b->a){
        return (b->RA * x*x) / (2.0f * EI) + b->C1;
    } else {
        float xm = x - b->a;
        return (b->RA * x*x) / (2.0f * EI) - (b->P * xm*xm) / (2.0f * EI) + b->C3;
    }
}

float y_of(const Beam *b, float x){
    const float EI = b->EI;
    if (x < b->a){
        return (b->RA * x*x*x) / (6.0f * EI) + b->C1 * x; // C2=0
    } else {
        float xm = x - b->a;
        return (b->RA * x*x*x) / (6.0f * EI)
             - (b->P * xm*xm*xm) / (6.0f * EI)
             + b->C3 * x + b->C4;
    }
}
