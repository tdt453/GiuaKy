#include "main.h"
#include "keypad.h"

#define ROWS 4
#define COLS 4

static GPIO_TypeDef* rowPorts[ROWS] = {
    KEYPAD_ROW_1_GPIO_Port, KEYPAD_ROW_2_GPIO_Port,
    KEYPAD_ROW_3_GPIO_Port, KEYPAD_ROW_4_GPIO_Port
};
static uint16_t rowPins[ROWS] = {
    KEYPAD_ROW_1_Pin, KEYPAD_ROW_2_Pin, KEYPAD_ROW_3_Pin, KEYPAD_ROW_4_Pin
};

static GPIO_TypeDef* colPorts[COLS] = {
    KEYPAD_COL_1_GPIO_Port, KEYPAD_COL_2_GPIO_Port,
    KEYPAD_COL_3_GPIO_Port, KEYPAD_COL_4_GPIO_Port
};
static uint16_t colPins[COLS] = {
    KEYPAD_COL_1_Pin, KEYPAD_COL_2_Pin, KEYPAD_COL_3_Pin, KEYPAD_COL_4_Pin
};

static char keymap[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char Keypad_GetKey(void) {
    for (int r = 0; r < ROWS; r++) {
        /* Set current row LOW */
        HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_RESET);

        for (int c = 0; c < COLS; c++) {
            if (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_RESET) {
                HAL_Delay(20); // debounce
                if (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_RESET) {
                    while (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_RESET); // chờ thả phím
                    HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_SET);
                    return keymap[r][c];
                }
            }
        }

        /* Reset row back to HIGH */
        HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_SET);
    }
    return 0;
}
