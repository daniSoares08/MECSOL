/*  src/tensoes.c
    Módulo MAX. TENSOES para MECSOL - TI-84 Plus CE
    Usa:
      - Seção / formato do módulo CENTROID (x_bar, y_bar, Ix, unidade)
      - Viga do módulo VIGA (M(x) ou M_max)

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

#define STRBUF 64

typedef struct { double val; const char *unit; double factor; } DispVal;
typedef struct { double factor; const char *name; } UnitOpt;

/* ======== API externa que vem de centroid.c ======== */
/* (implementadas lá no fim do arquivo) */

int   centroid_has_figure(void);
void  centroid_get_centroid(double *px, double *py);
double centroid_get_Ix(void);
void  centroid_get_y_bounds(double *pymin, double *pymax);
const char *centroid_get_unit_name(void);
double centroid_get_unit_factor(void);

/* ======== API externa que vem de viga.c ======== */

int    viga_has_beam(void);
double viga_get_length(void);
double viga_momento_em(double x);
double viga_momento_max_abs(double *px_max);

/* ======== UNIDADES (escolha dinâmica mm/cm/m apenas para exibir) ======== */

static const UnitOpt UOPTS[] = {
    {0.001, "mm"},
    {0.01,  "cm"},
    {1.0,   "m"},
};

static const UnitOpt *pick_unit(double meters, double preferred_factor) {
    const UnitOpt *pref = &UOPTS[2];
    for (size_t i=0;i<3;i++) if (fabs(UOPTS[i].factor - preferred_factor) < 1e-12) pref = &UOPTS[i];

    const double LOW = 0.01;
    const double HIGH = 9999.0;

    const UnitOpt *best = pref;
    double val = (best->factor > 0.0) ? meters / best->factor : meters;
    double amag = fabs(val);

    if (amag > HIGH && best->factor < 1.0) {
        if (best->factor < 0.01) best = &UOPTS[1]; else best = &UOPTS[2];
    } else if (amag > 0 && amag < LOW && best->factor > 0.001) {
        if (best->factor > 0.01) best = &UOPTS[1]; else best = &UOPTS[0];
    }
    return best;
}

static DispVal disp_len(double meters, double preferred_factor) {
    const UnitOpt *u = pick_unit(meters, preferred_factor);
    double v = (u->factor > 0.0) ? meters / u->factor : meters;
    return (DispVal){ v, u->name, u->factor };
}

static DispVal disp_pow(double meters_pow, double preferred_factor, int power) {
    const UnitOpt *u = pick_unit(pow(fabs(meters_pow), 1.0/power), preferred_factor);
    double f = u->factor;
    double denom = 1.0;
    for (int i=0;i<power;i++) denom *= f;
    double v = (f > 0.0) ? meters_pow / denom : meters_pow;
    return (DispVal){ v, u->name, u->factor };
}

static DispVal disp_m4(double m4, double preferred_factor) {
    return disp_pow(m4, preferred_factor, 4);
}

/* converte SIG [N/m^2] para N/(unit)^2 usando o fator da unidade escolhida */
static double sig_to_unit(double sig_N_m2, double unit_factor) {
    return sig_N_m2 * unit_factor * unit_factor;
}

/* ======== TECLADO / UTIL ======== */

static inline void check_on_exit(void) {
    kb_Scan();
    if (kb_On) {
        gfx_End();
        exit(0);
    }
}

/* espera soltar todas as teclas (igual outros módulos) */
static void wait_key_release(void) {
    do {
        kb_Scan();
    } while (kb_Data[1] | kb_Data[2] | kb_Data[3] |
             kb_Data[4] | kb_Data[5] | kb_Data[6] | kb_Data[7]);
    delay(15);
}

/* pressed_once: mesmo estilo do centroid.c */
static uint8_t pressed_once(kb_lkey_t lk) {
    static uint8_t prev_enter=0, prev_clear=0;
    static uint8_t prev0=0, prev1=0, prev2=0, prev3=0, prev4=0;
    static uint8_t prev5=0, prev6=0, prev7=0, prev8=0, prev9=0;
    static uint8_t prevDot=0, prevNeg=0;
    uint8_t *prev=NULL;
    switch(lk) {
        case kb_KeyEnter:  prev=&prev_enter; break;
        case kb_KeyClear:  prev=&prev_clear; break;
        case kb_Key0:      prev=&prev0; break;
        case kb_Key1:      prev=&prev1; break;
        case kb_Key2:      prev=&prev2; break;
        case kb_Key3:      prev=&prev3; break;
        case kb_Key4:      prev=&prev4; break;
        case kb_Key5:      prev=&prev5; break;
        case kb_Key6:      prev=&prev6; break;
        case kb_Key7:      prev=&prev7; break;
        case kb_Key8:      prev=&prev8; break;
        case kb_Key9:      prev=&prev9; break;
        case kb_KeyDecPnt: prev=&prevDot; break;
        case kb_KeyChs:    prev=&prevNeg; break;
        default: return 0;
    }
    uint8_t down = kb_IsDown(lk) ? 1 : 0;
    uint8_t edge = (down && !*prev);
    *prev = down;
    return edge;
}

/* Espera ENTER ou CLEAR (tela simples) */
static void wait_enter_or_clear_tens(void) {
    wait_key_release();
    while (1) {
        check_on_exit();
        kb_Scan();
        if (pressed_once(kb_KeyEnter)) return;
        if (pressed_once(kb_KeyClear)) return;
        delay(10);
    }
}

/* ======== INPUT NUMÉRICO (estilo centroid.c) ======== */

static void input_line(char *buf, int maxlen, const char *prompt){
    int idx=0;
    bool dirty = true;
    buf[0]='\0';

    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY(prompt, 2, 2);
    gfx_PrintStringXY("ENTER=ok CLEAR=apaga ON=sair", 2, 78);

    for(;;){
        check_on_exit();
        kb_Scan();

        if (dirty) {
            gfx_SetColor(0);
            gfx_FillRectangle(0, 18, 320, 12);
            gfx_SetTextFGColor(1);
            gfx_PrintStringXY(buf, 2, 18);
            dirty = false;
        }

        if (pressed_once(kb_KeyEnter)) {
            buf[idx]='\0';
            return;
        }
        if (pressed_once(kb_KeyClear)) {
            if (idx>0){
                idx--;
                buf[idx]='\0';
                dirty = true;
            }
        }
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
    char s[STRBUF];
    input_line(s, STRBUF, prompt);
    for (int i=0; s[i]; ++i) if (s[i] == ',') s[i] = '.';
    return atof(s);
}

/* ======== CONVERSÕES DE UNIDADE ======== */

/* ======== TELAS DAS 3 ETAPAS ======== */

/* etapa 1: centroide */
static void draw_page_centroid(double xbar, double ybar,
                               const char *unit_name,
                               double unit_factor) {
    (void)unit_name; /* unidade real exibida ajustada dinamicamente */
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);

    gfx_PrintStringXY("ETAPA 1 - CENTROIDE DA SECAO", 2, 2);

    gfx_PrintStringXY("Formulas:", 2, 20);
    gfx_PrintStringXY("x_bar = Sum(Ai*xi) / Sum(Ai)", 2, 32);
    gfx_PrintStringXY("y_bar = Sum(Ai*yi) / Sum(Ai)", 2, 44);

    char buf[STRBUF];
    DispVal dx = disp_len(xbar, unit_factor);
    sprintf(buf, "x_bar = %.3f %s", dx.val, dx.unit);
    gfx_PrintStringXY(buf, 2, 70);
    DispVal dy = disp_len(ybar, unit_factor);
    sprintf(buf, "y_bar = %.3f %s", dy.val, dy.unit);
    gfx_PrintStringXY(buf, 2, 82);

    gfx_PrintStringXY("Detalhe completo em: Formato > Centroide", 2, 110);
}

/* etapa 2: Ix + distancias extremas ao eixo neutro */
static void draw_page_inercia(double Ix,
                              double y_sup, double y_inf,
                              const char *unit_name,
                              double unit_factor) {
    (void)unit_name;
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);

    gfx_PrintStringXY("ETAPA 2 - INERCIA Ix", 2, 2);

    gfx_PrintStringXY("Ix = Sum( Ixc_i + Ai*di^2 )", 2, 20);
    gfx_PrintStringXY("Ixc_i (ret) = b*h^3 / 12", 2, 32);

    char buf[STRBUF];
    DispVal dIx = disp_m4(Ix, unit_factor);
    sprintf(buf, "Ix = %.3f %s^4", dIx.val, dIx.unit);
    gfx_PrintStringXY(buf, 2, 56);

    DispVal dSup = disp_len(y_sup, unit_factor);
    sprintf(buf, "y_sup = %.3f %s (fibra superior)", dSup.val, dSup.unit);
    gfx_PrintStringXY(buf, 2, 74);

    DispVal dInf = disp_len(y_inf, unit_factor);
    sprintf(buf, "y_inf = %.3f %s (fibra inferior)", dInf.val, dInf.unit);
    gfx_PrintStringXY(buf, 2, 86);

    gfx_PrintStringXY("Detalhe em: Formato > Inercia Ix", 2, 110);
}

/* origem: 0=manual sem viga, 1=viga ponto x, 2=viga Mmax, 3=viga simples P em L */
static void draw_page_tensoes(double M, double x_pos, int origem,
                              double xbar, double ybar,
                              double ymin, double ymax,
                              double Ix,
                              const char *unit_name,
                              double unit_factor) {
    (void)unit_name;
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);

    gfx_PrintStringXY("ETAPA 3 - TENSOES POR FLEXAO", 2, 2);

    char buf[STRBUF];
    DispVal dxpos = disp_len(x_pos, unit_factor);

    if (origem == 0) {
        gfx_PrintStringXY("Caso: momento informado (sem viga)", 2, 18);
    } else if (origem == 1) {
        sprintf(buf, "Caso: viga no ponto x=%.3f %s", dxpos.val, dxpos.unit);
        gfx_PrintStringXY(buf, 2, 18);
    } else if (origem == 2) {
        sprintf(buf, "Caso: Mmax da viga em x=%.3f %s", dxpos.val, dxpos.unit);
        gfx_PrintStringXY(buf, 2, 18);
    } else {
        sprintf(buf, "Caso: viga simples (P em L, x=%.3f %s)", dxpos.val, dxpos.unit);
        gfx_PrintStringXY(buf, 2, 18);
    }

    sprintf(buf, "M = %.3f N*m (%.3f kN*m)", M, M/1000.0);
    gfx_PrintStringXY(buf, 2, 32);

    gfx_PrintStringXY("Formula geral:", 2, 50);
    gfx_PrintStringXY("SIG = - M * y / Ix", 2, 62);

    /* y relativos ao eixo neutro (y>0 para cima) */
    double dy_sup = ymax - ybar;   /* >0 */
    double dy_inf = ymin - ybar;   /* <0 (fibra inferior) */

    double y_sup_abs = fabs(dy_sup);
    double y_inf_abs = fabs(dy_inf);

    DispVal dy_disp = disp_len(y_sup_abs, unit_factor); /* usa mesma unidade para sup/inf */
    double y_sup_show = dy_disp.val;
    double y_inf_show = dy_inf < 0 ? -(y_inf_abs / dy_disp.factor)
                                   :  (y_inf_abs / dy_disp.factor);

    sprintf(buf, "y_sup = +%.3f %s", y_sup_show, dy_disp.unit);
    gfx_PrintStringXY(buf, 2, 80);
    sprintf(buf, "y_inf = %.3f %s", y_inf_show, dy_disp.unit);
    gfx_PrintStringXY(buf, 2, 92);

    if (Ix == 0.0) {
        gfx_PrintStringXY("Ix = 0 -> nao e possivel obter SIG.", 2, 112);
        return;
    }

    /* SIG no topo e na base (em N/m^2) */
    double sig_sup = -M * dy_sup / Ix;
    double sig_inf = -M * dy_inf / Ix;

    /* converte para N/(unidade)^2 */
    double sig_sup_u = sig_to_unit(sig_sup, dy_disp.factor);
    double sig_inf_u = sig_to_unit(sig_inf, dy_disp.factor);

    sprintf(buf, "SIG_sup = %.3f N/%s^2", sig_sup_u, dy_disp.unit);
    gfx_PrintStringXY(buf, 2, 118);
    sprintf(buf, "SIG_inf = %.3f N/%s^2", sig_inf_u, dy_disp.unit);
    gfx_PrintStringXY(buf, 2, 130);

    /* máximas de tracao (>0) e compressao (<0) */
    double sig_trac = 0.0, sig_comp = 0.0;

    if (sig_sup_u >= 0.0 && sig_inf_u <= 0.0) {
        sig_trac = sig_sup_u;
        sig_comp = sig_inf_u;
    } else if (sig_sup_u <= 0.0 && sig_inf_u >= 0.0) {
        sig_trac = sig_inf_u;
        sig_comp = sig_sup_u;
    } else {
        /* caso raro: ambos com o mesmo sinal */
        if (fabs(sig_sup_u) >= fabs(sig_inf_u)) {
            if (sig_sup_u >= 0.0) sig_trac = sig_sup_u;
            else sig_comp = sig_sup_u;
        } else {
            if (sig_inf_u >= 0.0) sig_trac = sig_inf_u;
            else sig_comp = sig_inf_u;
        }
    }

    int y = 150;
    sprintf(buf, "SIG_tracao(max) = %.3f N/%s^2", sig_trac, dy_disp.unit);
    gfx_PrintStringXY(buf, 2, y); y += 12;
    sprintf(buf, "SIG_comp(max)   = %.3f N/%s^2", sig_comp, dy_disp.unit);
    gfx_PrintStringXY(buf, 2, y);
}

/* ======== MOSTRAR AS 3 ETAPAS COM NAVEGACAO ======== */
/* origem: 0=manual, 1=ponto viga, 2=Mmax viga, 3=viga simples P em L */

static void mostrar_etapas(double M, double x_pos, int origem) {
    if (!centroid_has_figure()) return;

    double xbar, ybar;
    centroid_get_centroid(&xbar, &ybar);

    double ymin, ymax;
    centroid_get_y_bounds(&ymin, &ymax);

    double Ix = centroid_get_Ix();
    const char *unit_name = centroid_get_unit_name();
    double unit_factor = centroid_get_unit_factor();

    /* distancias extremas positivas (usadas na tela 2) */
    double y_sup = fabs(ymax - ybar);
    double y_inf = fabs(ybar - ymin);

    int page = 0;
    const int PAGE_MAX = 2; /* 0: centroide, 1: Ix, 2: SIG */

    wait_key_release();

    while (1) {
        if (page == 0) {
            draw_page_centroid(xbar, ybar, unit_name, unit_factor);
        } else if (page == 1) {
            draw_page_inercia(Ix, y_sup, y_inf, unit_name, unit_factor);
        } else {
            draw_page_tensoes(M, x_pos, origem,
                              xbar, ybar, ymin, ymax,
                              Ix, unit_name, unit_factor);
        }

        gfx_SetTextFGColor(1);
        if (page < PAGE_MAX)
            gfx_PrintStringXY("LEFT/RIGHT: paginas  ENTER: prox  CLEAR: sair", 2, 220);
        else
            gfx_PrintStringXY("LEFT/RIGHT: paginas  ENTER/CLEAR: sair", 2, 220);

        while (1) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_KeyLeft)) {
                if (page > 0) page--;
                break;
            }
            if (pressed_once(kb_KeyRight)) {
                if (page < PAGE_MAX) page++;
                break;
            }
            if (pressed_once(kb_KeyEnter)) {
                if (page < PAGE_MAX)
                    page++;
                else
                    return;
                break;
            }
            if (pressed_once(kb_KeyClear)) {
                return;
            }
            delay(10);
        }
    }
}

/* ======== FLUXOS DO MÓDULO ======== */

/* sem formato definido -> só avisa e volta */
static void fluxo_sem_formato(void) {
    gfx_FillScreen(0);
    gfx_SetTextFGColor(1);
    gfx_PrintStringXY("=== MAX. TENSOES ===", 2, 2);
    gfx_PrintStringXY("Nenhum FORMATO definido.", 2, 24);
    gfx_PrintStringXY("Use o menu 'Formato' para criar", 2, 36);
    gfx_PrintStringXY("a secao (retangulos / recortes).", 2, 48);
    gfx_PrintStringXY("ENTER/CLEAR: voltar", 2, 210);
    wait_enter_or_clear_tens();
}

/* formato OK, mas sem viga definida */
static void fluxo_sem_viga(void) {
    while (1) {
        gfx_FillScreen(0);
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("=== MAX. TENSOES ===", 2, 2);
        gfx_PrintStringXY("Formato: OK (centroide & Ix)", 2, 20);
        gfx_PrintStringXY("Viga: NAO DEFINIDA", 2, 32);

        gfx_PrintStringXY("1) SIG com M conhecido", 2, 52);
        gfx_PrintStringXY("2) Calcular M em viga simples", 2, 64);
        gfx_PrintStringXY("3) Adicionar viga (voltar)", 2, 76);
        gfx_PrintStringXY("ENTER/1..3 escolhe, CLEAR volta", 2, 96);

        uint8_t opt = 0;
        while (!opt) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_Key1) || pressed_once(kb_KeyEnter)) opt = 1;
            else if (pressed_once(kb_Key2)) opt = 2;
            else if (pressed_once(kb_Key3) || pressed_once(kb_KeyClear)) opt = 3;
            delay(10);
        }

        if (opt == 1) {
            /* Momento informado direto */
            double M = input_double("Momento M (Nm, sinal conv.):");
            mostrar_etapas(M, 0.0, 0);
        }
        else if (opt == 2) {
            /* Viga simplesmente apoiada com carga P em a (bem basico) */
            double L = input_double("Viga simples: comprimento L (m):");
            if (L <= 0.0) continue;

            double P = input_double("Carga pontual P (N, >0 p/baixo):");

            double a;
            while (1) {
                char buf[STRBUF];
                sprintf(buf, "Posicao da carga a (m, 0..%.3f):", L);
                a = input_double(buf);
                if (a >= 0.0 && a <= L) break;
            }

            /* Reacao em A (RA) e momento max sob a carga: Mmax = RA*a */
            double RA = P * (L - a) / L;
            double Mmax = RA * a; /* sagging positivo */
            mostrar_etapas(Mmax, a, 3);
        }
        else {
            /* opt 3: apenas volta pro menu principal (pra criar viga) */
            return;
        }
    }
}

/* formato OK e viga definida */
static void fluxo_com_viga(void) {
    while (1) {
        gfx_FillScreen(0);
        gfx_SetTextFGColor(1);
        gfx_PrintStringXY("=== MAX. TENSOES ===", 2, 2);
        gfx_PrintStringXY("Formato: OK", 2, 20);
        gfx_PrintStringXY("Viga: DEFINIDA", 2, 32);

        gfx_PrintStringXY("1) SIG em ponto x da viga", 2, 52);
        gfx_PrintStringXY("2) SIG max (tracao/comp.)", 2, 64);
        gfx_PrintStringXY("3) Voltar", 2, 76);
        gfx_PrintStringXY("ENTER/1..3 escolhe, CLEAR volta", 2, 96);

        uint8_t opt = 0;
        while (!opt) {
            check_on_exit();
            kb_Scan();
            if (pressed_once(kb_Key1) || pressed_once(kb_KeyEnter)) opt = 1;
            else if (pressed_once(kb_Key2)) opt = 2;
            else if (pressed_once(kb_Key3) || pressed_once(kb_KeyClear)) opt = 3;
            delay(10);
        }

        if (opt == 1) {
            /* SIG em ponto x da viga (usa M(x) da viga) */
            double L = viga_get_length();
            double x;
            while (1) {
                char buf[STRBUF];
                sprintf(buf, "Posicao x (m, 0..%.3f):", L);
                x = input_double(buf);
                if (x >= 0.0 && x <= L) break;
            }
            double M = viga_momento_em(x);
            mostrar_etapas(M, x, 1);
        }
        else if (opt == 2) {
            /* SIG max -> Mmax ao longo da viga */
            double xM = 0.0;
            double M = viga_momento_max_abs(&xM);
            mostrar_etapas(M, xM, 2);
        }
        else {
            return;
        }
    }
}

/* ======== API PUBLICA ======== */

void tensoes_module(void) {
    /* 1) precisa ter FORMATO */
    if (!centroid_has_figure()) {
        fluxo_sem_formato();
        return;
    }

    /* 2) checa se ha viga ja criada */
    if (!viga_has_beam()) {
        fluxo_sem_viga();
    } else {
        fluxo_com_viga();
    }
}
