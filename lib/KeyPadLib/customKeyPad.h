#include <Keypad_I2C.h>
#include "display_lcd.h"
#define I2CADDR 0x27
#define KEYPADROW 3
#define KEYPADCOL 4
extern uint8_t stateData;
extern uint8_t stateInput;
extern Keypad_I2C customKeypad;
extern LiquidCrystal_I2C lcd;

// <length> to check corect length of inputdata
// <mode> 0: number and character 1: only number 2:only character
void readKeyPad(uint8_t length);