/*  src/viga.c
    Módulo VIGA para MECSOL - TI-84 Plus CE
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

#include "beam.h"   /* já integrado ao projeto, reservado p/ usos futuros */

#define MAX_APOIOS   2
#define MAX_CARGAS_P 8
#define MAX_CARGAS_D 6
#define MAX_MOMENTOS 6
#define MAX_PONTOS   20
#define STRBUF       64

#define MAX_EVENTS (2 + MAX_APOIOS + MAX_CARGAS_P + 2*MAX_CARGAS_D + MAX_MOMENTOS)
#define MAX_LABELS (2*MAX_EVENTS + 2)

/* --- estruturas --- */
typedef struct { char tipo; double pos; double Ry; double Ma; } Apoio;
typedef struct { double pos; double F; } CargaP;
typedef struct { double x_ini, x_fim, f_ini, f_fim; } CargaD;
typedef struct { double pos, val; } Momento;

typedef struct {
    double xq;        /* ponto de amostragem (ev[i]±EPS) para posicionar a letra */
    double val;       /* V(xq) ou M(xq) */
    char   tag[4];    /* "A","B",...,"Z","AA"... */
} PtLabel;

/* --- armazenamento global --- */
static double L = 0.0;
static Apoio   apoios[MAX_APOIOS];     static int n_apoios   = 0;
static CargaP  cargas_p[MAX_CARGAS_P]; static int n_cargas_p = 0;
static CargaD  cargas_d[MAX_CARGAS_D]; static int n_cargas_d = 0;
static Momento momentos[MAX_MOMENTOS]; static int n_momentos = 0;
static double  pontos[MAX_PONTOS];     static int n_pontos   = 0;

/* ======== UTIL GRÁFICO ======== */
static void scr_clear(void) {
    gfx_FillScreen(0); /* branco (paleta 0) */
}
static void scr_print_xy(const char *s, int x, int y) {
    gfx_PrintStringXY(s, x, y);
}

/* ======== TECLADO / HINT ======== */

static inline uint8_t any_key_down(void) {
    return (kb_Data[1] | kb_Data[2] | kb_Data[3] | kb_Data[4] |
            kb_Data[5] | kb_Data[6] | kb_Data[7] | (kb_On ? 0xFF : 0));
}

static void wait_key_release(void) {
    do { kb_Scan(); } while (any_key_down());
    delay(20);
}

/* ON sai sempre */
static inline void check_on_exit(void) {
    kb_Scan();
    if (kb_On) {
        gfx_End();
        exit(0);
    }
}

static void draw_hint(const char *s) {
    if (!s) return;
    gfx_SetColor(0); /* barra inferior branca */
    gfx_FillRectangle(0, 220, 320, 12);
    gfx_SetTextFGColor(1); /* texto preto */
    gfx_PrintStringXY(s, 2, 220);
}

/* retorna true se ENTER, false se CLEAR */
static bool wait_enter_or_clear(const char *hint) {
    draw_hint(hint);
    wait_key_release();
    while (1) {
        kb_Scan();
        if (kb_On) { gfx_End(); exit(0); }
        if (kb_Data[6] & kb_Enter) { wait_key_release(); return true; }
        if (kb_Data[6] & kb_Clear) { wait_key_release(); return false; }
        delay(10);
    }
}

static void wait_enter_or_on(void) {
    wait_key_release();
    while (1) {
        kb_Scan();
        if (kb_On) { gfx_End(); exit(0); }
        if (kb_Data[6] & kb_Enter) { wait_key_release(); return; }
        delay(10);
    }
}

/* ======== INPUTS ======== */

/* lê linha SEM limpar tela inteira; prompt no topo, buffer em y=60 */
static void input_line_inline(char *buf, int maxlen, const char *prompt) {
    const int px = 2,  py = 2;
    const int bx = 2,  by = 60;
    const int bw = 316, bh = 12;

    int idx = 0;
    buf[0] = '\0';

    gfx_SetTextFGColor(1);
    gfx_PrintStringXY(prompt, px, py);

    while (1) {
        /* limpa faixa do buffer e redesenha texto digitado */
        gfx_SetColor(0);
        gfx_FillRectangle(bx, by, bw, bh);
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY(buf, bx, by);

        kb_Scan();
        if (kb_On) { gfx_End(); exit(0); }

        /* ENTER e CLEAR/DEL */
        if (kb_Data[6] & kb_Enter) { wait_key_release(); buf[idx] = '\0'; return; }
        if ((kb_Data[6] & kb_Clear) || (kb_Data[1] & kb_Del)) {
            if (idx > 0) { buf[--idx] = '\0'; }
            wait_key_release();
            continue;
        }

        /* dígitos, ponto, menos */
        char cc = 0;
        if      (kb_Data[3] & kb_0) cc='0';
        else if (kb_Data[3] & kb_1) cc='1';
        else if (kb_Data[4] & kb_2) cc='2';
        else if (kb_Data[5] & kb_3) cc='3';
        else if (kb_Data[3] & kb_4) cc='4';
        else if (kb_Data[4] & kb_5) cc='5';
        else if (kb_Data[5] & kb_6) cc='6';
        else if (kb_Data[3] & kb_7) cc='7';
        else if (kb_Data[4] & kb_8) cc='8';
        else if (kb_Data[5] & kb_9) cc='9';
        else if (kb_Data[4] & kb_DecPnt) cc='.';               /* ponto */
        else if ((kb_Data[6] & kb_Sub) || (kb_Data[5] & kb_Chs)) cc='-'; /* menos */

        if (cc && idx < maxlen-1) { buf[idx++] = cc; buf[idx] = '\0'; wait_key_release(); }

        delay(10);
    }
}

/* limpa tela e chama o inline */
static void input_line_screen(char *buf, int maxlen, const char *prompt) {
    scr_clear();
    input_line_inline(buf, maxlen, prompt);
}

/* numéricos */
static double input_double(const char *prompt) {
    char s[STRBUF];
    while (1) {
        input_line_screen(s, STRBUF, prompt);
        if (strlen(s) == 0) continue;
        for (size_t i = 0, n = strlen(s); i < n; i++) if (s[i] == ',') s[i] = '.';
        return atof(s);
    }
}

static int input_int(const char *prompt) {
    char s[STRBUF];
    while (1) {
        input_line_screen(s, STRBUF, prompt);
        if (strlen(s) == 0) continue;
        return atoi(s);
    }
}

/* ======== FÍSICA ======== */

static void calcular_reacoes(void) {
    scr_clear();
    gfx_SetTextFGColor(1);
    scr_print_xy("--- Calculando Reacoes ---", 2, 2);

    if (n_apoios == 1 && apoios[0].tipo == 'E') {
        /* Cantilever (engastada) */
        Apoio *a = &apoios[0];
        double pos_e = a->pos;
        double soma_fy = 0.0, soma_me = 0.0;

        for (int i=0;i<n_cargas_p;i++) {
            soma_fy += cargas_p[i].F;
            soma_me += cargas_p[i].F * (cargas_p[i].pos - pos_e);
        }
        for (int i=0;i<n_cargas_d;i++) {
            double x_ini = cargas_d[i].x_ini, x_fim = cargas_d[i].x_fim;
            double f_ini = cargas_d[i].f_ini, f_fim = cargas_d[i].f_fim;
            double Ld = x_fim - x_ini;
            double F_ret = f_ini * Ld;
            double F_tri = 0.5 * (f_fim - f_ini) * Ld;
            soma_fy += (F_ret + F_tri);
            double pos_ret = x_ini + Ld/2.0;
            double pos_tri = x_ini + 2.0*Ld/3.0;
            soma_me += F_ret * (pos_ret - pos_e);
            soma_me += F_tri * (pos_tri - pos_e);
        }
        for (int i=0;i<n_momentos;i++) soma_me += momentos[i].val;

        a->Ry = soma_fy;
        a->Ma = -soma_me;

        char buf[STRBUF];
        sprintf(buf, "Apoio x=%.3f m", a->pos);             scr_print_xy(buf, 2, 22);
        sprintf(buf, "  Reacao Vertical = %.3f N", a->Ry);  scr_print_xy(buf, 2, 34);
        sprintf(buf, "  Reacao Momento = %.3f Nm", a->Ma);  scr_print_xy(buf, 2, 46);

        (void)wait_enter_or_clear("ENTER/CLEAR: voltar");
        return;
    }
    else if (n_apoios == 2) {
        Apoio *A = &apoios[0], *B = &apoios[1];
        double pos_a = A->pos, pos_b = B->pos;
        double soma_ma = 0.0, soma_fy = 0.0;

        for (int i=0;i<n_cargas_p;i++) {
            soma_fy += cargas_p[i].F;
            soma_ma += cargas_p[i].F * (cargas_p[i].pos - pos_a);
        }
        for (int i=0;i<n_cargas_d;i++) {
            double x_ini=cargas_d[i].x_ini, x_fim=cargas_d[i].x_fim;
            double f_ini=cargas_d[i].f_ini, f_fim=cargas_d[i].f_fim;
            double Ld = x_fim - x_ini;
            double F_res = (f_ini + f_fim)/2.0 * Ld;
            soma_fy += F_res;
            double pos_res;
            if ((f_ini + f_fim) != 0.0)
                pos_res = x_ini + (Ld * (2.0*f_fim + f_ini)) / (3.0 * (f_ini + f_fim));
            else pos_res = x_ini + Ld/2.0;
            soma_ma += F_res * (pos_res - pos_a);
        }
        for (int i=0;i<n_momentos;i++) soma_ma += momentos[i].val;

        char buf[STRBUF];
        if (fabs(pos_b - pos_a) > 1e-9) {
            B->Ry =  soma_ma / (pos_b - pos_a);
            A->Ry = soma_fy - B->Ry;

            sprintf(buf, "Apoio x=%.3f m: Ry=%.3f N", A->pos, A->Ry);  scr_print_xy(buf, 2, 22);
            sprintf(buf, "Apoio x=%.3f m: Ry=%.3f N", B->pos, B->Ry);  scr_print_xy(buf, 2, 34);
            (void)wait_enter_or_clear("ENTER/CLEAR: voltar");
        } else {
            scr_print_xy("Apoios na mesma posicao!", 2, 22);
            (void)wait_enter_or_clear("ENTER/CLEAR: voltar");
        }
        return;
    }

    scr_print_xy("Configuracao de apoios nao suportada.", 2, 22);
    (void)wait_enter_or_clear("ENTER/CLEAR: voltar");
}

/* resolve reações sem UI, para uso em diagramas */
static bool resolver_reacoes(void) {
    if (n_apoios == 1 && apoios[0].tipo == 'E') {
        Apoio *a = &apoios[0];
        double pos_e = a->pos;
        double soma_fy = 0.0, soma_me = 0.0;

        for (int i=0;i<n_cargas_p;i++) {
            soma_fy += cargas_p[i].F;
            soma_me += cargas_p[i].F * (cargas_p[i].pos - pos_e);
        }
        for (int i=0;i<n_cargas_d;i++) {
            double x_ini=cargas_d[i].x_ini, x_fim=cargas_d[i].x_fim;
            double f_ini=cargas_d[i].f_ini, f_fim=cargas_d[i].f_fim;
            double Ld = x_fim - x_ini;
            double F_ret = f_ini * Ld;
            double F_tri = 0.5 * (f_fim - f_ini) * Ld;
            soma_fy += (F_ret + F_tri);
            double pos_ret = x_ini + Ld/2.0;
            double pos_tri = x_ini + 2.0*Ld/3.0;
            soma_me += F_ret * (pos_ret - pos_e);
            soma_me += F_tri * (pos_tri - pos_e);
        }
        for (int i=0;i<n_momentos;i++) soma_me += momentos[i].val;

        a->Ry = soma_fy;
        a->Ma = -soma_me;
        return true;
    } else if (n_apoios == 2) {
        Apoio *A=&apoios[0], *B=&apoios[1];
        double pos_a=A->pos, pos_b=B->pos;
        if (fabs(pos_b-pos_a) < 1e-9) return false;

        double soma_ma=0.0, soma_fy=0.0;
        for (int i=0;i<n_cargas_p;i++) {
            soma_fy += cargas_p[i].F;
            soma_ma += cargas_p[i].F * (cargas_p[i].pos - pos_a);
        }
        for (int i=0;i<n_cargas_d;i++) {
            double x_ini=cargas_d[i].x_ini, x_fim=cargas_d[i].x_fim;
            double f_ini=cargas_d[i].f_ini, f_fim=cargas_d[i].f_fim;
            double Ld=x_fim-x_ini;
            double F_res=(f_ini+f_fim)/2.0 * Ld;
            soma_fy += F_res;
            double pos_res;
            if ((f_ini+f_fim)!=0.0)
                pos_res = x_ini + (Ld*(2.0*f_fim+f_ini))/(3.0*(f_ini+f_fim));
            else pos_res = x_ini + Ld/2.0;
            soma_ma += F_res * (pos_res - pos_a);
        }
        for (int i=0;i<n_momentos;i++) soma_ma += momentos[i].val;

        B->Ry = soma_ma / (pos_b - pos_a);
        A->Ry = soma_fy - B->Ry;
        A->Ma = 0.0; B->Ma = 0.0;
        return true;
    }
    return false;
}

/* V(x), M(x) em um ponto x (versão modernizada) */
static void calcular_forcas_internas_em(double x, double *V_out, double *M_out) {
    double V = 0.0, M = 0.0;

    /* reações/momentos de apoio */
    for (int i=0;i<n_apoios;i++) {
        Apoio *a = &apoios[i];
        if (a->pos < x) {
            V += a->Ry;
            M -= a->Ry * (x - a->pos);
            M += a->Ma;
        }
    }

    /* cargas pontuais */
    for (int i=0;i<n_cargas_p;i++) {
        if (cargas_p[i].pos < x) {
            V -= cargas_p[i].F;
            M += cargas_p[i].F * (x - cargas_p[i].pos);
        }
    }

    /* cargas distribuídas lineares */
    for (int i=0;i<n_cargas_d;i++) {
        double a  = cargas_d[i].x_ini;
        double b  = cargas_d[i].x_fim;
        double qa = cargas_d[i].f_ini;
        double qb = cargas_d[i].f_fim;
        if (x <= a) continue;

        double Ld  = (b - a);
        double m   = (fabs(Ld) > 1e-12) ? (qb - qa) / Ld : 0.0;

        if (x <= b) {
            /* dentro da faixa: integra de a..x */
            double dx = x - a;
            double Fseg = qa*dx + 0.5*m*dx*dx;
            V -= Fseg;
            M += 0.5*qa*dx*dx + (1.0/6.0)*m*dx*dx*dx;
        } else {
            /* após o fim: usa resultante total e seu baricentro */
            double Ftot = qa*Ld + 0.5*m*Ld*Ld;                 /* = (qa+qb)/2 * Ld */
            V -= Ftot;
            double Ma_about_a = 0.5*qa*Ld*Ld + (1.0/3.0)*m*Ld*Ld*Ld;  /* ∫ t q(t) dt */
            M += (x - a)*Ftot - Ma_about_a;                 /* = Ftot*(x - x_res) */
        }
    }

    /* momentos aplicados (positivo = horário) */
    for (int i=0;i<n_momentos;i++) {
        if (momentos[i].pos < x) M -= momentos[i].val;
    }

    *V_out = V;
    *M_out = M;
}

/* mapeia [0..L] -> [x0..x0+w_beam] (usa L global) */
static inline int xmap(double xm, int x0, int w_beam) {
    if (L <= 0) return x0;
    if (xm < 0) xm = 0;
    else if (xm > L) xm = L;
    return x0 + (int)((xm * w_beam) / L + 0.5);
}

/* A, B, ... Z, AA, AB ... (estilo planilha) */
static void make_label(int idx, char out[4]) {
    char tmp[4]; int i = 0; int n = idx;
    do {
        tmp[i++] = 'A' + (n % 26);
        n = n / 26 - 1;
    } while (n >= 0 && i < 3);
    for (int k = 0; k < i; k++) out[k] = tmp[i - 1 - k];
    out[i] = '\0';
}

/* desenha a viga e carregamentos no menu (apoios S/E, cargas P/D, momentos) */
static void desenhar_viga_menu(void) {
    /* área de desenho */
    const int x0 = 8;                  /* margem esquerda */
    const int x1 = 312;                /* margem direita  */
    const int y_beam = 120;            /* linha central da viga */
    const int w_beam = x1 - x0;

    if (L <= 0) return;

    gfx_SetColor(1); /* preto */

    /* viga grossinha, lá ele (3 px) */
    gfx_FillRectangle(x0, y_beam - 1, w_beam, 3);

    /* marcações nas extremidades */
    gfx_VertLine(x0,   y_beam - 6, 12);
    gfx_VertLine(x1-1, y_beam - 6, 12);

    /* === apoios === */
    for (int i = 0; i < n_apoios; i++) {
        int x = xmap(apoios[i].pos, x0, w_beam);
        char t = apoios[i].tipo;

        if (t == 'S') { /* simples: triângulo para baixo */
            int h = 12, hw = 10;
            gfx_FillTriangle(x, y_beam, x - hw, y_beam + h, x + hw, y_beam + h);
        } else if (t == 'E') { /* engastado: bloco + hachuras à esquerda */
            int w = 6, h = 18;
            int rx = x - w/2, ry = y_beam;                 /* bloco sob a viga */
            gfx_FillRectangle(rx, ry, w, h);
            for (int k = 2; k < h; k += 4)                 /* hachuras */
                gfx_Line(rx, ry + k, rx - 8, ry + k + 3);
        } else {
            /* L (livre) ou desconhecido: nada a desenhar */
        }
    }

    /* === cargas pontuais (setas) === */
    for (int i = 0; i < n_cargas_p; i++) {
        int x = xmap(cargas_p[i].pos, x0, w_beam);
        double F = cargas_p[i].F;
        int len = 18;          /* comprimento do traço */
        int hw  = 5, hh = 7;   /* cabeça da seta */

        if (F >= 0) { /* para baixo */
            int y0 = y_beam - len, y1 = y_beam;
            gfx_Line(x, y0, x, y1);
            gfx_FillTriangle(x, y1, x - hw, y1 - hh, x + hw, y1 - hh);
        } else {      /* para cima */
            int y0 = y_beam, y1 = y_beam + len;
            gfx_Line(x, y0, x, y1);
            gfx_FillTriangle(x, y0, x - hw, y0 + hh, x + hw, y0 + hh);
        }
    }

    /* === cargas distribuídas (retângulos) === */
    for (int i = 0; i < n_cargas_d; i++) {
        int xa = xmap(cargas_d[i].x_ini, x0, w_beam);
        int xb = xmap(cargas_d[i].x_fim, x0, w_beam);
        if (xb < xa) { int tmp = xa; xa = xb; xb = tmp; }

        int h = 12;
        int y = y_beam - 26 - h; /* acima da viga */
        gfx_FillRectangle(xa, y, (xb - xa), h);
    }

    /* === momentos (círculo + “cabeça” indicando sentido) === */
    for (int i = 0; i < n_momentos; i++) {
        int x = xmap(momentos[i].pos, x0, w_beam);
        int r = 10, cy = y_beam - 20;
        double m = momentos[i].val;

        gfx_Circle(x, cy, r);
        if (m >= 0) {
            /* horário (tortona pra direita) */
            gfx_FillTriangle(x + r, cy, x + r - 6, cy - 3, x + r - 6, cy + 3);
        } else {
            /* anti-horário (tortona pra esquerda) */
            gfx_FillTriangle(x - r, cy, x - r + 6, cy - 3, x - r + 6, cy + 3);
        }
    }

    /* legenda simples: 0 e L */
    char buf[STRBUF];
    sprintf(buf, "0");   gfx_PrintStringXY(buf, x0 - 4, y_beam + 14);
    sprintf(buf, "%.2f", L);
    gfx_PrintStringXY(buf, x1 - 28, y_beam + 14);
}

/* === construção de eventos / labels / escalas === */

static void compute_scale(bool isV, const double *ev, int nev,
                          double *p_amax, double *p_u, const char **p_unit) {
    const int SAMP = 12;
    double minv=0,maxv=0; bool first=true;
    for (int i=0;i<nev-1;i++){
        for (int k=0;k<=SAMP;k++){
            double x = ev[i] + (ev[i+1]-ev[i])*(double)k/(double)SAMP;
            double V,M; calcular_forcas_internas_em(x,&V,&M);
            double y = isV?V:M;
            if(first){minv=maxv=y; first=false;} else { if(y<minv)minv=y; if(y>maxv)maxv=y; }
        }
    }
    double amax=fmax(fabs(minv),fabs(maxv)); if(amax<1e-9) amax=1.0;
    bool use_k = (amax>=1000.0);
    double u = use_k? 1.0/1000.0 : 1.0;
    *p_amax = amax; *p_u = u;
    *p_unit = isV ? (use_k? "kN":"N") : (use_k? "kN*m":"N*m");
}

static int build_labels_for(bool isV, const double *ev, int nev,
                            PtLabel *Lab, int maxL) {
    const double EPS = 1e-4;
    int n = 0; double V, M;

    /* início (x = 0+) */
    if (n < maxL) {
        double xq = ev[0] + EPS;
        if (xq > L) xq = L;
        calcular_forcas_internas_em(xq, &V, &M);
        Lab[n++] = (PtLabel){ xq, isV ? V : M, "" };
    }

    /* eventos internos: valor à esquerda e à direita de cada salto */
    for (int i = 1; i < nev - 1 && n + 2 <= maxL; i++) {
        double xl = ev[i] - EPS; if (xl < 0) xl = 0;
        double xr = ev[i] + EPS; if (xr > L) xr = L;

        calcular_forcas_internas_em(xl, &V, &M);
        Lab[n++] = (PtLabel){ xl, isV ? V : M, "" };

        calcular_forcas_internas_em(xr, &V, &M);
        Lab[n++] = (PtLabel){ xr, isV ? V : M, "" };
    }

    /* fim (x = L-) */
    if (n < maxL) {
        double xq = ev[nev - 1] - EPS; if (xq < 0) xq = 0;
        calcular_forcas_internas_em(xq, &V, &M);
        Lab[n++] = (PtLabel){ xq, isV ? V : M, "" };
    }

    /* gera A, B, C... */
    for (int i = 0; i < n; i++) make_label(i, Lab[i].tag);
    return n;
}

/* junta todos os pontos-chave do eixo x e devolve quantidade (ordenada e única) */
static int coletar_eventos(double *xs, int maxn) {
    int n = 0;
    if (L <= 0) return 0;

    /* sempre incluir extremos */
    xs[n++] = 0.0;
    xs[n++] = L;

    /* apoios */
    for (int i=0; i<n_apoios && n<maxn; i++) xs[n++] = apoios[i].pos;
    /* cargas pontuais */
    for (int i=0; i<n_cargas_p && n<maxn; i++) xs[n++] = cargas_p[i].pos;
    /* distribuídas: início e fim */
    for (int i=0; i<n_cargas_d && n+1<maxn; i++) {
        xs[n++] = cargas_d[i].x_ini;
        xs[n++] = cargas_d[i].x_fim;
    }
    /* momentos aplicados */
    for (int i=0; i<n_momentos && n<maxn; i++) xs[n++] = momentos[i].pos;

    /* ordena (insertion sort) */
    for (int i=1; i<n; i++) {
        double v = xs[i]; int j = i-1;
        while (j>=0 && xs[j] > v) { xs[j+1] = xs[j]; j--; }
        xs[j+1] = v;
    }
    /* remove duplicados muito próximos */
    int m = 0;
    for (int i=0; i<n; i++) {
        if (m==0 || fabs(xs[i] - xs[m-1]) > 1e-9) xs[m++] = xs[i];
    }
    return m;
}

/* desenha 1 diagrama (V se isV=true, M se false), com marcações verticais */
static void desenhar_diagrama_letras(bool isV,
                                     const double *ev, int nev,
                                     const PtLabel *Lab, int nlab,
                                     double amax, double u, const char *unit_title) {
    const int gx0 = 12, gx1 = 308;
    const int gy_top = 40, gy_bot = 220;
    const int gw = gx1 - gx0, gh = gy_bot - gy_top;
    const int y0 = gy_bot - gh/2;
    const int SAMP = 12;
    const double EPS = 1e-4;

    scr_clear();
    gfx_SetTextFGColor(1);

    double ys = (gh*0.42) / amax;

    char title[64];
    sprintf(title, isV ? "Diagrama de Cortante V(x) [%s]" : "Diagrama de Momento M(x) [%s]", unit_title);
    gfx_PrintStringXY(title, 8, 8);

    gfx_SetColor(1);
    gfx_HorizLine(gx0, y0, gw);

    /* marcações verticais */
    for (int i=0;i<nev;i++) {
        int xx = xmap(ev[i], gx0, gw);
        gfx_VertLine(xx, gy_top, gh);
    }

    /* curva por trechos (evita amostrar exatamente no evento) */
    int lx=0, ly=0; bool have=false;
    for (int i=0;i<nev-1;i++){
        double a=ev[i], b=ev[i+1], Ls=a+EPS, Rs=b-EPS; if(Rs<=Ls) continue;
        for (int k=0;k<=SAMP;k++){
            double x = Ls + (Rs-Ls)*(double)k/(double)SAMP;
            double V,M; calcular_forcas_internas_em(x,&V,&M);
            double val = isV?V:M;
            int px=xmap(x,gx0,gw), py=y0-(int)round(val*ys);
            if(have) gfx_Line(lx,ly,px,py);
            lx=px; ly=py; have=true;
        }
    }
    /* degraus visíveis */
    for (int i=0;i<nev;i++){
        double xl=fmax(0.0,ev[i]-EPS), xr=fmin(L,ev[i]+EPS);
        double Vl,Ml,Vr,Mr; calcular_forcas_internas_em(xl,&Vl,&Ml); calcular_forcas_internas_em(xr,&Vr,&Mr);
        double vl=isV?Vl:Ml, vr=isV?Vr:Mr;
        int px=xmap(ev[i],gx0,gw), yl=y0-(int)round(vl*ys), yr=y0-(int)round(vr*ys);
        if(yl!=yr){ int y1=(yl<yr?yl:yr), h=abs(yr-yl)+1; gfx_FillRectangle(px-1,y1,3,h); }
    }

    /* letras (A,B,...) nos pontos amostrados: início, lados dos eventos e fim */
    for (int i=0;i<nlab;i++){
        int px = xmap(Lab[i].xq, gx0, gw);
        int py = y0 - (int)round(Lab[i].val * ys);
        int y  = (Lab[i].val >= 0) ? (py - 10) : (py + 2);
        if (y < 8) y = 8; if (y > 230) y = 230;

        int w = (int)strlen(Lab[i].tag) * 6;
        int tx = px - w/2; if (tx < 2) tx = 2; if (tx + w > 318) tx = 318 - w;

        gfx_SetTextFGColor(2); /* vermelho claro */
        gfx_PrintStringXY(Lab[i].tag, tx, y);
        gfx_SetTextFGColor(1); /* volta para preto */
    }

    /* rodapé: ENTER alterna V/M, setas alternam diagrama/legenda */
    gfx_SetColor(0);
    gfx_FillRectangle(0,220,320,12);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY(isV ? "ENTER: M(x)   LEFT/RIGHT: legenda"
                          : "ENTER: V(x)   LEFT/RIGHT: legenda", 2, 220);
}

static void desenhar_legenda(bool isV,
                             const PtLabel *Lab, int nlab,
                             double u, const char *unit_title) {
    scr_clear();
    gfx_SetTextFGColor(1);

    char title[48];
    sprintf(title, isV ? "Legenda V(x) [%s]" : "Legenda M(x) [%s]", unit_title);
    gfx_PrintStringXY(title, 8, 8);

    int y = 24;
    for (int i=0;i<nlab;i++){
        char line[64];
        double val = Lab[i].val * u;
        if (fabs(val) < 1000.0) sprintf(line, "%-3s x=%.2f m  %.2f %s", Lab[i].tag, Lab[i].xq, val, unit_title);
        else                    sprintf(line, "%-3s x=%.2f m  %.1f %s", Lab[i].tag, Lab[i].xq, val, unit_title);
        gfx_PrintStringXY(line, 8, y);
        y += 12; if (y > 208) break;
    }

    gfx_SetColor(0);
    gfx_FillRectangle(0,220,320,12);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("LEFT/RIGHT: diagrama   ENTER: alternar", 2, 220);
}

static void mostrar_diagramas(void) {
    if (!resolver_reacoes()) {
        scr_clear();
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("Configuracao de apoios nao suportada.", 8, 18);
        draw_hint("ENTER: voltar");
        wait_enter_or_on();
        return;
    }

    double ev[MAX_EVENTS];
    int nev = coletar_eventos(ev, MAX_EVENTS);
    if (nev < 2) {
        scr_clear();
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("Sem eventos.", 8, 18);
        wait_enter_or_on();
        return;
    }

    bool isV = true;     /* começa no V */
    bool legend = false; /* começa no diagrama */

    while (1) {
        /* escala (amax/u/unidade) e letras para o diagrama atual */
        double amax,u; const char *unit;
        compute_scale(isV, ev, nev, &amax, &u, &unit);

        PtLabel Ls[MAX_LABELS];
        int nlab = build_labels_for(isV, ev, nev, Ls, MAX_LABELS);

        if (legend) desenhar_legenda(isV, Ls, nlab, u, unit);
        else        desenhar_diagrama_letras(isV, ev, nev, Ls, nlab, amax, u, unit);

        /* teclado: CLEAR sai; ENTER alterna V/M; setas alternam diagrama/legenda */
        wait_key_release();
        while (1) {
            kb_Scan();
            if (kb_On) { gfx_End(); exit(0); }
            if (kb_Data[6] & kb_Clear) return;
            if (kb_Data[6] & kb_Enter) { isV = !isV; break; }
            if (kb_Data[7] & (kb_Left | kb_Right)) { legend = !legend; break; }
            delay(10);
        }
    }
}

/* ======== ENTRADA DE DADOS ======== */
static void obter_dados(void) {
    char tmp[STRBUF];

    /* 1) comprimento */
    while (1) {
        double t = input_double("1) Comprimento da viga (m):");
        if (t > 0.0) { L = t; break; }
    }

    /* 2) apoios */
    while (1) {
        int na = input_int("2) N de apoios (1 ou 2):");
        if (na >=1 && na <=2) { n_apoios = na; break; }
    }
    for (int i=0;i<n_apoios;i++) {
        sprintf(tmp, "3) Apoio %d - Tipo (1=Simples, 2=Engastado):", i+1);
        int ta = input_int(tmp);
        apoios[i].tipo = (ta == 2) ? 'E' : 'S';

        while (1) {
            sprintf(tmp, "   Apoio %d - Posicao (m, 0..%.3f):", i+1, L);
            double p = input_double(tmp);
            if (p >= 0.0 && p <= L) { apoios[i].pos = p; break; }
        }
        apoios[i].Ry = 0.0; apoios[i].Ma = 0.0;
    }

    /* 4) cargas pontuais */
    n_cargas_p = input_int("4) N de cargas pontuais (0..8):");
    if (n_cargas_p > MAX_CARGAS_P) n_cargas_p = MAX_CARGAS_P;
    for (int i=0;i<n_cargas_p;i++) {
        sprintf(tmp, "5) Carga pontual %d - Posicao (m):", i+1);
        cargas_p[i].pos = input_double(tmp);
        sprintf(tmp, "   Carga pontual %d - Forca (N, >0 p/ baixo):", i+1);
        cargas_p[i].F = input_double(tmp);
    }

    /* 6) cargas distribuidas */
    n_cargas_d = input_int("6) N de cargas distribuidas (0..6):");
    if (n_cargas_d > MAX_CARGAS_D) n_cargas_d = MAX_CARGAS_D;
    for (int i=0;i<n_cargas_d;i++) {
        sprintf(tmp, "7) Carga dist %d - X inicial (m):", i+1);
        cargas_d[i].x_ini = input_double(tmp);
        sprintf(tmp, "   Carga dist %d - X final (m):", i+1);
        cargas_d[i].x_fim = input_double(tmp);
        sprintf(tmp, "   Carga dist %d - F inicial (N/m):", i+1);
        cargas_d[i].f_ini = input_double(tmp);
        sprintf(tmp, "   Carga dist %d - F final (N/m):", i+1);
        cargas_d[i].f_fim = input_double(tmp);
    }

    /* 8) momentos */
    n_momentos = input_int("8) N de momentos (0..6):");
    if (n_momentos > MAX_MOMENTOS) n_momentos = MAX_MOMENTOS;
    for (int i=0;i<n_momentos;i++) {
        sprintf(tmp, "9) Momento %d - Posicao (m):", i+1);
        momentos[i].pos = input_double(tmp);
        sprintf(tmp, "   Momento %d - Valor (Nm, horario>0):", i+1);
        momentos[i].val = input_double(tmp);
    }

    /* 10/11) pontos de interesse */
    n_pontos = input_int("10) N de pontos p/ calculo (0..20):");
    if (n_pontos > MAX_PONTOS) n_pontos = MAX_PONTOS;
    for (int i=0;i<n_pontos;i++) {
        sprintf(tmp, "11) Ponto %d - Posicao (m, 0..%.3f):", i+1, L);
        while (1) {
            double px = input_double(tmp);
            if (px >= 0.0 && px <= L) { pontos[i] = px; break; }
        }
    }
}

/* ======== MENU ======== */

static char ler_opcao_menu(void) {
    char sel[STRBUF];

    scr_clear();
    gfx_SetTextFGColor(1);
    scr_print_xy("=== MENU VIGA ===", 2, 2);
    scr_print_xy("1) Calcular Reacoes de Apoio", 2, 18);
    scr_print_xy("2) Forcas Internas nos Pontos", 2, 30);
    scr_print_xy("3) Diagramas V e M", 2, 42);
    scr_print_xy("4) Voltar menu principal", 2, 54);

    desenhar_viga_menu();  /* viga desenhada abaixo do menu */

    input_line_inline(sel, STRBUF, "Escolha (1-4) e ENTER:");
    return sel[0];
}

static void menu_loop(void) {
    while (1) {
        char op = ler_opcao_menu();

        if (op == '1') {
            scr_clear();
            calcular_reacoes(); /* espera ENTER/CLEAR dentro */
        }
        else if (op == '2') {
            if (n_pontos == 0) {
                scr_clear();
                gfx_SetTextFGColor(1);
                scr_print_xy("Nenhum ponto definido.", 2, 18);
                (void)wait_enter_or_clear("ENTER/CLEAR: voltar ao menu");
            } else {
                for (int i=0;i<n_pontos;i++) {
                    double V, M; char buf[STRBUF];
                    calcular_forcas_internas_em(pontos[i], &V, &M);

                    scr_clear();
                    gfx_SetTextFGColor(1);
                    sprintf(buf, "Resultado no ponto %d/%d", i+1, n_pontos);  scr_print_xy(buf, 2, 2);
                    sprintf(buf, "x = %.3f m", pontos[i]);                    scr_print_xy(buf, 2, 18);
                    sprintf(buf, "  V = %.3f N", V);                           scr_print_xy(buf, 2, 30);
                    sprintf(buf, "  M = %.3f Nm", M);                          scr_print_xy(buf, 2, 42);

                    if (!wait_enter_or_clear("ENTER: proximo   CLEAR: menu"))
                        break;
                }
            }
        }
        else if (op == '3') {
            mostrar_diagramas(); /* alterna V/M com ENTER, CLEAR volta */
        }
        else if (op == '4' || op == '0' || op == '9') {
            /* Voltar ao menu principal do MECAN */
            return;
        }
    }
}

/* Função pública chamada pelo main.c do MECAN */
void viga_module(void) {
    /* Se ainda não tiver viga definida, pede dados */
    if (L <= 0.0 || n_apoios == 0) {
        obter_dados();
    }
    menu_loop();
}
