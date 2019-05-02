#ifndef MAQUINA1
#define MAQUINA1



typedef struct {
	const uint8_t *data;
	uint16_t width;
	uint16_t height;
	uint8_t dataSize;
} tImage;


#include "_ionicons_svg_md-lock.h"
#include "_ionicons_svg_md-unlock.h"
#include "a60e3f50683d3063c36bb8f4f08e9ad0.h";
#include "back.h";
#include "centrifuge.h";
#include "fast.h";
#include "kilograms-weight-.h";
#include "next.h";
#include "Washing-Icon-300x280.h";
#include "washing-machine-icon-22.h";
#include "left-arrow.h"
#include "comeca.h"

const uint32_t BUTTON_W = 120;
const uint32_t BUTTON_H = 150;
const uint32_t BUTTON_BORDER = 2;
const uint32_t BUTTON_X = ILI9488_LCD_WIDTH/2;
const uint32_t BUTTON_Y = ILI9488_LCD_HEIGHT/2;

const uint32_t ARROW_W = 50;
const uint32_t ARROW_H = 100;
const uint32_t ARROW_X = 0;
const uint32_t ARROW_Y = 275;

const uint32_t BOX_Y = 100;
const uint32_t BOX_H = 325;

const uint32_t ICON_X = 0+50+55;
const uint32_t ICON_Y = 150;
const uint32_t LABEL_X = 0+50+55;
const uint32_t LABEL_Y = 310;

const uint32_t LOCK_X = 250;
const uint32_t LOCK_Y = 25;

const uint32_t START_X = 100;
const uint32_t START_Y = 325;

const uint32_t BACK_X = 25;
const uint32_t BACK_Y = 25;

const uint32_t DOOR_X = (ILI9488_LCD_WIDTH/2)-25;
const uint32_t DOOR_Y = 29;

const uint32_t CLOCK_X = 40;
const uint32_t CLOCK_Y = 400;

const uint32_t WARN_X = 5;
const uint32_t WARN_Y = 455;

const uint32_t TIME_Y0 = 390;
const uint32_t TIME_Y1 = 415;

typedef struct ciclo t_ciclo;

struct ciclo{
	char nome[32];           // nome do ciclo, para ser exibido
	int  enxagueTempo;       // tempo que fica em cada enxague
	int  enxagueQnt;         // quantidade de enxagues
	int  centrifugacaoRPM;   // velocidade da centrifugacao
	int  centrifugacaoTempo; // tempo que centrifuga
	char heavy;              // modo pesado de lavagem
	char bubblesOn;          // smart bubbles on (???)
	t_ciclo *previous;
    t_ciclo *next;
	tImage *image;
};

t_ciclo c_rapido = {.nome = "Rapido",
	.enxagueTempo = 5,
	.enxagueQnt = 3,
	.centrifugacaoRPM = 900,
	.centrifugacaoTempo = 5,
	.heavy = 0,
	.bubblesOn = 1,
	.image = &fast
};

t_ciclo c_diario = {.nome = "Diario",
	.enxagueTempo = 15,
	.enxagueQnt = 2,
	.centrifugacaoRPM = 1200,
	.centrifugacaoTempo = 8,
	.heavy = 0,
	.bubblesOn = 1,
	.image = &washingmachineicon22
};

t_ciclo c_pesado = {.nome = "Pesado",
	.enxagueTempo = 10,
	.enxagueQnt = 3,
	.centrifugacaoRPM = 1200,
	.centrifugacaoTempo = 10,
	.heavy = 1,
	.bubblesOn = 1,
	.image = &kilogramsweight
};

t_ciclo c_enxague = {.nome = "Enxague",
	.enxagueTempo = 10,
	.enxagueQnt = 1,
	.centrifugacaoRPM = 0,
	.centrifugacaoTempo = 0,
	.heavy = 0,
	.bubblesOn = 0,
	.image =  &WashingIcon300x280
};

t_ciclo c_centrifuga = {.nome = "Centrifuga",
	.enxagueTempo = 0,
	.enxagueQnt = 0,
	.centrifugacaoRPM = 1200,
	.centrifugacaoTempo = 10,
	.heavy = 0,
	.bubblesOn = 0,
	.image = &centrifuge
};

//t_ciclo *initMenuOrder();

#endif