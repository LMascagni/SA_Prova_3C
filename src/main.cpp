#include <Arduino.h>
#include <SPI.h>

#define N_CHANNELS 4

const int CS_PIN = 5;
const int MOSI_PIN = 23;
const int MISO_PIN = 19;
const int CLK_PIN = 18;


float v[N_CHANNELS];

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
  SPI.beginTransaction(SPISettings(1500000, MSBFIRST, SPI_MODE0));

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
}

void loop()
{
  for (uint16_t channel = 0; channel < N_CHANNELS; ++channel)
  {
    int rawValue = readADC(channel);
    v[channel] = ((float)rawValue/4095.0)*3.3;
  }
  for(int channel=0; channel<N_CHANNELS; channel++){
    Serial.println("ch" + String(channel) + ": " + String(v[channel]));
  }
   
  delayMicroseconds(100); // campionamento 20Hz
}
