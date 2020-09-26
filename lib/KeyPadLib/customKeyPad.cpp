#include "customKeyPad.h"
int count = 0;
uint8_t stateData;
uint8_t stateInput;
void readKeyPad(uint8_t length)
{
    char key = customKeypad.getKey();
    if (key != NO_KEY)
    {
        switch (key)
        {
        case 'A':
            if (count != length)
            {
                oneLineFix("Wrong format");
                delay(2000);
                stateInput = 0;
                count = 0;
                break;
            }
            if (count == length)
            {
                oneLineFix("Confirm??");
                lcd.setCursor(0, 1);
                lcd.print("A: Yes");
                lcd.setCursor(7, 1);
                lcd.print("B: No");
                if (customKeypad.waitForKey() == 'A')
                {
                    stateData = 2;
                    count = 0;
                    break;
                }
                if (customKeypad.waitForKey() == 'B')
                {
                    stateInput = 0;
                    count = 0;
                    break;
                }
            }
            break;
        case 'B':
            count--;
            key = ' ';
            lcd.setCursor(count, 1);
            lcd.print(key);
            break;
        case 'C':
        if(customKeypad.getState()==HOLD){
            stateInput =3;
        }
            break;

        default:
            lcd.setCursor(count, 1);
            lcd.print(key);
            count++;
            break;
        }
    }
}