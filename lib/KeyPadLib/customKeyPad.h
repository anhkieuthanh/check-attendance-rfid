#include <Keypad_I2C.h>
#include "display_lcd.h"
#define I2CADDR 0x20
#define KEYPADROW 4
#define KEYPADCOL 4

extern uint8_t stateData;
extern uint8_t stateInput;
extern int count;
extern Keypad_I2C customKeypad;
extern LiquidCrystal_I2C lcd;

// <length> to check corect length of inputdata
void readKeyPad(int length,String &result);