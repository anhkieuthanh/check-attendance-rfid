#include "customKeyPad.h"
int count = 0;
uint8_t stateData;
uint8_t stateInput;
void readKeyPad(int length, String &result)
{
    
    char key = customKeypad.waitForKey();
    char key2[2];
    key2[0] = key;
    if (key != NO_KEY)
    {
        switch (key)
        {
        case 'A':
            if (count != length)
            {
                oneLineFix("Wrong format");
                delay(1500);
                stateInput = 0;
                count = 0;
                result ="";
                break;
            }
            if (count == length)
            {
                Serial.println("In Function");
                Serial.println(result);
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
            if (count > 0)
            {
                count--;
                key = ' ';
                lcd.setCursor(count, 1);
                result.remove(count);
                lcd.print(key);
            }
            break;
        case 'C':
            if (customKeypad.getState() == HOLD)
            {
                stateInput = 3;
            }
            break;

        default:
            lcd.setCursor(count, 1);
            lcd.print(key);
            result += key2[0];
            Serial.println(result);
            count++;
            break;
        }
    }
}