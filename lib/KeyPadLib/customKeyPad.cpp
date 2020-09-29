#include "customKeyPad.h"
int count = 0;
uint8_t stateData;
uint8_t stateInput;
char bufferData[30];
void readKeyPad(int length, char *result)
{

    char key = customKeypad.waitForKey();
    if (key != NO_KEY)
    {
        switch (key)
        {
        case 'A':
            if (count != length)
            {
                oneLineFix("Wrong format");
                memset(bufferData,0,30);
                delay(1500);
                stateInput = 0;
                count = 0;
                break;
            }
            if (count == length)
            {
                strcpy(result,bufferData);
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
                memset(bufferData,0,30);
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
            bufferData[count] = key;
            count++;
            break;
        }
    }
}