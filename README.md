# Prova 3c: ADC multi canale con MCP3204

La ESP32 si collega in SPI con un ADC a 4 canali MCP3204, che ha risoluzione di 12bit.
 
Il programma acquisisce le tensioni dei 4 canali (ciascuno condizionato nell'intervallo da 0 a 3.3V) con velocità di campionamento di 10 campionamenti al secondo.
I dati vengono emessi sul terminale seriale per essere visualizzati con TelePlot in VS Code.
