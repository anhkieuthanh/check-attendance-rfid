#include "handle.h"
char data[50];
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
  data[0] = '/0';
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