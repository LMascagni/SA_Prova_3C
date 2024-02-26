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
#include <Wifi.h>
#include <WifiUdp.h>

#define N_CHANNELS 4

#define WIFI_TIMEOUT_MS 10000       // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 1000 // Wait 30 seconds after a failed connection attempt

const int CS_PIN = 5;
const int MOSI_PIN = 23;
const int MISO_PIN = 19;
const int CLK_PIN = 18;

unsigned int localPort = 9999;
const char *ssid = "Pisello";
const char *password = "caccacacca";

static const uint8_t msgQueueLen = 100; // dimensione della coda di comunicazione tra ISR e stampa
static QueueHandle_t msg_queue;
volatile i16Data campione; // un campionamento dei 4 canali della ISR

hw_timer_t *timer0 = NULL;

WiFiUDP udp;

void IRAM_ATTR readADC();

void printTask(void *parameters);

void keepWiFiAlive(void *parameter);

void setup()
{
  // inizializzazione comunicazione seriale
  Serial.begin(921600);
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
  /*
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
  */
  xTaskCreatePinnedToCore(
      keepWiFiAlive,
      "keepWiFiAlive",
      5000,
      NULL,
      1,
      NULL,
      ARDUINO_RUNNING_CORE);

  // avvio del timer per il triggering della ISR
 // timerAlarmEnable(timer0);
}

void loop()
{
  // può rimanere vuoto
}

void IRAM_ATTR readADC()
{
  BaseType_t xHigherPriorityTaskWoken;

  // BaseType_t task_woken = pdFALSE;

  uint16_t rawValue;
  byte control = 0b00000110;
  uint16_t channel;
  uint16_t ch;
  // canale 0
  ch = 0;
  channel = ch << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch0 = SPI.transfer16(channel) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  // canale 1
  ch = 1;
  channel = ch << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch1 = SPI.transfer16(channel) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  // canale 2
  ch = 2;
  channel = ch << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch2 = SPI.transfer16(channel) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  // canale 3
  ch = 3;
  channel = ch << 14;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(control);
  campione.ch3 = SPI.transfer16(channel) & 0x0fff;
  digitalWrite(CS_PIN, HIGH);

  if (xQueueSendFromISR(msg_queue, (void *)&campione, &xHigherPriorityTaskWoken) != pdTRUE)
  {
  }
  /*
    if (xHigherPriorityTaskWoken)
    {
      portYIELD_FROM_ISR();
    }
  */
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

      Serial.println(uxQueueSpacesAvailable(msg_queue));

      if (!uxQueueSpacesAvailable(msg_queue))
      {
        timerAlarmDisable(timer0);
      }
      else
      {
        timerAlarmEnable(timer0);
      }
    }
    else
    {
      // Serial.println("attesa dati...");
    }
  }
}

void keepWiFiAlive(void *parameter)
{
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println("[WIFI] Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
    {
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("[WIFI] FAILED");
      vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println("[WIFI] Connected: " + WiFi.localIP());
  }
}