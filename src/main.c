/*  src/main.c
    MECSOL - Mecanica (Viga + Secao) - TI-84 Plus CE
    Programa pra centralizar contrução de vigas, gráficos de V e M
    cálculo e exibição de centroide de figura composta

    Autor: Daniel Campos Soares
    https://github.com/daniSoares08
*/

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Entradas dos módulos */
void centroid_module(void);
void viga_module(void);

/* ON sai imediatamente */
static inline void check_on_exit(void) {
    kb_Scan();
    if (kb_On) {
        gfx_End();
        exit(0);
    }
}

/* Detecção de borda de tecla (aperto único) */
static uint8_t pressed_once(kb_lkey_t lk) {
    static uint8_t prev_enter=0, prev_clear=0;
    static uint8_t prev0=0, prev1=0, prev2=0, prev3=0, prev4=0;
    static uint8_t prev5=0, prev6=0, prev7=0, prev8=0, prev9=0;
    uint8_t *prev = NULL;

    switch(lk) {
        case kb_KeyEnter:  prev = &prev_enter; break;
        case kb_KeyClear:  prev = &prev_clear; break;
        case kb_Key0:      prev = &prev0; break;
        case kb_Key1:      prev = &prev1; break;
        case kb_Key2:      prev = &prev2; break;
        case kb_Key3:      prev = &prev3; break;
        case kb_Key4:      prev = &prev4; break;
        case kb_Key5:      prev = &prev5; break;
        case kb_Key6:      prev = &prev6; break;
        case kb_Key7:      prev = &prev7; break;
        case kb_Key8:      prev = &prev8; break;
        case kb_Key9:      prev = &prev9; break;
        default: return 0;
    }

    uint8_t down = kb_IsDown(lk) ? 1 : 0;
    uint8_t edge = (down && !*prev);
    *prev = down;
    return edge;
}

/* Espera ENTER ou CLEAR numa telinha simples */
static void wait_enter_or_clear_main(void) {
    while (1) {
        check_on_exit();
        kb_Scan();
        if (pressed_once(kb_KeyEnter)) return;
        if (pressed_once(kb_KeyClear)) return;
    }
}

int main(void) {
    kb_DisableOnLatch();
    gfx_Begin();

    /* Paleta:
       0 = branco (fundo)
       1 = preto (texto / linhas)
       2 = vermelho claro (rótulos / diagramas) */
    gfx_palette[0] = gfx_RGBTo1555(255, 255, 255);
    gfx_palette[1] = gfx_RGBTo1555(0, 0, 0);
    gfx_palette[2] = gfx_RGBTo1555(255, 64, 64);

    gfx_SetTextFGColor(1);
    gfx_SetTextBGColor(0);
    gfx_SetTextTransparentColor(0);
    gfx_SetTextScale(1, 1);

    bool running = true;

    while (running) {
        uint8_t key = 0;

        /* Tela do menu principal */
        while (!key) {
            check_on_exit();
            kb_Scan();

            gfx_FillScreen(0);   /* branco */
            gfx_SetTextFGColor(1);

            gfx_PrintStringXY("=== MENU PRINCIPAL ===", 2, 2);
            gfx_PrintStringXY("1) Formato (centroide & Ix)", 2, 18);
            gfx_PrintStringXY("2) Viga (reacoes & internos)", 2, 30);
            gfx_PrintStringXY("3) Max. tensoes  [TO DO]", 2, 42);
            gfx_PrintStringXY("4) Sair", 2, 54);

            if (pressed_once(kb_Key1)) key = 1;
            else if (pressed_once(kb_Key2)) key = 2;
            else if (pressed_once(kb_Key3)) key = 3;
            else if (pressed_once(kb_Key4)) key = 4;
        }

        if (key == 1) {
            /* Formato / centroide & Ix */
            centroid_module();
        }
        else if (key == 2) {
            /* Viga */
            viga_module();
        }
        else if (key == 3) {
            /* Placeholder para Máx. tensões */
            while (1) {
                check_on_exit();
                kb_Scan();
                gfx_FillScreen(0);
                gfx_SetTextFGColor(1);
                gfx_PrintStringXY("Max. tensoes: [TO DO]", 2, 30);
                gfx_PrintStringXY("ENTER/CLEAR: voltar", 2, 46);
                if (pressed_once(kb_KeyEnter) || pressed_once(kb_KeyClear)) {
                    break;
                }
            }
        }
        else if (key == 4) {
            running = false;
        }
    }

    gfx_End();
    return 0;
}
