/*********
  Projeto sistema de medição de temperatura para baterias de automóveis. https://sites.google.com/view/sistema-de-medicao-de-temp/introdu%C3%A7%C3%A3o
  Esse código lê 'n' sensores DS18B20 printa no monitor serial a temperatura medida por cada sensor e mostra o valor médio em um display OLED 0,96" 128x64 pixels.
  Além disso permite setar dois valores de temperatura máxima e mínima para acionar um alarme.

*********/

//carrega bibliotecas ////////////////////////////////

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//bibliotecas para SD
#include "FS.h"
#include "SD.h"
#include <SPI.h>
//bibliotecas para pegar hora pela internet
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// Colocar dados da rede Wi-Fi
const char* ssid     = "moto";
const char* password = "ff1234567";

// Define pino CS para o SD card module
#define SD_CS 5

// Save reading number on RTC memory
RTC_DATA_ATTR int readingID = 0;

String dataMessage;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

///////////////////////////////////////////////////////////////////////////////////////

#define SCREEN_WIDTH 128 // Display OLED comprimento, em pixels
#define SCREEN_HEIGHT 64 // Display OLED altura, em pixels

float media = 0; //cria uma variavél tipo float para calcular e armazenar o valor da média posteriormente.
float tempC;  //VARIAVEL QUE ARMAZENA TEMPERATURA MEDIDA DO SENSOR
float temp_desvio_padrao [10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float DesvioPadrao = 0;
char endereco[] = "N/A";

bool controle = false;


// Declaração para um Display SSD1306 que está conectado nas portas I2C(SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

///////////////////////////////////////////////////////////////////////////////////////

// Fio de dados está conectado na porta GPIO 4
#define ONE_WIRE_BUS 4

#define buzzer 2

// Setup para a instância oneWire se comunicar com qualquer dispositivo OneWire (não apenas para Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Passa nossa referência oneWire para Dallas temperature.
DallasTemperature sensors(&oneWire);

// Número(quantidade) de dispositivos de temperatura encontrados
int numberOfDevices;

// Usaremos essa variavel para armazenar o endereço do dispositivo encontrado.
DeviceAddress tempDeviceAddress;

///////////////////////////////////////////////////////////////////////////////////////

void setup() {

  pinMode(buzzer, OUTPUT);

  // inicia a porta serial USAR SEMPRE EM 115200
  Serial.begin(115200);
  //Inicia a biblioteca
  sensors.begin();
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-10800);

  // Initialize SD card
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if (!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "ID Leitura, Data, Hora, Temperatura média, Desvio Padrão, Endereço Alarme, Temperatura do sensor em alarme\r\n");
  }
  else {
    Serial.println("File already exists");
  }
  file.close();

  /////////////////////////// Inicializa o display /////////////////////////////////////

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(5000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  ///////////////////////////////////////////////////////////////////////////////////////


  // Conta a quantidade de sensores no barramento
  numberOfDevices = sensors.getDeviceCount();

  // Localiza os dispositivos no barramento
  Serial.print("Localizando dispositivos...");
  Serial.print("Encontrado ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" dispositivo.");



  // Faz o loop por cada dispositivo, e mostra o endereço
  for (int i = 0; i < numberOfDevices; i++) {
    // Procura no fio para o endereço
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Encontrado dispositivo");
      Serial.print(i, DEC);
      Serial.print(" com endereço: ");
      printAddress(tempDeviceAddress); //vai para a função printAddress
      Serial.println();
      sensors.setResolution(tempDeviceAddress, 9); //seta resolução dos sensores, padrão é 12 bits
    } else {
      Serial.print("Encontrado dispositivo fantasma em ");
      Serial.print(i, DEC);
      Serial.print(",mas não detectado endereço. Verifique a conexão de alimentação.");
    }

    ////////////////////////////// reordenação


    //******** Configuração dos alarmes
    Serial.println("Setando temperaturas de alarmes...");

    // Alarme para quando a temperatura é maior que 'x'
    sensors.setHighAlarmTemp(tempDeviceAddress, 32);

    // Alarme para quando a temperatura é menor que 'x'
    sensors.setLowAlarmTemp(tempDeviceAddress, -10);

    Serial.print("Novo alarme: ");
    Serial.print(i, DEC);
    Serial.print("Alarmes: ");
    printAlarms(tempDeviceAddress); // vai para a função printAlarms
    Serial.println();
  }
}

//     fim do void setup  *************************************************************************************************************


void loop() {
  sensors.requestTemperatures(); //Manda um comando para pegar temperaturas

  for (int i = 0; i < numberOfDevices; i++) { //loop faz a leitura de temperatura em todos os sensores e verifica se algum sensor está detectando um alarme
    if (sensors.hasAlarm(tempDeviceAddress)) {
      Serial.println();
      Serial.print("ALARM: ");
      controle = true;
      printAddress(tempDeviceAddress); //printa o endereço do sensor no monitor serial
      printBuzzerEDisplay(tempDeviceAddress); //mostra a temperatura de alarme no display

    }
    if (!sensors.hasAlarm(tempDeviceAddress)) {
      digitalWrite(buzzer, LOW);
      controle = false;
      temperaturaAtual(tempDeviceAddress, i); //mostra a temperatura atual (média)
    }

  }

}

void temperaturaAtual(DeviceAddress tempDeviceAddress, int i) {

  // Search the wire for address
  if (sensors.getAddress(tempDeviceAddress, i)) {
    // Output the device ID
    Serial.println();
    Serial.print("Temperatura para dispositivo: ");
    Serial.println(i, DEC);
    // Print the data
    tempC = sensors.getTempC(tempDeviceAddress);
    Serial.print("Temp C: ");
    Serial.print(tempC);
    //Serial.print(" Temp F: ");
    //Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
    Serial.println();
    media += tempC; // soma as temperaturas lidas ao longo loop na variavel media


    if (i == 0) temp_desvio_padrao[i] = tempC;
    if (i == 1) temp_desvio_padrao[i] = tempC;
    if (i == 2) temp_desvio_padrao[i] = tempC;
    if (i == 3) temp_desvio_padrao[i] = tempC;

  }

  if (i == 2) {
    media = media / numberOfDevices; //calcula a média dos sensores
    Serial.println();//mostra a temperatura média no monitor serial
    Serial.print("Temperatura média: ");
    Serial.print(media);
    Serial.println();

    //Calcula desvio padrão

    DesvioPadrao = sqrt(((pow((temp_desvio_padrao[0] - media), 2) + pow((temp_desvio_padrao[1] - media), 2) + pow((temp_desvio_padrao[2] - media), 2)) / numberOfDevices));
    Serial.print("Desvio Padrão: ");
    Serial.print(DesvioPadrao);
    Serial.println();


    printTela(media, DesvioPadrao);
    getTimeStamp();
    logSDCard(tempDeviceAddress);

    // Increment readingID on every new reading
    readingID++;

    media = 0; //zera o valor da media para a próxima média.
  }


  Serial.println();
  Serial.print("********************************************");
  delay(1000); // intervalo de 5 segundas para mostra a próxima amostragem

}

void printTela(float media, float DesvioPadrao) {

  // clear display - Mostra o resultado da media no display
  display.clearDisplay();

  // Mostra a temperatura no display
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temperatura: ");
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(media);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("Desvio Padrao: ");
  display.setTextSize(2);
  display.setCursor(0, 40);
  display.print(DesvioPadrao);
  display.print(" ");

  display.display();

}

// função para printar o endereço do dispositivo
void printAddress(DeviceAddress tempDeviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (tempDeviceAddress[i] < 16) Serial.print("0");
    Serial.print(tempDeviceAddress[i], HEX);
    }
}

// função para printar alarmes
void printAlarms(uint8_t tempDeviceAddress[]) /// deviceAddress
{
  char temp;
  temp = sensors.getHighAlarmTemp(tempDeviceAddress);
  Serial.print("Alarme de temperatura elevada: ");
  Serial.print(temp, DEC);
  Serial.print("C/");
  Serial.print(DallasTemperature::toFahrenheit(temp));
  Serial.print("F | Alarme de temperatura baixa: ");
  temp = sensors.getLowAlarmTemp(tempDeviceAddress);
  Serial.print(temp, DEC);
  Serial.print("C/");
  Serial.print(DallasTemperature::toFahrenheit(temp));
  Serial.print("F");
}

//print de alertas no display e no buzzer
void printBuzzerEDisplay(DeviceAddress tempDeviceAddress) {

  tempC = sensors.getTempC(tempDeviceAddress);

  // clear display - Mostra o resultado da media no display
  //display.clearDisplay();
  display.clearDisplay();
  // Mostra a temperatura no display
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("!!! ALARME !!! ");
  display.setTextSize(2);
  display.setCursor(0, 40);
  display.print(tempC);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  display.display();

  digitalWrite(buzzer, HIGH);

  printAddress(tempDeviceAddress);
  getTimeStamp();
  logSDCard(tempDeviceAddress);
  delay(1000);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// FUNÇÕES PARA O DATALOGGING

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Function to get date and time from NTPClient
void getTimeStamp() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.println(timeStamp);
}

// Write the sensor readings on the SD card
void logSDCard(DeviceAddress tempDeviceAddress) {
  if (controle == true) {
    dataMessage = String(readingID) + "," + String(dayStamp) + "," + String(timeStamp) + "," +
                  String(media) + "," + String(DesvioPadrao) + "," + 
                  String(endereco) + "," + String(tempC) + "\r\n";
  }
  if (controle == false) {
    dataMessage = String(readingID) + "," + String(dayStamp) + "," + String(timeStamp) + "," +
                  String(media) + "," + String(DesvioPadrao) + "\r\n";
  }

  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
