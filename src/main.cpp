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
#include <UDT/data.h>

#define N_CHANNELS 4

const int CS_PIN = 5;
const int MOSI_PIN = 23;
const int MISO_PIN = 19;
const int CLK_PIN = 18;

static const uint8_t msgQueueLen = 100; // dimensione della coda di comunicazione tra ISR e stampa
static QueueHandle_t msg_queue;
volatile i16Data campione; // un campionamento dei 4 canali della ISR

int frequency = 8000; // in Hz, numero di campionamenti al secondo

hw_timer_t *timer0 = NULL;

void IRAM_ATTR readADC();

void printTask(void *parameters);

void setup()
{
  // inizializzazione comunicazione seriale
  Serial.begin(921600);
  Serial.println("SA_PROVA_03C (v2.0)");

  // setup comunicazione SPI
  SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  
  // impostazione della linea di chip select dell'ADC
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);


  // setup timer
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, readADC, true);
  timerAlarmWrite(timer0, int((1/frequency)*1000000), true);

  // creazione della coda per la comunicazione tra ISR e stampa dati
  msg_queue = xQueueCreate(msgQueueLen, sizeof(i16Data));

  // creazione ed avvio del task di stampa
  xTaskCreatePinnedToCore(
    printTask,
    "Print Task",
    2048,
    NULL,
    1,
    NULL,
    PRO_CPU_NUM
  );

  // avvio del timer per il triggering della ISR
  timerAlarmEnable(timer0);
}

void loop()
{
  // può rimanere vuoto
}

void IRAM_ATTR readADC()
{
  BaseType_t xHigherPriorityTaskWoken;

  uint16_t rawValue;
  uint16_t channel_shifted;
  uint16_t channel;
  byte control = 0b00000110;

  // canale 0
  channel = 0;
  channel_shifted = channel << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch0 = SPI.transfer16(channel_shifted) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  // canale 1
  channel = 1;
  channel_shifted = channel << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch1 = SPI.transfer16(channel_shifted) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  // canale 2
  channel = 2;
  channel_shifted = channel << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch2 = SPI.transfer16(channel_shifted) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  // canale 3
  channel = 3;
  channel_shifted = channel << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch3 = SPI.transfer16(channel_shifted) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  xQueueSendFromISR(msg_queue, (void *)&campione, &xHigherPriorityTaskWoken);

}

void printTask(void *parameters)
{
  i16Data rawValue;

  while (1)
  {
    if (xQueueReceive(msg_queue, (void *)&rawValue, 0) == pdTRUE)
    {
      Serial.print(">ch0:");
      Serial.println(rawValue.ch0);
      Serial.print(">ch1:");
      Serial.println(rawValue.ch1);
      Serial.print(">ch2:");
      Serial.println(rawValue.ch2);
      Serial.print(">ch3:");
      Serial.println(rawValue.ch3);

      Serial.println(uxQueueSpacesAvailable(msg_queue ));

      if(!uxQueueSpacesAvailable(msg_queue )){
        timerAlarmDisable(timer0);
      }else{
        timerAlarmEnable(timer0);
      }
    }
  }
}
