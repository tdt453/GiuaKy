#include "lcd_i2c.h"

#define LCD_I2C_ADDR (0x27 << 1) // Dia chi I2C cua LCD

static I2C_HandleTypeDef *lcd_i2c;

static void LCD_SendCommand(uint8_t cmd);
static void LCD_SendData(uint8_t data);

void LCD_Init(I2C_HandleTypeDef *hi2c) {
    lcd_i2c = hi2c;
    HAL_Delay(50);

    LCD_SendCommand(0x33);
    LCD_SendCommand(0x32);
    LCD_SendCommand(0x28); // 4-bit, 2 dong
    LCD_SendCommand(0x0C); // Bat hien thi, tat con tro
    LCD_SendCommand(0x06); // Tu dong tang vi tri
    LCD_Clear();
}

void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? 0x80 : 0xC0;
    addr += col;
    LCD_SendCommand(addr);
}

void LCD_PrintString(char *str) {
    while (*str) {
        LCD_SendData((uint8_t)(*str));
        str++;
    }
}

// ========== Ham noi bo ==========
static void LCD_Write4Bits(uint8_t data) {
    HAL_I2C_Master_Transmit(lcd_i2c, LCD_I2C_ADDR, &data, 1, 10);
}

static void LCD_SendInternal(uint8_t data, uint8_t flags) {
    uint8_t high = data & 0xF0;
    uint8_t low  = (data << 4) & 0xF0;
    uint8_t buf[4];
    buf[0] = high | 0x0C | flags;
    buf[1] = high | 0x08 | flags;
    buf[2] = low  | 0x0C | flags;
    buf[3] = low  | 0x08 | flags;
    for (int i = 0; i < 4; i++) {
        LCD_Write4Bits(buf[i]);
    }
}

static void LCD_SendCommand(uint8_t cmd) {
    LCD_SendInternal(cmd, 0);
}

static void LCD_SendData(uint8_t data) {
    LCD_SendInternal(data, 1);
}
