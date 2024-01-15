#include <Arduino.h>
#include <SPI.h>

struct main
{
  float channel_1;
  float channel_2;
  float channel_3;
  float channel_4;
};

const int CS_PIN = 15;
const int MOSI_PIN = 13;
const int MISO_PIN = 12;
const int CLK_PIN = 14;

int readADC(int channel) {
  // MCP3204 formato: 0b0000XXX0, dove XXX è il numero del canale
  byte command = B11000000 | (channel << 4);

  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(1); 
  int rawValue = SPI.transfer(command) & 0x0F;
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);
  digitalWrite(CS_PIN, HIGH);

  return rawValue;
}

void setup() {
  Serial.begin(115200);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); 

  SPI.begin();
}

void loop() {
  for (int channel = 0; channel < 4; ++channel) {
    int rawValue = readADC(channel);
    float tension = map(rawValue, 0, 4095, 0, 5);

    Serial.print("Canale ");
    Serial.print(channel);
    Serial.print(": Lettura grezza = ");
    Serial.print(rawValue);
    Serial.print(", Tensione = ");
    Serial.print(" V");
    Serial.println(tension);

    delay(1000);  //campionamento ogni secondo
  }
}
