#include <Arduino.h>
#include <MP.h>

constexpr int8_t MSG_ID = 1;

void setup()
{
  if (MP.begin() < 0) while (true);
}

void loop()
{
  static uint32_t data = 0;
  MP.Send(MSG_ID, data++);
  delay(100);
}
