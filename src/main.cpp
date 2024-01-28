/*
  Nome del Programma: SA_Prova_3C
  Autori: -Bianchin Francesco
          -Fontana Matteo
          -Gratton Claudio
          -Mascagni Lorenzo
          -Zorat Fabio
  Scopo del Programma:
  La ESP32 si collega in SPI con un ADC a 4 canali MCP3204, che ha risoluzione di 12bit.
  Il programma acquisisce le tensioni dei 4 canali (ciascuno condizionato nell'intervallo da 0 a 3.3V) con velocità di campionamento di 10 campionamenti al secondo.
  I dati vengono emessi sul terminale seriale per essere visualizzati con TelePlot in VS Code.
*/

#include <Arduino.h>
#include <SPI.h>

#define N_CHANNELS 4

const int CS_PIN = 5;
const int MOSI_PIN = 23;
const int MISO_PIN = 19;
const int CLK_PIN = 18;


//per ora inutile
int frequency = 8000; // Hz

hw_timer_t *timer0 = NULL;

volatile float v[N_CHANNELS];

void IRAM_ATTR readADC();

void setup()
{
  // inizializzazione comunicazione seriale
  Serial.begin(115200);

  // setup comunicazione SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(1500000, MSBFIRST, SPI_MODE0));

  // setup timer
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, readADC, true);
  timerAlarmWrite(timer0, 1000000, true);
  timerAlarmEnable(timer0);

  // setup pin CS (GPIO_5)
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
}

void loop()
{
  Serial.println(millis());

  // stampa dei valori su teleplot
  for (int channel = 0; channel < N_CHANNELS; channel++)
  {
    Serial.println(">ch" + String(channel) + ": " + String(v[channel])); // formato di stringa per teleplot
  }
}

void IRAM_ATTR readADC()
{
  for (uint16_t ch = 0; ch < N_CHANNELS; ch++)
  {
    uint16_t rawValue;
    byte control = 0b00000110;
    uint16_t channel = ch << 14;

    digitalWrite(CS_PIN, LOW);
    delayMicroseconds(1);

    SPI.transfer(control);
    rawValue = SPI.transfer16(channel);

    digitalWrite(CS_PIN, HIGH);

    //controllo del bit 13 di rawValue
    if (rawValue & (1 << 12))
    {
      // conversione
      v[ch] = ((float)rawValue / 4095.0) * 3.3;
    }
  }
}