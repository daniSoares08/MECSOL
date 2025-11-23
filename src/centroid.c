/*  src/centroid.c
    Módulo FORMATO / CENTROID para MECSOL - TI-84 Plus CE
    Autor: https://github.com/daniSoares08
*/

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_RECT 12
#define STRBUF 64

/* Estruturas para retângulos */
typedef struct { unsigned char rec; double w,h,x0,y0; } Rect;
typedef struct { double A,cx,cy; } Props;

/* Dados globais da figura (permanece em memória até ON) */
static Rect R[MAX_RECT];
static int N = 0;
static double xbar = 0.0, ybar = 0.0;

/* ======== Teclado / util ======== */

static inline void check_on_exit(void) {
    kb_Scan();
    if (kb_On) {
        gfx_End();
        exit(0);
    }
}

/* pressed_once: borda usando keypadc (igual ao rascunho) */
static uint8_t pressed_once(kb_lkey_t lk) {
    static uint8_t prev_enter=0, prev_clear=0;
    static uint8_t prev0=0, prev1=0, prev2=0, prev3=0, prev4=0;
    static uint8_t prev5=0, prev6=0, prev7=0, prev8=0, prev9=0;
    static uint8_t prevDot=0, prevNeg=0;
    uint8_t *prev=NULL;
    switch(lk) {
        case kb_KeyEnter: prev=&prev_enter; break;
        case kb_KeyClear: prev=&prev_clear; break;
        case kb_Key0: prev=&prev0; break;
        case kb_Key1: prev=&prev1; break;
        case kb_Key2: prev=&prev2; break;
        case kb_Key3: prev=&prev3; break;
        case kb_Key4: prev=&prev4; break;
        case kb_Key5: prev=&prev5; break;
        case kb_Key6: prev=&prev6; break;
        case kb_Key7: prev=&prev7; break;
        case kb_Key8: prev=&prev8; break;
        case kb_Key9: prev=&prev9; break;
        case kb_KeyDecPnt: prev=&prevDot; break;
        case kb_KeyChs: prev=&prevNeg; break;
        default: return 0;
    }
    uint8_t down = kb_IsDown(lk) ? 1 : 0;
    uint8_t edge = (down && !*prev);
    *prev = down;
    return edge;
}

/* input_line (teclado estilo CENTROID no rascunho) */
static void input_line(char *buf, int maxlen, const char *prompt){
    int idx=0; buf[0]='\0';
    for(;;){
        check_on_exit();
        kb_Scan();
        gfx_FillScreen(0);              /* fundo branco */
        gfx_SetTextFGColor(1);          /* texto preto */
        gfx_PrintStringXY(prompt, 2, 2);
        gfx_PrintStringXY(buf, 2, 18);
        gfx_PrintStringXY("ENTER=ok CLEAR=apaga ON=sair", 2, 78);

        if (pressed_once(kb_KeyEnter)) { buf[idx]='\0'; return; }
        if (pressed_once(kb_KeyClear)) { if (idx>0){ idx--; buf[idx]='\0'; } }
        if (pressed_once(kb_Key0) && idx<maxlen-1){ buf[idx++]='0'; buf[idx]='\0'; }
        if (pressed_once(kb_Key1) && idx<maxlen-1){ buf[idx++]='1'; buf[idx]='\0'; }
        if (pressed_once(kb_Key2) && idx<maxlen-1){ buf[idx++]='2'; buf[idx]='\0'; }
        if (pressed_once(kb_Key3) && idx<maxlen-1){ buf[idx++]='3'; buf[idx]='\0'; }
        if (pressed_once(kb_Key4) && idx<maxlen-1){ buf[idx++]='4'; buf[idx]='\0'; }
        if (pressed_once(kb_Key5) && idx<maxlen-1){ buf[idx++]='5'; buf[idx]='\0'; }
        if (pressed_once(kb_Key6) && idx<maxlen-1){ buf[idx++]='6'; buf[idx]='\0'; }
        if (pressed_once(kb_Key7) && idx<maxlen-1){ buf[idx++]='7'; buf[idx]='\0'; }
        if (pressed_once(kb_Key8) && idx<maxlen-1){ buf[idx++]='8'; buf[idx]='\0'; }
        if (pressed_once(kb_Key9) && idx<maxlen-1){ buf[idx++]='9'; buf[idx]='\0'; }
        if (pressed_once(kb_KeyDecPnt) && idx<maxlen-1){ buf[idx++]='.'; buf[idx]='\0'; }
        if (pressed_once(kb_KeyChs)    && idx<maxlen-1){ buf[idx++]='-'; buf[idx]='\0'; }
        delay(10);
    }
}
static double input_double(const char *prompt){
    char s[STRBUF]; input_line(s, STRBUF, prompt);
    for (int i=0; s[i]; ++i) if (s[i] == ',') s[i] = '.';
    return atof(s);
}
static int input_int(const char *prompt){
    char s[STRBUF]; input_line(s, STRBUF, prompt);
    return atoi(s);
}

/* ======== Pequeno render de frações (tipo LatexViewer simplificado) ======== */
/* OBS: Base funcional, tenho que implementar melhor futuramente. Meu
    projeto de compilador latex ainda não foi finalizado, quando for, eu 
    completo esse render
*/

static void draw_fraction_str(const char *num, const char *den, int cx, int y_top) {
    unsigned int w_num = gfx_GetStringWidth(num);
    unsigned int w_den = gfx_GetStringWidth(den);
    unsigned int w = (w_num > w_den ? w_num : w_den) + 6;

    int x_num = cx - (int)w_num/2;
    int x_den = cx - (int)w_den/2;
    int y_num = y_top;
    int y_bar = y_top + 10;
    int y_den = y_bar + 4;

    gfx_SetTextFGColor(1);  /* preto */
    gfx_PrintStringXY(num, x_num, y_num);
    gfx_HorizLine(cx - (int)w/2, y_bar, (int)w);
    gfx_PrintStringXY(den, x_den, y_den);
}

/* ======== Cálculos de centroide e inércia ======== */

static Props props(const Rect *r) {
    Props p;
    p.A = r->w * r->h;
    if (r->rec) p.A = -p.A; /* recorte subtrai */
    p.cx = r->x0 + r->w/2.0;
    p.cy = r->y0 + r->h/2.0;
    return p;
}

/* tela introdutória para as fórmulas (mostra frações genéricas) */
static void tela_formula_centroide(void) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("CENTROIDE (retangulos)", 2, 2);

    gfx_PrintStringXY("x_bar =", 8, 32);
    draw_fraction_str("Sum(A_i x_i)", "Sum(A_i)", 150, 32);

    gfx_PrintStringXY("y_bar =", 8, 80);
    draw_fraction_str("Sum(A_i y_i)", "Sum(A_i)", 150, 80);

    gfx_PrintStringXY("ENTER=passos  CLEAR=voltar", 2, 140);
}

/* tela introdutória para Ix */
static void tela_formula_ix(void) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("INERCIA Ix", 2, 2);
    gfx_PrintStringXY("Ix = Sum( Ixc + A*dy^2 )", 2, 20);
    gfx_PrintStringXY("Ixc = b*h^3/12 (cada ret.)", 2, 32);
    gfx_PrintStringXY("dy = |cy - y_bar|", 2, 44);
    gfx_PrintStringXY("ENTER=passos  CLEAR=voltar", 2, 80);
}

/* calcula centroide; se show!=0, mostra passo a passo em telas */
static void calc_centroid(int show) {
    double SA = 0.0, SAx = 0.0, SAy = 0.0;

    if (show) {
        tela_formula_centroide();
        while (1) { check_on_exit(); kb_Scan(); if (pressed_once(kb_KeyEnter)) break; if (pressed_once(kb_KeyClear)) return; }
    }

    for (int i = 0; i < N; i++) {
        Props q = props(&R[i]);
        SA  += q.A;
        SAx += q.A * q.cx;
        SAy += q.A * q.cy;
        if (show) {
            gfx_FillScreen(0); gfx_SetTextFGColor(1);
            char buf[64];
            sprintf(buf, "Item %d/%d (%s)", i+1, N, R[i].rec?"REC":"MAT");
            gfx_PrintStringXY(buf, 2, 2);
            sprintf(buf, "b=%.3f h=%.3f A=%.3f", R[i].w, R[i].h, q.A);
            gfx_PrintStringXY(buf, 2, 18);
            sprintf(buf, "x0=%.3f y0=%.3f", R[i].x0, R[i].y0);
            gfx_PrintStringXY(buf, 2, 30);
            sprintf(buf, "cx=%.3f cy=%.3f", q.cx, q.cy);
            gfx_PrintStringXY(buf, 2, 42);
            sprintf(buf, "Ax=%.3f Ay=%.3f", q.A*q.cx, q.A*q.cy);
            gfx_PrintStringXY(buf, 2, 54);
            gfx_PrintStringXY("ENTER=proximo  CLEAR=voltar", 2, 100);
            while (1) { check_on_exit(); kb_Scan(); if (pressed_once(kb_KeyEnter)) break; if (pressed_once(kb_KeyClear)) return; }
        }
    }
    if (SA != 0.0) {
        xbar = SAx / SA;
        ybar = SAy / SA;
    } else {
        xbar = ybar = 0.0;
    }

    if (show) {
        gfx_FillScreen(0); gfx_SetTextFGColor(1);
        char num[32], den[32];

        sprintf(num, "SAx=%.3f", SAx);
        sprintf(den, "SA=%.3f",  SA);
        gfx_PrintStringXY("x_bar =", 2, 24);
        draw_fraction_str(num, den, 150, 24);

        sprintf(num, "SAy=%.3f", SAy);
        sprintf(den, "SA=%.3f",  SA);
        gfx_PrintStringXY("y_bar =", 2, 80);
        draw_fraction_str(num, den, 150, 80);

        char buf[64];
        sprintf(buf, "x_bar=%.3f  y_bar=%.3f", xbar, ybar);
        gfx_PrintStringXY(buf, 2, 140);
        gfx_PrintStringXY("ENTER=ok", 2, 160);
        while (!pressed_once(kb_KeyEnter)) { check_on_exit(); kb_Scan(); }
    }
}

/* calcula Ix; se show!=0, exibe passo a passo com frações simples e termos */
static double calc_Ix(int show) {
    double Ix = 0.0;

    if (show) {
        tela_formula_ix();
        while (1) { check_on_exit(); kb_Scan(); if (pressed_once(kb_KeyEnter)) break; if (pressed_once(kb_KeyClear)) return 0.0; }
    }

    for (int i = 0; i < N; i++) {
        Props q = props(&R[i]);
        double Ixc_mag = (R[i].w * pow(R[i].h,3)) / 12.0;
        double Ixc = R[i].rec ? -Ixc_mag : Ixc_mag;
        double dy = fabs(q.cy - ybar);
        double term = Ixc + q.A * dy * dy;
        Ix += term;
        if (show) {
            gfx_FillScreen(0); gfx_SetTextFGColor(1);
            char buf[64];
            sprintf(buf, "Item %d/%d (%s)", i+1, N, R[i].rec?"REC":"MAT");
            gfx_PrintStringXY(buf, 2, 2);
            sprintf(buf, "b=%.3f h=%.3f", R[i].w, R[i].h);
            gfx_PrintStringXY(buf, 2, 18);
            sprintf(buf, "cy=%.3f dy=%.3f", q.cy, dy);
            gfx_PrintStringXY(buf, 2, 30);
            sprintf(buf, "Ixc=%.3f A=%.3f", Ixc, q.A);
            gfx_PrintStringXY(buf, 2, 42);
            sprintf(buf, "Termo=%.3f", term);
            gfx_PrintStringXY(buf, 2, 54);
            gfx_PrintStringXY("ENTER=proximo  CLEAR=voltar", 2, 100);
            while (1) { check_on_exit(); kb_Scan(); if (pressed_once(kb_KeyEnter)) break; if (pressed_once(kb_KeyClear)) return Ix; }
        }
    }

    if (show) {
        gfx_FillScreen(0); gfx_SetTextFGColor(1);
        char num[32], den[32];
        /* Exibe Ix final em forma simples (Ix = soma dos termos) */
        sprintf(num, "Ix_total=%.3f", Ix);
        sprintf(den, "units^4");
        draw_fraction_str(num, den, 160, 24);
        gfx_PrintStringXY("ENTER=ok", 2, 140);
        while (!pressed_once(kb_KeyEnter)) { check_on_exit(); kb_Scan(); }
    }
    return Ix;
}

/* ======== Desenho da seção (preview) ======== */
/* desenha a seção composta por retângulos dentro da área disponível */
static void desenhar_secao_preview(void) {
    /* área de desenho abaixo do menu: x in [8..312], y in [110..200] */
    const int x0 = 8, x1 = 312;
    const int y0 = 110, y1 = 200;
    const int w = x1 - x0;
    const int h = y1 - y0;

    gfx_SetColor(1); /* preto */

    /* se nenhuma figura, escreve aviso */
    if (N == 0) {
        gfx_PrintStringXY("Nenhuma figura definida.", x0, y0 + 10);
        return;
    }

    /* encontra bounding box da geometria (em unidades usadas: x/y dos retângulos) */
    double minx = 1e9, maxx = -1e9, miny = 1e9, maxy = -1e9;
    for (int i = 0; i < N; ++i) {
        double rx0 = R[i].x0;
        double rx1 = R[i].x0 + R[i].w;
        double ry0 = R[i].y0;
        double ry1 = R[i].y0 + R[i].h;
        if (rx0 < minx) minx = rx0;
        if (rx1 > maxx) maxx = rx1;
        if (ry0 < miny) miny = ry0;
        if (ry1 > maxy) maxy = ry1;
    }
    if (minx >= maxx) { minx -= 1.0; maxx += 1.0; }
    if (miny >= maxy) { miny -= 1.0; maxy += 1.0; }

    double width = maxx - minx;
    double height = maxy - miny;

    /* escala uniforme com padding */
    double pad = 6.0;
    double sx = (w - 2*pad) / width;
    double sy = (h - 2*pad) / height;
    double s = (sx < sy) ? sx : sy;
    if (s <= 0) s = 1.0;

    /* origem em pixels para posicionar (centraliza) */
    double px0 = x0 + pad + (w - 2*pad - s*width)/2.0;
    double py0 = y0 + pad + (h - 2*pad - s*height)/2.0;

    /* desenha fundo da área */
    gfx_SetColor(0);
    gfx_FillRectangle(x0, y0, w, h);
    gfx_SetTextFGColor(1);

    /* desenha retângulos (matéria preta, recortes desenhados em branco sobrepostos) */
    for (int i = 0; i < N; ++i) {
        int rx = (int)round(px0 + (R[i].x0 - minx) * s);
        int ry = (int)round(py0 + (maxy - (R[i].y0 + R[i].h)) * s); /* invert y: y0 is base */
        int rw = (int)round(R[i].w * s);
        int rh = (int)round(R[i].h * s);

        if (R[i].rec) {
            /* recorte: desenha um retângulo branco sobreposto */
            gfx_SetColor(0); /* branco */
            gfx_FillRectangle(rx, ry, rw, rh);
        } else {
            gfx_SetColor(1); /* preto */
            gfx_FillRectangle(rx, ry, rw, rh);
        }
    }

    /* contorno da área de preview */
    gfx_SetColor(1);
    gfx_Rectangle(x0, y0, w, h);

    /* pequenos rótulos 0..width (em unidades) */
    char tmp[64];
    sprintf(tmp, "W=%.3f", width);
    gfx_SetTextFGColor(2); /* vermelho para rótulos se desejar */
    gfx_PrintStringXY(tmp, x1 - 60, y1 - 10);
    gfx_SetTextFGColor(1);
}

/* ======== Construir figura (entrada do usuário) ======== */
static void tela_construir(void) {
    memset(R, 0, sizeof(R));
    N = 0;

    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("FIGURA: retangulos (0,0) na base", 2, 2);
    gfx_PrintStringXY("Unidade livre (mm, cm, ...)", 2, 18);
    gfx_PrintStringXY("ENTER=ok", 2, 40);

    int nM = input_int("N materiais (0..12):");
    if (nM < 0) nM = 0; if (nM > MAX_RECT) nM = MAX_RECT;
    int nR = input_int("N recortes (0..12):");
    if (nR < 0) nR = 0; if (nM + nR > MAX_RECT) nR = MAX_RECT - nM;

    for (int i = 0; i < nM; ++i) {
        char t[STRBUF];
        snprintf(t, sizeof t, "[MAT %d] b (larg.):", i+1);
        R[N].w  = input_double(t);
        R[N].h  = input_double("h (alt.):");
        R[N].x0 = input_double("x0 (canto inf.):");
        R[N].y0 = input_double("y0 (canto inf.):");
        R[N].rec = 0;
        N++;
        /* mostrar preview entre entradas */
        gfx_FillScreen(0);
        gfx_PrintStringXY("Preview:", 2, 56);
        desenhar_secao_preview();
        gfx_PrintStringXY("ENTER=continuar CLEAR=cancelar", 2, 206);
        while (1) { check_on_exit(); kb_Scan(); if (pressed_once(kb_KeyEnter)) break; if (pressed_once(kb_KeyClear)) return; }
    }
    for (int i = 0; i < nR; ++i) {
        char t[STRBUF];
        snprintf(t, sizeof t, "[REC %d] b (larg.):", i+1);
        R[N].w  = input_double(t);
        R[N].h  = input_double("h (alt.):");
        R[N].x0 = input_double("x0 (canto inf.):");
        R[N].y0 = input_double("y0 (canto inf.):");
        R[N].rec = 1;
        N++;
        gfx_FillScreen(0);
        gfx_PrintStringXY("Preview:", 2, 56);
        desenhar_secao_preview();
        gfx_PrintStringXY("ENTER=continuar CLEAR=cancelar", 2, 206);
        while (1) { check_on_exit(); kb_Scan(); if (pressed_once(kb_KeyEnter)) break; if (pressed_once(kb_KeyClear)) return; }
    }

    /* calcula xbar/ybar sem mostrar passos */
    calc_centroid(0);
}

/* ======== MENU do módulo ======== */
static void tela_menu(void) {
    for (;;) {
        check_on_exit();
        kb_Scan();

        gfx_FillScreen(0);
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("=== MENU FORMATO ===", 2, 2);
        gfx_PrintStringXY("1) Centroide (passos)", 2, 18);
        gfx_PrintStringXY("2) Inercia Ix (passos)", 2, 30);
        gfx_PrintStringXY("3) Refazer figura", 2, 42);
        gfx_PrintStringXY("4) Voltar menu principal", 2, 54);

        /* preview abaixo */
        desenhar_secao_preview();

        /* espera tecla com borda */
        while (1) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_Key1)) { /* centroide passos */ calc_centroid(1); break; }
            if (pressed_once(kb_Key2)) { /* Ix passos */ calc_Ix(1); break; }
            if (pressed_once(kb_Key3)) { tela_construir(); break; }
            if (pressed_once(kb_Key4) || pressed_once(kb_KeyClear)) { return; }
            delay(10);
        }
    }
}

/* ======== API pública ======== */
void centroid_module(void) {
    /* Se não houver figura, chama o construir */
    if (N == 0) {
        tela_construir();
    }
    tela_menu();
}
