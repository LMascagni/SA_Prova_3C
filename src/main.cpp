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

static const uint8_t msgQueueLen = 5 * N_CHANNELS; //lunghezza * numero di canali
static QueueHandle_t msg_queue;

//per ora inutile
int frequency = 8000; // Hz

hw_timer_t *timer0 = NULL;

volatile float v[N_CHANNELS];

void IRAM_ATTR readADC();

void printTask();

void setup()
{
  // inizializzazione comunicazione seriale
  Serial.begin(115200);
  Serial.println("SA_PROVA_03C (v2.0)");

  // setup comunicazione SPI
  SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

  // setup timer
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, readADC, true);
  timerAlarmWrite(timer0, 125, true);
  timerAlarmEnable(timer0);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  msg_queue = xQueueCreate(msgQueueLen, sizeof(int32_t));
  
  xTaskCreatePinnedToCore(
      printTask,
      "Print Task",
      1024,
      NULL,
      1,
      NULL,
      APP_CPU_NUM
  );
}

void loop()
{
  // stampa dei valori su teleplot
  // for (int channel = 0; channel < N_CHANNELS; channel++)
  // {
  //   Serial.println(">ch" + String(channel) + ": " + String(v[channel])); // formato di stringa per teleplot
  // }
}

void IRAM_ATTR readADC()
{
  for (uint16_t ch = 0; ch < N_CHANNELS; ch++)
  {
    uint16_t rawValue;
    byte control = 0b00000110;
    uint16_t channel = ch << 14;

    digitalWrite(CS_PIN, LOW);

    SPI.transfer(control);
    rawValue = SPI.transfer16(channel);

    digitalWrite(CS_PIN, HIGH);

    if(xQueueSend(msg_queue, (void *)&rawValue, 10) != pdTRUE){
      Serial.println("coda piena");
    }

    //controllo del bit 13 di rawValue
    //if (rawValue & (1 << 12))
    //{
      // conversione
     // v[ch] = ((float)rawValue / 4095.0) * 3.3;
   // }
  }
}

void printTask(void *parameters)
{
  int32_t rawValue;
  
  while (1)
  {
    if (xQueueReceive(msg_queue, (void *)&rawValue, 0) == pdTRUE)
    {
      Serial.println(rawValue);
    }
    else
    {
      Serial.println("attesa dati...");
    }
    
  }
}