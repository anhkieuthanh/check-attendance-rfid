#include "handle.h"
char data[50];
char data2[100];
char *string2char(String command)
{
  if (command.length() != 0)
  {
    char *p = const_cast<char *>(command.c_str());
    return p;
  }
}
const char *dataCombine(const char *uid)
{
  memset(data,0,50);
  DynamicJsonDocument doc(200);
  // Add values in the document
  doc["id"] = uid;
  // Generate the minified JSON and send it to the Serial port.
  serializeJson(doc, data);
  // The above line prints:
  Serial.println("test code: ");
  Serial.print(data);
  return data;
}
const char * dataCombineReg(const char* id,const char* stdCode,const char* phone){
  memset(data2,0,100);
  DynamicJsonDocument doc(200);
  doc["id"] = id;
  doc["stdCode"] = stdCode;
  doc["phone"] = phone;
  serializeJson(doc,data2);
  return data2;
}