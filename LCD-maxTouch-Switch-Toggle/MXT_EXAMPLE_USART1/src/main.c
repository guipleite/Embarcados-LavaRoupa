/**
 * 5 semestre - Eng. da Computa??o - Insper
  ** Entrega realizada por:
  ** 
  **  - Guilherme Leite - guilhermepl3@al.insper.edu.br
  **
  **/

#include <asf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "conf_board.h"
#include "conf_example.h"
#include "conf_uart_serial.h"

#include "maquina1.h"

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

// defines da Porta
#define DOOR_PIO           PIOC
#define DOOR_PIO_ID        ID_PIOC
#define DOOR_PIO_IDX       17u
#define DOOR_PIO_IDX_MASK  (1u << DOOR_PIO_IDX)

// Configurando Led
#define LED_PIO           PIOC                  // periferico que controla o LED
#define LED_PIO_ID        12                    // ID do perif?rico PIOC (controla LED)
#define LED_PIO_IDX       8u                    // ID do LED no PIO
#define LED_PIO_IDX_MASK  (1u << LED_PIO_IDX)   // Mascara para CONTROLARMOS o LED

// LCD struct
struct ili9488_opt_t g_ili9488_display_opt;

// Variaveis boobleanas para indicar os estados
volatile Bool f_rtt_alarme = false;

 Bool start = false;
 Bool locked = false;
 Bool selection = true;

int c ;
int c2;
int clkd;

int total_time;

uint32_t last_state = 255; // undefineds

/**
 * Inicializa ordem do menu
 * retorna o primeiro ciclo que
 * deve ser exibido.
 */
t_ciclo *cicle ;

t_ciclo *initMenuOrder(){
	c_rapido.previous = &c_centrifuga;
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

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);

void RTT_Handler(void){
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

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

void display_cicle(void){
	
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	ili9488_draw_filled_rectangle(ARROW_X+ARROW_W, BOX_Y, ILI9488_LCD_WIDTH-ARROW_W, BOX_H);
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
	uint8_t stingLCD[256];
	
	ili9488_draw_pixmap(ICON_X,ICON_Y-50,100,100,cicle->image->data);
	
	sprintf(stingLCD,"Tempo enxague %d\nQuant. enxagues %d\nRPM centrifug.%d\nTempo centrifug.%d\nModo Pesado %d\nBolhas %d",
			cicle->enxagueTempo,
			cicle->enxagueQnt,
			cicle->centrifugacaoRPM,
			cicle->centrifugacaoTempo,
			cicle->heavy,
			cicle->bubblesOn
			);
	ili9488_draw_string(ICON_X-50, ICON_Y+50, stingLCD);
	
	
	ili9488_draw_string(LABEL_X, LABEL_Y,  cicle->nome);
}

void select_screen(void){

	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
		
	display_cicle();

	ili9488_draw_pixmap(ARROW_X,ARROW_Y-ARROW_H,back.width,back.height,back.data);

	ili9488_draw_pixmap(ILI9488_LCD_WIDTH-ARROW_W,ARROW_Y-ARROW_H,next.width,next.height,next.data);
	ili9488_draw_pixmap(START_X,START_Y,comeca.width,comeca.height,comeca.data);
		
}

void draw_button(uint32_t clicked) {
	
	uint8_t stingLCD[256];

	if(clicked == last_state) return;
	
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
	
	if(selection==false){
		if (locked){ //Pinta a tela de preto para indicar que esta travado
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
			ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH, ILI9488_LCD_HEIGHT);
			
			ili9488_draw_pixmap(LOCK_X,LOCK_Y,_ionicons_svg_mdunlock.width,_ionicons_svg_mdunlock.height,_ionicons_svg_mdunlock.data);

			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
			ili9488_draw_string(LABEL_X, LABEL_Y+10,  cicle->nome);

		}
		else {	
			ili9488_draw_pixmap(BACK_X,BACK_Y,leftarrow.width,leftarrow.height,leftarrow.data);
			ili9488_draw_string(LABEL_X, LABEL_Y+10,  cicle->nome);

			if (clicked==3){ //Voltando de um estado 
				ili9488_draw_pixmap(BACK_X,BACK_Y,leftarrow.width,leftarrow.height,leftarrow.data);;

				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
				ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH, ILI9488_LCD_HEIGHT);
	
				ili9488_draw_pixmap(LOCK_X,LOCK_Y,_ionicons_svg_mdlock.width,_ionicons_svg_mdlock.height,_ionicons_svg_mdlock.data);

				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
				ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2, BUTTON_Y-BUTTON_H/2, BUTTON_X+BUTTON_W/2, BUTTON_Y+BUTTON_H/2);
				ili9488_draw_string(LABEL_X, LABEL_Y+10,  cicle->nome);

				
				if(last_state==1) {
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
					ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y+BUTTON_H/2-BUTTON_BORDER);
				}
				else if (last_state==0){
					ili9488_draw_pixmap(BACK_X,BACK_Y,leftarrow.width,leftarrow.height,leftarrow.data);
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
					ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y-BUTTON_H/2+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y-BUTTON_BORDER);
				}
			}
		
			else if(clicked==1) {
				if(pio_get(DOOR_PIO,PIO_INPUT ,DOOR_PIO_IDX_MASK)==0){
					
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
					ili9488_draw_filled_rectangle(DOOR_X,DOOR_Y,(ILI9488_LCD_WIDTH/2)-25+a60e3f50683d3063c36bb8f4f08e9ad0.width,29+a60e3f50683d3063c36bb8f4f08e9ad0.height);
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
					ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2, BUTTON_Y-BUTTON_H/2, BUTTON_X+BUTTON_W/2, BUTTON_Y+BUTTON_H/2);
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
					ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2+BUTTON_BORDER, BUTTON_Y+BUTTON_BORDER, BUTTON_X+BUTTON_W/2-BUTTON_BORDER, BUTTON_Y+BUTTON_H/2-BUTTON_BORDER);
		
					RTC_init();
					start = true;
					last_state = clicked;
					ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
					ili9488_draw_filled_rectangle(BACK_X,BACK_Y,BACK_X+leftarrow.width,BACK_Y+leftarrow.height);
				}
				else{
					ili9488_draw_pixmap(DOOR_X,DOOR_Y,a60e3f50683d3063c36bb8f4f08e9ad0.width,a60e3f50683d3063c36bb8f4f08e9ad0.height,a60e3f50683d3063c36bb8f4f08e9ad0.data);
				}
			} 
		
			else if(clicked == 0) {
				ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
				ili9488_draw_filled_rectangle(BUTTON_X-BUTTON_W/2, BUTTON_Y-BUTTON_H/2, BUTTON_X+BUTTON_W/2, BUTTON_Y+BUTTON_H/2);
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

void start_wash(int sig){
	if (sig==1){
		pio_clear(LED_PIO,LED_PIO_IDX_MASK); // Aciona as interfaces da maquina
	}
	else{
		pio_set(LED_PIO,LED_PIO_IDX_MASK);
	}
}

void touch_handler(uint32_t tx, uint32_t ty) {
	if(selection==true){
		if(tx >=START_X && tx <= ILI9488_LCD_WIDTH-START_X && ty >= START_Y && ty <= START_Y+comeca.height) {
			selection=false;
			draw_button(3);
		}
		if(ty <= ARROW_Y && ty >= ARROW_Y-ARROW_H){
			if(tx >=ARROW_X && tx <= ARROW_X+ARROW_W  ){
				c2++;
				if(c2%2!=0){
					cicle = cicle->next;
				}
			}
			if(tx <=ILI9488_LCD_WIDTH && tx >= ILI9488_LCD_WIDTH-ARROW_W ){
				c2++;
				if(c2%2!=0){
					cicle = cicle->previous;
				}	
			}
			display_cicle();
		}
	}
	if(selection==false){
		if(locked==false){ 
		
			// Botoes de liga desliga lavagem
			if(tx >= BUTTON_X-BUTTON_W/2 && tx <= BUTTON_X + BUTTON_W/2) {
				if(ty >= BUTTON_Y-BUTTON_H/2 && ty <= BUTTON_Y) {
					clkd =1;
				} 
				else if(ty > BUTTON_Y && ty < BUTTON_Y + BUTTON_H/2) {
					clkd =0;
				}
				start_wash(clkd);
				pio_clear(LED_PIO,LED_PIO_IDX_MASK);
				draw_button(clkd);
			}
			//Botao de voltar
			
			if(tx>=BACK_X && ty >= BACK_Y && tx <= BACK_X+leftarrow.width && ty <=BACK_Y+leftarrow.height && (start==false)){
				selection=true;
				start_wash(0);
				select_screen();
			}
		}
	
		if(tx >=LOCK_X && tx <= LOCK_X+_ionicons_svg_mdunlock.width && ty >= LOCK_Y && ty <= LOCK_Y+_ionicons_svg_mdunlock.height) { // Botao do Lock
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
	clear_LCD(TIME_Y0,TIME_Y1);

	if (start==true){
		uint8_t stingLCD[256];
		
		rtc_get_time(RTC, &hour, &minu, &seg);
		sprintf(stingLCD, "%d :%d de %d min.",minu,seg,tempo);
		ili9488_draw_string(CLOCK_X, CLOCK_Y, stingLCD);
				
		if(minu == tempo){
			sprintf(stingLCD, "ACABOU A LAVAGEM");
			ili9488_draw_string(WARN_X, WARN_Y, stingLCD);
			delay_s(1);
			start=false;
			selection=true;
			start_wash(0);
			last_state = 0;
			select_screen();
		}
	}
	else{return;}
}
int get_time(){
	total_time = ((cicle->enxagueTempo)*(cicle->enxagueQnt))+cicle->centrifugacaoTempo;
	
	if (cicle->heavy){
		total_time*=1.2;
	}
	return total_time;
}

void init (){
	sysclk_init(); /* Initialize system clocks */
	pmc_enable_periph_clk(DOOR_PIO_ID);
	// configura pino da porta como entrada com um pull-up.
	pio_set_input(DOOR_PIO_ID,DOOR_PIO_IDX_MASK,PIO_DEFAULT);
	// ativa pullup
	pio_pull_up(DOOR_PIO_ID, DOOR_PIO_IDX_MASK, 1);
	
	// Ativa o PIO na qual o LED foi conectado
	// para que possamos controlar o LED.
	pmc_enable_periph_clk(LED_PIO_ID);
	//Inicializa PC8 como sa?da
	pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 1, 0, 0);
    pio_clear(LED_PIO,LED_PIO_IDX_MASK);
	
	board_init();  /* Initialize board */
	configure_lcd();
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
	init();

	mxt_init(&device);	/* Initialize the mXT touch device */
	
	/* Initialize stdio on USART */
	stdio_serial_init(USART_SERIAL_EXAMPLE, &usart_serial_options);

	printf("\n\rmaXTouch data USART transmitter\n\r");
		
	cicle = initMenuOrder();
		
	select_screen();
	
	f_rtt_alarme = true;

	display_cicle();

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
			int tm = get_time(cicle);

			crono(tm);
		}
	}
	return 0;
}