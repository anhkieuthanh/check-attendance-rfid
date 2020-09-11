#define LCD_ROW 2
#define LCD_COLUMN 16
#define LCD_ADDRESS 0x3F

#include<LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

void scrollSingleLine(String str1, String str2, int *flag);
void oneLineBack(String text, int timeDelay);
void twoLineBack(String text1,String text2,int timeDelay);
void turnBackDefault();