#include "lcd.h"

#define WAIT_FOREVER -1

//Definindo constantes para comandos
static const uint8_t CMD_CLEAR_DISPLAY = 0x01;
static const uint8_t CMD_FUNCTION_SET = 0x28; //DL = 0 (4 bits), N = 1 (2 linhas), F = 0 (5x8 dots)
static const uint8_t CMD_DISPLAY_ON = 0x0C;
static const uint8_t CMD_DISPLAY_OFF = 0x08;
static const uint8_t CMD_ENTRY_MODE_SET = 0x06; //
static const uint8_t CMD_CURSOR_BLINK = 0x0F; //Cursor visivel e piscando
static const uint8_t CMD_CURSOR_NO_BLINK = 0x0C; //Cursor visivel e sem piscar
static const uint8_t CMD_CURSOR_TO_LEFT = 0x10;
static const uint8_t CMD_CURSOR_TO_RIGHT = 0x14;
static const uint8_t CMD_SHIFT_LEFT = 0x18;
static const uint8_t CMD_SHIFT_RIGHT = 0x1C;

/*
PO -> RS (Register Select) 0 = instrucao, 1 = dado
P1 -> RW (Read/Write) 0 = escrita, 1 = leitura
P2 -> En (Enable)
P3 -> BL (Backlight)
P4 -> DB4
P5 -> DB5
P6 -> DB6
P7 -> DB7
*/

static void send_command(i2c_master_dev_handle_t i2c_dev_handle ,uint8_t command) {
    uint8_t data_upper, data_lower; 
    uint8_t data_transmit[4]; //Array para armazenar os dados a serem transmitidos
    data_upper = (command & 0xF0); //Pegando 4 bits menos significativos
    data_lower = ((command << 4) & 0xF0); //Pegando 4 bits mais significativos
    //cmd HHHHLLLL separar para data_h = HHHH0000 e data_l = LLLL0000
	//comandos enviados dado 8 bits, separar em 2 partes
	//os 4 ultimos bits de cada byte são RS,RW,EN,BL

    data_transmit[0] = data_upper | 0x0C; // BL = 1, EN = 1, RW = 0(escrita), RS = 0(comando)
    data_transmit[1] = data_upper | 0x08; // BL = 1, EN = 0, RW = 0(escrita), RS = 0(comando)
    data_transmit[2] = data_lower | 0x0C;
    data_transmit[3] = data_lower | 0x08;

    i2c_master_transmit(i2c_dev_handle, data_transmit, sizeof(data_transmit), WAIT_FOREVER);
}

static void send_data(i2c_master_dev_handle_t i2c_dev_handle ,uint8_t data) {
    uint8_t data_upper, data_lower; 
    uint8_t data_transmit[4]; //Array para armazenar os dados a serem transmitidos
    data_upper = (data & 0xF0); //Pegando 4 bits menos significativos
    data_lower = ((data << 4) & 0xF0); 
    
    data_transmit[0] = data_upper | 0x0D; //BL = 1 En = 1 RW = 0 RS = 1 (dados)
	data_transmit[1] = data_upper | 0x09; //BL = 1 En = 0 RW = 0 RS = 1 (dados)
	data_transmit[2] = data_lower | 0x0D;
	data_transmit[3] = data_lower | 0x09;

    i2c_master_transmit(i2c_dev_handle, data_transmit, sizeof(data_transmit), WAIT_FOREVER);
}

void lcd_init(i2c_master_dev_handle_t i2c_dev_handle) {
    vTaskDelay(pdMS_TO_TICKS(50));
    //Para sincronizar enviar 0x30 3 vezes com intervalos
    send_command(i2c_dev_handle, 0x30);
    vTaskDelay(pdMS_TO_TICKS(5));
    send_command(i2c_dev_handle, 0x30);
    vTaskDelay(pdMS_TO_TICKS(5));
    send_command(i2c_dev_handle, 0x30);
    vTaskDelay(pdMS_TO_TICKS(5));
    //Configurar com modo 4 bits
    send_command(i2c_dev_handle, 0x20);
    vTaskDelay(pdMS_TO_TICKS(10));
    //Inicialização
    send_command(i2c_dev_handle, CMD_FUNCTION_SET);
    vTaskDelay(pdMS_TO_TICKS(1));
    send_command(i2c_dev_handle, CMD_DISPLAY_OFF);
    vTaskDelay(pdMS_TO_TICKS(1));
    send_command(i2c_dev_handle, CMD_CLEAR_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(2));
    send_command(i2c_dev_handle, CMD_DISPLAY_ON);
    vTaskDelay(pdMS_TO_TICKS(1));
    send_command(i2c_dev_handle, CMD_ENTRY_MODE_SET);
}

void lcd_send_string(i2c_master_dev_handle_t i2c_dev_handle, char *str) {
    //checa se o valor apontado é o caracter nulo \0 que indica o fim da string
    //se não for envia o caracter para o lcd
    while(*str) {
        //envia caracter para lcd e incrementa o ponteiro para a próxima posição
        send_data(i2c_dev_handle, *str++);
    }
}

void lcd_clear(i2c_master_dev_handle_t i2c_dev_handle) {
    send_command(i2c_dev_handle, CMD_CLEAR_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_shift_right(i2c_master_dev_handle_t i2c_dev_handle) {
    send_command(i2c_dev_handle, CMD_SHIFT_RIGHT);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_shift_left(i2c_master_dev_handle_t i2c_dev_handle) {
    send_command(i2c_dev_handle, CMD_SHIFT_LEFT);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_cursor_to_left(i2c_master_dev_handle_t i2c_dev_handle) {
    send_command(i2c_dev_handle, CMD_CURSOR_TO_LEFT);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_cursor_to_right(i2c_master_dev_handle_t i2c_dev_handle) {
    send_command(i2c_dev_handle, CMD_CURSOR_TO_RIGHT);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_cursor_blink(i2c_master_dev_handle_t i2c_dev_handle, bool blink) {
    if(blink) {
        send_command(i2c_dev_handle, CMD_CURSOR_BLINK);
    } else {
        send_command(i2c_dev_handle, CMD_CURSOR_NO_BLINK);
    }
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_cursor(i2c_master_dev_handle_t i2c_dev_handle, uint8_t row, uint8_t col) {
    //Calcula valor de enrdereço memoria DDRAM
	//segundo data sheet 0x0 começo primeira linha e 0x40 segunda linha
	//entao se linha = 1 vai ficar 0 no calculo e vai ser 0x0
	//0x80 segundo datasheet o comando set DDRAM Address começa com 1 no bit mais significativo
    //001X XXXX sendo X valor do endereço
    uint8_t addr = 0x80 | (col + row * 0x40);
    send_command(i2c_dev_handle, addr);
    vTaskDelay(pdMS_TO_TICKS(2));
}