/*  src/centroid.c
    Módulo FORMATO / CENTROID para MECSOL - TI-84 Plus CE
    Autor: https://github.com/daniSoares08
*/

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdbool.h>
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
static double unit_factor = 1.0;   /* multiplicador para converter da unidade escolhida para metro */
static const char *unit_name = "m";

typedef struct { double val; const char *unit; } DispVal;
typedef struct { double factor; const char *name; } UnitOpt;

static double round_dec(double v, int dec) {
    double s = pow(10.0, dec);
    return round(v * s) / s;
}

/* Seleciona unidade para exibição (mm/cm/m) com base na magnitude, preferindo a unidade escolhida */
static const UnitOpt *pick_unit(double meters) {
    static const UnitOpt opts[] = {
        {0.001, "mm"},
        {0.01,  "cm"},
        {1.0,   "m"}
    };
    const UnitOpt *pref = &opts[2]; /* default m */
    for (size_t i=0;i<3;i++) if (fabs(opts[i].factor - unit_factor) < 1e-12) pref = &opts[i];

    const double LOW = 0.01;
    const double HIGH = 9999.0;

    const UnitOpt *best = pref;
    double val = (best->factor > 0.0) ? meters / best->factor : meters;
    double amag = fabs(val);

    if (amag > HIGH && best->factor < 1.0) {
        if (best->factor < 0.01) best = &opts[1]; else best = &opts[2];
    } else if (amag > 0 && amag < LOW && best->factor > 0.001) {
        if (best->factor > 0.01) best = &opts[1]; else best = &opts[0];
    }
    return best;
}

static DispVal disp_len(double meters) {
    const UnitOpt *u = pick_unit(meters);
    double v = (u->factor > 0.0) ? meters / u->factor : meters;
    return (DispVal){ v, u->name };
}

static DispVal disp_area(double m2) {
    const UnitOpt *u = pick_unit(sqrt(fabs(m2)));
    double f = u->factor;
    double v = (f > 0.0) ? m2 / (f * f) : m2;
    return (DispVal){ v, u->name };
}

static DispVal disp_m3(double m3) {
    const UnitOpt *u = pick_unit(cbrt(fabs(m3)));
    double f = u->factor;
    double v = (f > 0.0) ? m3 / (f * f * f) : m3;
    return (DispVal){ v, u->name };
}

static DispVal disp_m4(double m4) {
    const UnitOpt *u = pick_unit(pow(fabs(m4), 0.25));
    double f = u->factor;
    double f2 = f * f;
    double v = (f > 0.0) ? m4 / (f2 * f2) : m4;
    return (DispVal){ v, u->name };
}

/* ======== Teclado / util ======== */

static inline void check_on_exit(void) {
    kb_Scan();
    if (kb_On) {
        gfx_End();
        exit(0);
    }
}

/* espera soltar todas as teclas (evita propagar tecla 4 para o main) */
static void wait_key_release(void) {
    do { kb_Scan(); } while (kb_Data[1] | kb_Data[2] | kb_Data[3] |
                              kb_Data[4] | kb_Data[5] | kb_Data[6] | kb_Data[7]);
    delay(15);
}

static void set_unit_by_choice(int opt) {
    switch (opt) {
        case 1: unit_factor = 0.001; unit_name = "mm"; break;
        case 2: unit_factor = 0.01;  unit_name = "cm"; break;
        default: unit_factor = 1.0;  unit_name = "m"; break;
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

/* pergunta unidade (mm/cm/m) e ajusta fator para salvar em metros */
static void selecionar_unidade(void) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("Unidade das entradas:", 2, 2);
    gfx_PrintStringXY("1) mm", 2, 18);
    gfx_PrintStringXY("2) cm", 2, 30);
    gfx_PrintStringXY("3) m",  2, 42);
    gfx_PrintStringXY("ENTER/1..3 escolhe  CLEAR volta", 2, 70);

    while (1) {
        check_on_exit();
        kb_Scan();
        if (pressed_once(kb_Key1)) { set_unit_by_choice(1); break; }
        if (pressed_once(kb_Key2)) { set_unit_by_choice(2); break; }
        if (pressed_once(kb_Key3) || pressed_once(kb_KeyEnter)) { set_unit_by_choice(3); break; }
        if (pressed_once(kb_KeyClear)) { break; }
        delay(10);
    }
    wait_key_release();
}

/* input_line (teclado estilo CENTROID no rascunho) */
static void input_line(char *buf, int maxlen, const char *prompt){
    int idx=0; buf[0]='\0';
    bool dirty = true;

    gfx_FillScreen(0);              /* fundo branco (desenha uma vez) */
    gfx_SetTextFGColor(1);          /* texto preto */
    gfx_PrintStringXY(prompt, 2, 2);
    gfx_PrintStringXY("ENTER=ok CLEAR=apaga ON=sair", 2, 78);
    for(;;){
        check_on_exit();
        kb_Scan();
        if (dirty) {
            gfx_SetColor(0);
            gfx_FillRectangle(0, 18, 320, 12); /* limpa só a linha do buffer */
            gfx_SetTextFGColor(1);
            gfx_PrintStringXY(buf, 2, 18);
            dirty = false;
        }

        if (pressed_once(kb_KeyEnter)) { buf[idx]='\0'; return; }
        if (pressed_once(kb_KeyClear)) { if (idx>0){ idx--; buf[idx]='\0'; dirty = true; } }
        if (pressed_once(kb_Key0) && idx<maxlen-1){ buf[idx++]='0'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key1) && idx<maxlen-1){ buf[idx++]='1'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key2) && idx<maxlen-1){ buf[idx++]='2'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key3) && idx<maxlen-1){ buf[idx++]='3'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key4) && idx<maxlen-1){ buf[idx++]='4'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key5) && idx<maxlen-1){ buf[idx++]='5'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key6) && idx<maxlen-1){ buf[idx++]='6'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key7) && idx<maxlen-1){ buf[idx++]='7'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key8) && idx<maxlen-1){ buf[idx++]='8'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_Key9) && idx<maxlen-1){ buf[idx++]='9'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_KeyDecPnt) && idx<maxlen-1){ buf[idx++]='.'; buf[idx]='\0'; dirty = true; }
        if (pressed_once(kb_KeyChs)    && idx<maxlen-1){ buf[idx++]='-'; buf[idx]='\0'; dirty = true; }
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
/* OBS: Base funcional, tem que ser melhorado! TODO
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

/* ======== Helpers de super/subscrito (estilo LatexViewer simplificado) ======== */

static int draw_var_pow_idx(const char *base, const char *sub, const char *sup, int x, int y) {
    int w = 0;

    if (base && *base) {
        gfx_PrintStringXY(base, x, y);
        w = gfx_GetStringWidth(base);
    }
    if (sub && *sub) {
        /* subíndice desce um pouco */
        gfx_PrintStringXY(sub, x + w, y + 4);
        w += gfx_GetStringWidth(sub);
    }
    if (sup && *sup) {
        /* expoente sobe um pouco */
        gfx_PrintStringXY(sup, x + w, y - 4);
        w += gfx_GetStringWidth(sup);
    }
    return x + w;
}

/* Atalhos: só expoente ou só subíndice */
static int draw_power(const char *base, const char *sup, int x, int y) {
    return draw_var_pow_idx(base, NULL, sup, x, y);
}

static int draw_sub(const char *base, const char *sub, int x, int y) {
    return draw_var_pow_idx(base, sub, NULL, x, y);
}

/* ======== Cálculos de centroide e inércia ======== */

static Props props(const Rect *r) {
    Props p;
    p.A = round_dec(r->w * r->h, 10);
    if (r->rec) p.A = -p.A; /* recorte subtrai */
    p.cx = round_dec(r->x0 + r->w/2.0, 10);
    p.cy = round_dec(r->y0 + r->h/2.0, 10);
    return p;
}

/* tela introdutória para as fórmulas (mostra frações genéricas) */
static void tela_formula_centroide(void) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("CENTROIDE (retangulos)", 2, 2);

    /* x_bar = Sum(A_i * x_i) / Sum(A_i) */
    gfx_PrintStringXY("x_bar =", 8, 32);
    draw_fraction_str("Sum(A_i * x_i)", "Sum(A_i)", 150, 32);

    /* y_bar = Sum(A_i * y_i) / Sum(A_i) */
    gfx_PrintStringXY("y_bar =", 8, 80);
    draw_fraction_str("Sum(A_i * y_i)", "Sum(A_i)", 150, 80);

    gfx_PrintStringXY("ENTER=passos  CLEAR=voltar", 2, 140);
}

/* tela introdutória para Ix */
static void tela_formula_ix(void) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("INERCIA Ix (eixo x)", 2, 2);

    /* Ix = Sum( Ixc + A*dy^2 ) */
    int y = 24;
    int x = 2;
    gfx_PrintStringXY("Ix = Sum( Ixc + A*", x, y);
    x += gfx_GetStringWidth("Ix = Sum( Ixc + A*");
    x = draw_power("dy", "2", x, y);
    gfx_PrintStringXY(" )", x, y);

    /* Ixc = b*h^3/12 (cada ret.) */
    y += 12;
    x = 2;
    gfx_PrintStringXY("Ixc = b*", x, y);
    x += gfx_GetStringWidth("Ixc = b*");
    x = draw_power("h", "3", x, y);
    gfx_PrintStringXY("/12  (cada ret.)", x, y);

    /* dy = |cy - y_bar| */
    y += 12;
    gfx_PrintStringXY("dy = |cy - y_bar|", 2, y);

    gfx_PrintStringXY("ENTER=passos  CLEAR=voltar", 2, 140);
}

/* Mostra um resumo numerico da conta do centroide (A_i, x_i, y_i, somas) */
static bool show_centroid_summary(double SA, double SAx, double SAy,
                                  const double *Ai,
                                  const double *cxi,
                                  const double *cyi,
                                  int n) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);

    gfx_PrintStringXY("CENTROIDE - resumo numerico", 2, 2);

    int y = 18;
    char buf[128];

    /* Lista dos retangulos: A_i, x_i, y_i e produtos */
    for (int i = 0; i < n && y < 120; ++i) {
        DispVal a  = disp_area(Ai[i]);
        DispVal cx = disp_len(cxi[i]);
        DispVal cy = disp_len(cyi[i]);
        snprintf(buf, sizeof buf,
                 "A%d=%.3f %s^2  x%d=%.3f %s  y%d=%.3f %s",
                 i+1, a.val, a.unit,
                 i+1, cx.val, cx.unit,
                 i+1, cy.val, cy.unit);
        gfx_PrintStringXY(buf, 2, y);
        y += 10;

        DispVal ax = disp_m3(Ai[i]*cxi[i]);
        DispVal ay = disp_m3(Ai[i]*cyi[i]);
        snprintf(buf, sizeof buf,
                 "A%d*x%d=%.3f %s^3  A%d*y%d=%.3f %s^3",
                 i+1, i+1, ax.val, ax.unit,
                 i+1, i+1, ay.val, ay.unit);
        gfx_PrintStringXY(buf, 2, y);
        y += 10;
    }

    if (y < 170) {
        y += 4;
        DispVal sa  = disp_area(SA);
        DispVal sax = disp_m3(SAx);
        DispVal say = disp_m3(SAy);
        snprintf(buf, sizeof buf, "Sum A_i  = %.3f %s^2", sa.val, sa.unit);
        gfx_PrintStringXY(buf, 2, y); y += 10;
        snprintf(buf, sizeof buf, "Sum A_i*x_i = %.3f %s^3", sax.val, sax.unit);
        gfx_PrintStringXY(buf, 2, y); y += 10;
        snprintf(buf, sizeof buf, "Sum A_i*y_i = %.3f %s^3", say.val, say.unit);
        gfx_PrintStringXY(buf, 2, y); y += 10;
    }

    gfx_PrintStringXY("ENTER=continua  CLEAR=voltar", 2, 210);

    while (1) {
        check_on_exit();
        kb_Scan();
        if (pressed_once(kb_KeyEnter)) return true;
        if (pressed_once(kb_KeyClear)) return false;
        delay(10);
    }
}

/* calcula centroide; se show!=0, mostra passo a passo em telas */
static void calc_centroid(int show) {
    double SA  = 0.0, SAx = 0.0, SAy = 0.0;

    /* para poder montar o resumo tipo tabela */
    double Ai[MAX_RECT];
    double cxi[MAX_RECT];
    double cyi[MAX_RECT];

    if (show) {
        tela_formula_centroide();
        while (1) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_KeyEnter)) break;
            if (pressed_once(kb_KeyClear)) return;
            delay(10);
        }
    }

    for (int i = 0; i < N; i++) {
        Props q = props(&R[i]);

        SA  += q.A;
        SAx += q.A * q.cx;
        SAy += q.A * q.cy;

        Ai[i]  = q.A;
        cxi[i] = q.cx;
        cyi[i] = q.cy;

        if (show) {
            gfx_FillScreen(0);
            gfx_SetTextFGColor(1);
            char buf[64];

            sprintf(buf, "Item %d/%d (%s)", i+1, N, R[i].rec ? "REC" : "MAT");
            gfx_PrintStringXY(buf, 2, 2);

            DispVal b = disp_len(R[i].w);
            DispVal h = disp_len(R[i].h);
            sprintf(buf, "b=%.3f %s  h=%.3f %s", b.val, b.unit, h.val, h.unit);
            gfx_PrintStringXY(buf, 2, 18);

            DispVal a = disp_area(q.A);
            sprintf(buf, "A=%.3f %s^2", a.val, a.unit);
            gfx_PrintStringXY(buf, 2, 30);

            DispVal cx = disp_len(q.cx);
            DispVal cy = disp_len(q.cy);
            sprintf(buf, "cx=%.3f %s  cy=%.3f %s", cx.val, cx.unit, cy.val, cy.unit);
            gfx_PrintStringXY(buf, 2, 42);

            DispVal acx = disp_m3(q.A * q.cx);
            DispVal acy = disp_m3(q.A * q.cy);
            sprintf(buf, "A*cx=%.3f  A*cy=%.3f %s^3", acx.val, acy.val, acx.unit);
            gfx_PrintStringXY(buf, 2, 54);

            gfx_PrintStringXY("ENTER=proximo  CLEAR=voltar", 2, 100);
            while (1) {
                check_on_exit();
                kb_Scan();
                if (pressed_once(kb_KeyEnter)) break;
                if (pressed_once(kb_KeyClear)) return;
                delay(10);
            }
        }
    }

    SA  = round_dec(SA, 10);
    SAx = round_dec(SAx, 10);
    SAy = round_dec(SAy, 10);
    if (SA != 0.0) {
        xbar = round_dec(SAx / SA, 10);
        ybar = round_dec(SAy / SA, 10);
    } else {
        xbar = ybar = 0.0;
    }

    if (show) {
        /* 1) tela estilo tabela/somatorio (igual slide) */
        if (!show_centroid_summary(SA, SAx, SAy, Ai, cxi, cyi, N))
            return;

        /* 2) tela final com a fracao pronta (resultado) */
        gfx_FillScreen(0);
        gfx_SetTextFGColor(1);
        char num[32], den[32];

        /* x_bar */
        DispVal sax = disp_m3(SAx);
        DispVal sa  = disp_area(SA);
        sprintf(num, "Sum(A_i x_i)=%.3f %s^3", sax.val, sax.unit);
        sprintf(den, "Sum(A_i)=%.3f %s^2",     sa.val, sa.unit);
        gfx_PrintStringXY("x_bar =", 2, 24);
        draw_fraction_str(num, den, 150, 24);

        /* y_bar */
        DispVal say = disp_m3(SAy);
        sprintf(num, "Sum(A_i y_i)=%.3f %s^3", say.val, say.unit);
        sprintf(den, "Sum(A_i)=%.3f %s^2",     sa.val, sa.unit);
        gfx_PrintStringXY("y_bar =", 2, 80);
        draw_fraction_str(num, den, 150, 80);

        char buf[64];
        DispVal dxbar = disp_len(xbar);
        DispVal dybar = disp_len(ybar);
        sprintf(buf, "x_bar=%.3f %s  y_bar=%.3f %s",
                dxbar.val, dxbar.unit,
                dybar.val, dybar.unit);
        gfx_PrintStringXY(buf, 2, 140);

        gfx_PrintStringXY("ENTER=ok", 2, 160);
        while (!pressed_once(kb_KeyEnter)) {
            check_on_exit();
            kb_Scan();
            delay(10);
        }
    }
}

/* calcula Ix; se show!=0, exibe passo a passo */
static double calc_Ix(int show) {
    double Ix = 0.0;
    double termos[MAX_RECT];  /* termo_i = Ixc_i + A_i*dy_i^2 */

    if (show) {
        tela_formula_ix();
        while (1) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_KeyEnter)) break;
            if (pressed_once(kb_KeyClear)) return 0.0;
            delay(10);
        }
    }

    for (int i = 0; i < N; i++) {
        Props q = props(&R[i]);

        double Ixc_mag = round_dec((R[i].w * pow(R[i].h, 3)) / 12.0, 10);
        double Ixc     = R[i].rec ? -Ixc_mag : Ixc_mag;

        double dy   = round_dec(fabs(q.cy - ybar), 10);
        double Ady2 = round_dec(q.A * dy * dy, 10);
        double term = round_dec(Ixc + Ady2, 10);

        Ix         += term;
        termos[i]   = term;

        if (show) {
            gfx_FillScreen(0);
            gfx_SetTextFGColor(1);
            char buf[64];

            sprintf(buf, "Item %d/%d (%s)", i+1, N, R[i].rec ? "REC" : "MAT");
            gfx_PrintStringXY(buf, 2, 2);

            DispVal bw = disp_len(R[i].w);
            DispVal bh = disp_len(R[i].h);
            sprintf(buf, "b=%.3f %s  h=%.3f %s", bw.val, bw.unit, bh.val, bh.unit);
            gfx_PrintStringXY(buf, 2, 18);

            DispVal dcy = disp_len(q.cy);
            DispVal ddy = disp_len(dy);
            sprintf(buf, "cy=%.3f %s  dy=%.3f %s", dcy.val, dcy.unit, ddy.val, ddy.unit);
            gfx_PrintStringXY(buf, 2, 30);

            DispVal dIxc = disp_m4(Ixc);
            sprintf(buf, "Ixc=%.3f %s^4", dIxc.val, dIxc.unit);
            gfx_PrintStringXY(buf, 2, 42);

            DispVal dA = disp_area(q.A);
            sprintf(buf, "A=%.3f %s^2", dA.val, dA.unit);
            gfx_PrintStringXY(buf, 2, 54);

            DispVal dAdy2 = disp_m4(Ady2);
            sprintf(buf, "A*dy^2=%.3f %s^4", dAdy2.val, dAdy2.unit);
            gfx_PrintStringXY(buf, 2, 66);

            DispVal dterm = disp_m4(term);
            sprintf(buf, "Termo=Ixc+A*dy^2=%.3f %s^4", dterm.val, dterm.unit);
            gfx_PrintStringXY(buf, 2, 78);

            gfx_PrintStringXY("ENTER=proximo  CLEAR=voltar", 2, 100);
            while (1) {
                check_on_exit();
                kb_Scan();
                if (pressed_once(kb_KeyEnter)) break;
                if (pressed_once(kb_KeyClear)) return Ix;
                delay(10);
            }
        }
    }

    Ix = round_dec(Ix, 10);

    if (show) {
        /* tela final: soma dos termos, parecido com o slide */
        gfx_FillScreen(0);
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("INERCIA Ix - resumo", 2, 2);

        char buf[64];
        int y = 18;

        for (int i = 0; i < N && y < 180; ++i) {
            DispVal dt = disp_m4(termos[i]);
            sprintf(buf, "term%d = %.3f %s^4", i+1, dt.val, dt.unit);
            gfx_PrintStringXY(buf, 2, y);
            y += 10;
        }

        y += 4;
        DispVal dIx = disp_m4(Ix);
        sprintf(buf, "Ix = sum(term_i) = %.3f %s^4", dIx.val, dIx.unit);
        gfx_PrintStringXY(buf, 2, y);

        gfx_PrintStringXY("ENTER=ok", 2, 210);
        while (!pressed_once(kb_KeyEnter)) {
            check_on_exit();
            kb_Scan();
            delay(10);
        }
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
    DispVal dW = disp_len(width);
    sprintf(tmp, "W=%.3f %s", dW.val, dW.unit);
    gfx_SetTextFGColor(2); /* vermelho para rótulos se desejar */
    gfx_PrintStringXY(tmp, x1 - 60, y1 - 10);
    gfx_SetTextFGColor(1);
}

/* ======== Construir figura (entrada do usuário) ======== */
static void tela_construir(void) {
    memset(R, 0, sizeof(R));
    N = 0;

    selecionar_unidade(); /* escolhe mm/cm/m antes de entrar com dados */

    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    char info[64];
    snprintf(info, sizeof info, "FIGURA: retangulos (0,0) na base [%s]", unit_name);
    gfx_PrintStringXY(info, 2, 2);
    gfx_PrintStringXY("ENTER=ok", 2, 20);

    int nM = input_int("N materiais (0..12):");
    if (nM < 0) nM = 0; if (nM > MAX_RECT) nM = MAX_RECT;
    int nR = input_int("N recortes (0..12):");
    if (nR < 0) nR = 0; if (nM + nR > MAX_RECT) nR = MAX_RECT - nM;

    for (int i = 0; i < nM; ++i) {
        char t[STRBUF];
        snprintf(t, sizeof t, "[MAT %d] b (larg.) [%s]:", i+1, unit_name);
        R[N].w  = round_dec(input_double(t) * unit_factor, 10);
        snprintf(t, sizeof t, "h (alt.) [%s]:", unit_name);
        R[N].h  = round_dec(input_double(t) * unit_factor, 10);
        snprintf(t, sizeof t, "x0 (canto inf.) [%s]:", unit_name);
        R[N].x0 = round_dec(input_double(t) * unit_factor, 10);
        snprintf(t, sizeof t, "y0 (canto inf.) [%s]:", unit_name);
        R[N].y0 = round_dec(input_double(t) * unit_factor, 10);
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
        snprintf(t, sizeof t, "[REC %d] b (larg.) [%s]:", i+1, unit_name);
        R[N].w  = round_dec(input_double(t) * unit_factor, 10);
        snprintf(t, sizeof t, "h (alt.) [%s]:", unit_name);
        R[N].h  = round_dec(input_double(t) * unit_factor, 10);
        snprintf(t, sizeof t, "x0 (canto inf.) [%s]:", unit_name);
        R[N].x0 = round_dec(input_double(t) * unit_factor, 10);
        snprintf(t, sizeof t, "y0 (canto inf.) [%s]:", unit_name);
        R[N].y0 = round_dec(input_double(t) * unit_factor, 10);
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

/* ======== API P/ MODULO (MAX. TENSOES) ======== */

/* retorna 1 se existir pelo menos um retangulo definido */
int centroid_has_figure(void) {
    return (N > 0);
}

/* devolve x_bar e y_bar em METROS (recalcula rapido, sem telas) */
void centroid_get_centroid(double *px, double *py) {
    if (N <= 0) {
        if (px) *px = 0.0;
        if (py) *py = 0.0;
        return;
    }
    /* garante xbar/ybar atualizados */
    calc_centroid(0);

    if (px) *px = round_dec(xbar, 8);
    if (py) *py = round_dec(ybar, 8);
}

/* devolve Ix em m^4 (recalcula silenciosamente se preciso) */
double centroid_get_Ix(void) {
    if (N <= 0) return 0.0;

    /* assegura ybar correto antes de calcular Ix */
    calc_centroid(0);
    return round_dec(calc_Ix(0), 8);
}

/* limites inferiores/superiores em y (em METROS, sistema interno) */
void centroid_get_y_bounds(double *pymin, double *pymax) {
    if (N <= 0) {
        if (pymin) *pymin = 0.0;
        if (pymax) *pymax = 0.0;
        return;
    }

    double miny = 1e9, maxy = -1e9;
    for (int i = 0; i < N; ++i) {
        double y0 = R[i].y0;
        double y1 = R[i].y0 + R[i].h;
        if (y0 < miny) miny = y0;
        if (y1 > maxy) maxy = y1;
    }

    if (pymin) *pymin = round_dec(miny, 8);
    if (pymax) *pymax = round_dec(maxy, 8);
}

/* unidade usada na figura ("mm", "cm" ou "m") */
const char *centroid_get_unit_name(void) {
    return unit_name;
}

/* fator p/ converter UNIDADE -> metro (mesmo usado internamente) */
double centroid_get_unit_factor(void) {
    return unit_factor;
}


/* ======== MENU ======== */
static void tela_menu(void) {
    for (;;) {
        check_on_exit();
        kb_Scan();

        gfx_FillScreen(0);
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("=== MENU FIGURA ===", 2, 2);
        char ubuf[32];
        snprintf(ubuf, sizeof ubuf, "Unidade: %s", unit_name);
        gfx_PrintStringXY(ubuf, 200, 2);
        gfx_PrintStringXY("1) Centroide (passos)", 2, 18);
        gfx_PrintStringXY("2) Inercia Ix (passos)", 2, 30);
        gfx_PrintStringXY("3) Refazer figura", 2, 42);
        gfx_PrintStringXY("4) Voltar menu principal", 2, 54);
        gfx_PrintStringXY("5) Alterar unidade", 2, 66);

        /* preview abaixo */
        desenhar_secao_preview();

        /* espera tecla com borda */
        while (1) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_Key1)) { calc_centroid(1); break; }            /* centroide passos */
            if (pressed_once(kb_Key2)) { calc_Ix(1); break; }                  /* Ix passos */
            if (pressed_once(kb_Key3)) { tela_construir(); break; }
            if (pressed_once(kb_Key5)) { selecionar_unidade(); break; }
            if (pressed_once(kb_Key4) || pressed_once(kb_KeyClear)) { wait_key_release(); return; }
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
