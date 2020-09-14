#include "handle.h"

char *string2char(String command)
{
  if (command.length() != 0)
  {
    char *p = const_cast<char *>(command.c_str());
    return p;
  }
}
const char *dataCombine(const char *uid, const char *state)
{
  char data[50];
  //data[0] = '\0';
  DynamicJsonDocument doc(200);
  // Add values in the document
  doc["code"] = uid;
  doc["state"] = state;

  // Generate the minified JSON and send it to the Serial port.
  //
  serializeJson(doc, data);
  // The above line prints:
  // {"code":"uid","state":state}
  Serial.println("test code: ");
  Serial.print(data);
  Serial.println("---------------");
  return data;
}