#include <Arduino.h>
#include <SPI.h>

const int CS_PIN = 5;
const int MOSI_PIN = 23;
const int MISO_PIN = 19;
const int CLK_PIN = 18;

struct Channels
{
  float channel_0;
  float channel_1;
  float channel_2;
  float channel_3;
};

Channels channels;

int readADC(uint16_t channel)
{
  int rawValue2;
  byte control = 0b00000110;
  channel <<= 14;

  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(1);
  SPI.transfer(control);
  rawValue2 = SPI.transfer16(channel);
  digitalWrite(CS_PIN, HIGH);
  return rawValue2;
}

void setup()
{
  Serial.begin(115200);
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
}

void loop()
{
  for (uint16_t channel = 0; channel < 4; ++channel)
  {
    int rawValue = readADC(channel);
    float tension = ((float)rawValue/4095.0)*3.3;
    Serial.print("Canale ");
    Serial.print(channel);
    Serial.print(": Lettura grezza = ");
    Serial.print(rawValue);
    Serial.print(", Tensione = ");
    Serial.print(" V");
    Serial.println(tension,4);
  }
  delay(50); // campionamento 20Hz
}
