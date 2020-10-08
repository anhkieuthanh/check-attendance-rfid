#include "display_lcd.h"
int stringStart = 0, stringStop = 0;
int scrollCursor = 16;
void oneLineBack(String text, int timeDelay)
{
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Mandevices Lab");
  lcd.setCursor(1, 1);
  lcd.print(text);
  delay(timeDelay);
  turnBackDefault();
}
void twoLineBack(String text1, String text2, int timeDelay)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text1);
  lcd.setCursor(0, 1);
  lcd.print(text2);
  delay(timeDelay);
  turnBackDefault();
}
void turnBackDefault()
{
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Mandevices Lab");
}
void scrollSingleLine(String fixedString, String scrolledString, int *flag)
{

  lcd.setCursor(4, 0);
  lcd.print(fixedString);
  lcd.setCursor(scrollCursor, 1);
  lcd.print(scrolledString.substring(stringStart, stringStop));
  delay(250);
  lcd.clear();
  if (stringStart == 0 && scrollCursor > 0)
  {
    scrollCursor--;
    if(stringStop != scrolledString.length())
    stringStop++;
  }
  else if (stringStart == stringStop)
  {
    stringStart = stringStop = 0;
    scrollCursor = 16;
  }
  else if (stringStop == scrolledString.length() && scrollCursor == 0)
  {
    stringStart++;
  }
  else
  {
    stringStart++;
    stringStop++;
  }
  if (stringStop == 0)
    *flag = 1;
}
void oneLineFix(String text)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  if (text.length() > 16)
    lcd.scrollDisplayRight();
  else
    lcd.print(text);
}
