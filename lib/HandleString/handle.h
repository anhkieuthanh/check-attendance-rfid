#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>

char *string2char(String command);
const char *dataCombine(const char *uid);
const char * dataCombineReg(const char* uid,const char* stdCode, const char* phone);