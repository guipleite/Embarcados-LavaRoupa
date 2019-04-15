#include <asf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "conf_board.h"
#include "conf_example.h"
#include "conf_uart_serial.h"
#include "maquina1.h"
// #include "_ionicons_svg_md-lock.h"
// #include "_ionicons_svg_md-time.h"
// #include "_ionicons_svg_md-unlock.h"
// #include "_ionicons_svg_md-water.h"

#define MAX_ENTRIES        3
#define STRING_LENGTH     70

#define USART_TX_MAX_LENGTH     0xff

#define YEAR        2019
#define MONTH       3
#define DAY         19
#define WEEK        12
#define HOUR        0
#define MINUTE      0
#define SECOND      0
uint32_t hour, minu, seg;

struct ili9488_opt_t g_ili9488_display_opt;
const uint32_t BUTTON_W = 120;
const uint32_t BUTTON_H = 150;
const uint32_t BUTTON_BORDER = 2;
const uint32_t BUTTON_X = ILI9488_LCD_WIDTH/2;
const uint32_t BUTTON_Y = ILI9488_LCD_HEIGHT/2;

const uint32_t ARROW_W = 50;
const uint32_t ARROW_H = 100;
const uint32_t ARROW_X = 0;
const uint32_t ARROW_Y = 275;

/**
 * Inicializa ordem do menu
 * retorna o primeiro ciclo que
 * deve ser exibido.
 */
t_ciclo *initMenuOrder(){
  c_rapido.previous = &c_enxague;
  c_rapido.next = &c_diario;

  c_diario.previous = &c_rapido;
  c_diario.next = &c_pesado;

  c_pesado.previous = &c_diario;
  c_pesado.next = &c_enxague;

  c_enxague.previous = &c_pesado;
  c_enxague.next = &c_centrifuga;

  c_centrifuga.previous = &c_enxague;
  c_centrifuga.next = &c_rapido;

  return(&c_diario);
}
// Variaveis boobleanas para indicar os estados
volatile Bool f_rtt_alarme = false;
volatile Bool start = false;
volatile Bool locked = false;
volatile Bool selection = true;

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);
void RTT_Handler(void){
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		f_rtt_alarme = true;                  // flag RTT alarme
	}
}
static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}
static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses){
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}
static void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
}
/**
 * \brief Set maXTouch configuration
 *
 * This function writes a set of predefined, optimal maXTouch configuration data
 * to the maXTouch Xplained Pro.
 *
 * \param device Pointer to mxt_device struct
 */
static void mxt_init(struct mxt_device *device){
	enum status_code status;

	/* T8 configuration object data */
	uint8_t t8_object[] = {
		0x0d, 0x00, 0x05, 0x0a, 0x4b, 0x00, 0x00,
		0x00, 0x32, 0x19
	};

	/* T9 configuration object data */
	uint8_t t9_object[] = {
		0x8B, 0x00, 0x00, 0x0E, 0x08, 0x00, 0x80,
		0x32, 0x05, 0x02, 0x0A, 0x03, 0x03, 0x20,
		0x02, 0x0F, 0x0F, 0x0A, 0x00, 0x00, 0x00,
		0x00, 0x18, 0x18, 0x20, 0x20, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x02,
		0x02
	};

	/* T46 configuration object data */
	uint8_t t46_object[] = {
		0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x03,
		0x00, 0x00
	};
	
	/* T56 configuration object data */
	uint8_t t56_object[] = {
		0x02, 0x00, 0x01, 0x18, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	/* TWI configuration */
	twihs_master_options_t twi_opt = {
		.speed = MXT_TWI_SPEED,
		.chip  = MAXTOUCH_TWI_ADDRESS,
	};

	status = (enum status_code)twihs_master_setup(MAXTOUCH_TWI_INTERFACE, &twi_opt);
	Assert(status == STATUS_OK);

	/* Initialize the maXTouch device */
	status = mxt_init_device(device, MAXTOUCH_TWI_INTERFACE,
			MAXTOUCH_TWI_ADDRESS, MAXTOUCH_XPRO_CHG_PIO);
	Assert(status == STATUS_OK);

	/* Issue soft reset of maXTouch device by writing a non-zero value to
	 * the reset register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_COMMANDPROCESSOR_T6, 0)
			+ MXT_GEN_COMMANDPROCESSOR_RESET, 0x01);

	/* Wait for the reset of the device to complete */
	delay_ms(MXT_RESET_TIME);

	/* Write data to configuration registers in T7 configuration object */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 0, 0x20);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 1, 0x10);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 2, 0x4b);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 3, 0x84);

	/* Write predefined configuration data to configuration objects */
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_GEN_ACQUISITIONCONFIG_T8, 0), &t8_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_TOUCH_MULTITOUCHSCREEN_T9, 0), &t9_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_SPT_CTE_CONFIGURATION_T46, 0), &t46_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_PROCI_SHIELDLESS_T56, 0), &t56_object);

	/* Issue recalibration command to maXTouch device by writing a non-zero
	 * value to the calibrate register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_COMMANDPROCESSOR_T6, 0)
			+ MXT_GEN_COMMANDPROCESSOR_CALIBRATE, 0x01);
}
void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(RTC, YEAR, MONTH, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC,  RTC_IER_ALREN);

}
void clear_LCD(int a, int b){
	if (locked){
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
		ili9488_draw_filled_rectangle(0, a, ILI9488_LCD_WIDTH-1, b);
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	}
	else{
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
		ili9488_draw_filled_rectangle(0, a, ILI9488_LCD_WIDTH-1, b);
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
	}
}
void draw_screen(void) {
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
}

void draw_button(uint32_t clicked) {
	static uint32_t last_state = 255; // undefined
	uint8_t stingLCD[256];

	if(clicked == last_state) return;
	
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
	//ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2, BUTTON_Y-BUTTON_H/2, BUTTON_X+BUTTON_W/2, BUTTON_Y+BUTTON_H/2);
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
	ili9488_draw_filled_circle(275, 50,25);
	
	if(selection){
		clear_LCD(0,450);
		last_state = 1;
		if (locked){ //Pinta a tela de preto para indicar que esta travado
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
			ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH, ILI9488_LCD_HEIGHT);
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
			ili9488_draw_filled_circle(275, 50,25);
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
		}
		else{
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
			ili9488_draw_filled_rectangle(ARROW_X,ARROW_Y,ARROW_W,ARROW_Y-ARROW_H);

			ili9488_draw_filled_rectangle(ILI9488_LCD_WIDTH,ARROW_Y,ILI9488_LCD_WIDTH-ARROW_W,ARROW_Y-ARROW_H);
			ili9488_draw_filled_rectangle(100,325,ILI9488_LCD_WIDTH-100,375);
			
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
			ili9488_draw_filled_circle(275, 50,25);
		}
			
	}
	else{
		if (locked){ //Pinta a tela de preto para indicar que esta travado
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
			ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH, ILI9488_LCD_HEIGHT);
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
			ili9488_draw_filled_circle(275, 50,25);
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
		}
		else {		
			if (clicked==3){ //Voltando do estado de Travado
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
				ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH, ILI9488_LCD_HEIGHT);
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
				ili9488_draw_filled_circle(275, 50,25);
				//ili9488_draw_pixmap(275, 50, 50, 50, image_data__ionicons_svg_mdlock);
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
				ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2, BUTTON_Y-BUTTON_H/2, BUTTON_X+BUTTON_W/2, BUTTON_Y+BUTTON_H/2);
			
				if(last_state==1) {
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
					ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y+BUTTON_H/2-BUTTON_BORDER);
				}
				else if (last_state==0){
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
					ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y-BUTTON_H/2+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y-BUTTON_BORDER);
				}
			}
		
			else if(clicked==1) {
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
				ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y+BUTTON_H/2-BUTTON_BORDER);
		
				RTC_init();
				start = true;
				last_state = clicked;
			} 
		
			else if(clicked == 0) {
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
				ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y-BUTTON_H/2+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y-BUTTON_BORDER);
				start = false;
				last_state = clicked;
			}
		}
	}
}

uint32_t convert_axis_system_x(uint32_t touch_y) {
	// entrada: 4096 - 0 (sistema de coordenadas atual)
	// saida: 0 - 320
	return ILI9488_LCD_WIDTH - ILI9488_LCD_WIDTH*touch_y/4096;
}

uint32_t convert_axis_system_y(uint32_t touch_x) {
	// entrada: 0 - 4096 (sistema de coordenadas atual)
	// saida: 0 - 320
	return ILI9488_LCD_HEIGHT*touch_x/4096;
}

volatile int c =0;
volatile int clkd;

void touch_handler(uint32_t tx, uint32_t ty) {
	if(locked==false){ 
		if(selection==true){
			if(tx >=100 && tx <= ILI9488_LCD_WIDTH-100 && ty >= 325 && ty <= 375) {
				selection=false;	
				draw_button(3);
			}
		}
		else{
			// Botoes de liga desliga lavagem
			if(tx >= BUTTON_X-BUTTON_W/2 && tx <= BUTTON_X + BUTTON_W/2) {
				if(ty >= BUTTON_Y-BUTTON_H/2 && ty <= BUTTON_Y) {
					clkd =1;
				} 
				else if(ty > BUTTON_Y && ty < BUTTON_Y + BUTTON_H/2) {
					clkd =0;
				}
				draw_button(clkd);
			}
		}
	}
	
	if(tx >=275 && tx <= 315 && ty >= 25 && ty <= 50) { // Botao do Lock
		c++;
		if(c%2!=0){
			if (locked){
				locked=false;
			}
			else{
				locked=true;
			}
			draw_button(3);
		}
	}
}

void mxt_handler(struct mxt_device *device)
{
	/* USART tx buffer initialized to 0 */
	char tx_buf[STRING_LENGTH * MAX_ENTRIES] = {0};
	uint8_t i = 0; /* Iterator */

	/* Temporary touch event data struct */
	struct mxt_touch_event touch_event;

	/* Collect touch events and put the data in a string,
	 * maximum 2 events at the time */
	do {
		/* Temporary buffer for each new touch event line */
		char buf[STRING_LENGTH];
	
		/* Read next next touch event in the queue, discard if read fails */
		if (mxt_read_touch_event(device, &touch_event) != STATUS_OK) {
			continue;
		}
		
		 // eixos trocados (quando na vertical LCD)
		uint32_t conv_x = convert_axis_system_x(touch_event.y);
		uint32_t conv_y = convert_axis_system_y(touch_event.x);
		
		/* Format a new entry in the data string that will be sent over USART */
		sprintf(buf, "Nr: %1d, X:%4d, Y:%4d, Status:0x%2x conv X:%3d Y:%3d\n\r",
				touch_event.id, touch_event.x, touch_event.y,
				touch_event.status, conv_x, conv_y);
		touch_handler(conv_x, conv_y);

		/* Add the new string to the string buffer */
		strcat(tx_buf, buf);
		i++;

		/* Check if there is still messages in the queue and
		 * if we have reached the maximum numbers of events */
	} while ((mxt_is_message_pending(device)) & (i < MAX_ENTRIES));

	/* If there is any entries in the buffer, send them over USART */
	if (i > 0) {
		usart_serial_write_packet(USART_SERIAL_EXAMPLE, (uint8_t *)tx_buf, strlen(tx_buf));
	}
}

void crono(int tempo){
	clear_LCD(390,415);
	t_ciclo *p_primeiro = initMenuOrder();
	if (start==true){
		uint8_t stingLCD[256];
		
		rtc_get_time(RTC, &hour, &minu, &seg);
		sprintf(stingLCD, "%d :%d de %d min.",minu,seg,tempo);
		ili9488_draw_string(40, 400, stingLCD);
				
		if(minu == tempo){
			sprintf(stingLCD, "ACABOU A LAVAGEM SEU CORNO");
			ili9488_draw_string(5, 455, stingLCD);
			delay_s(1);
			start=false;
			selection=true;
			draw_button(3);
		}
	}
	else{return;}
}
 
int main(void){
	struct mxt_device device; /* Device data container */

	/* Initialize the USART configuration struct */
	const usart_serial_options_t usart_serial_options = {
		.baudrate     = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength   = USART_SERIAL_CHAR_LENGTH,
		.paritytype   = USART_SERIAL_PARITY,
		.stopbits     = USART_SERIAL_STOP_BIT
	};

	sysclk_init(); /* Initialize system clocks */
	board_init();  /* Initialize board */
	configure_lcd();
	draw_screen();
	draw_button(0);
	/* Initialize the mXT touch device */
	mxt_init(&device);
    t_ciclo *p_primeiro = initMenuOrder();

	/* Initialize stdio on USART */
	stdio_serial_init(USART_SERIAL_EXAMPLE, &usart_serial_options);

	printf("\n\rmaXTouch data USART transmitter\n\r");
	
	f_rtt_alarme = true;

	while (true) {
		if(f_rtt_alarme){
			uint16_t pllPreScale = (int) (((float) 32768) / 4);//delay de 1 s
			uint32_t irqRTTvalue  = 4;
			
			// reinicia RTT para gerar um novo IRQ
			RTT_init(pllPreScale, irqRTTvalue);
			
			f_rtt_alarme = false;
			/* Check for any pending messages and run message handler if any
			* message is found in the queue */
			if (mxt_is_message_pending(&device)) {
				mxt_handler(&device);
			}
			crono(1);	
		}
		//pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
	return 0;
}
