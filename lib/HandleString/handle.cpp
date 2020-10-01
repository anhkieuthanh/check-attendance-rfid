#include "handle.h"

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
  static char data[25];
  memset(data, 0, 25);
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
const char *dataCombineReg(const char *id, const char *stdCode, const char *phone)
{
  static char data2[70];
  memset(data2, 0, 70);
  DynamicJsonDocument doc(70);
  doc["id"] = id;
  doc["stdCode"] = stdCode;
  doc["phone"] = phone;
  serializeJson(doc, data2);
  return data2;
}