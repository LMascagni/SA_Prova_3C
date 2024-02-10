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

static const uint8_t msgQueueLen = 10; //dimensione della coda di comunicazione tra ISR e stampa
static QueueHandle_t msg_queue;
volatile i16Data campione;            // un campionamento dei 4 canali della ISR

//per ora inutile
int frequency = 8000; // Hz

hw_timer_t *timer0 = NULL;

volatile float v[N_CHANNELS];

void IRAM_ATTR readADC();

void printTask(void *parameters);

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
  
  // impostazione della linea di chip select dell'ADC
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

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

  for (uint16_t ch = 0; ch < N_CHANNELS; ch++)
  {
    uint16_t rawValue;
    byte control = 0b00000110;
    uint16_t channel = ch << 14;

    digitalWrite(CS_PIN, LOW);

    SPI.transfer(control);
    rawValue = SPI.transfer16(channel);

    digitalWrite(CS_PIN, HIGH);
    

    if(xQueueSendFromISR(msg_queue, (void *)&rawValue, &xHigherPriorityTaskWoken) != pdTRUE){
    //  Serial.println("coda piena");
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
  i16Data rawValue;
  
  while (1)
  {
    if (xQueueReceive(msg_queue, (void *)&rawValue, 0) == pdTRUE)
    {
      Serial.println(rawValue.ch0);
      Serial.println(rawValue.ch1);
      Serial.println(rawValue.ch2);
      Serial.println(rawValue.ch3);            
    }
    else
    {
      //Serial.println("attesa dati...");
    }
    
  }
}