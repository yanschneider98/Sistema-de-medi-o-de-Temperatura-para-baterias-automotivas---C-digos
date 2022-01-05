/*********
  Projeto sistema de medição de temperatura para baterias de automóveis. https://sites.google.com/view/sistema-de-medicao-de-temp/introdu%C3%A7%C3%A3o
 
  Esse código lê 'n' sensores DS18B20 printa no monitor serial a temperatura medida por cada sensor e mostra o valor médio em um display OLED 0,96" 128x64 pixels.
  Além disso permite setar dois valores de temperatura máxima e mínima para acionar um alarme.
  O código utiliza os dois núcleos presentes no ESP32 através do multitask, por padrão a IDE do arduino apenas trabalha com um núcleo.
  Task1 roda no núcleo 0 e void loop no núcleo 1. 
  
*********/

//carrega bibliotecas ////////////////////////////////

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
///////////////////////////////////////////////////////////////////////////////////////

TaskHandle_t Task1; //cria task

///////////////////////////////////////////////////////////////////////////////////////

#define SCREEN_WIDTH 128 // Display OLED comprimento, em pixels
#define SCREEN_HEIGHT 64 // Display OLED altura, em pixels

float media = 0; //cria uma variavél tipo float para calcular e armazenar o valor da média posteriormente.

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
  Serial.begin(115200); 

  pinMode(buzzer, OUTPUT);
    //Inicia a biblioteca
  sensors.begin();

/////////////////////////// Inicializa o display /////////////////////////////////////

   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);}
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
  for(int i=0;i<numberOfDevices; i++){
    // Procura no fio para o endereço
    if(sensors.getAddress(tempDeviceAddress, i)){
        Serial.print("Encontrado dispositivo");
        Serial.print(i, DEC);
        Serial.print(" com endereço: ");
        printAddress(tempDeviceAddress); //vai para a função printAddress
        Serial.println();
    } else {
        Serial.print("Encontrado dispositivo fantasma em ");
        Serial.print(i, DEC);
        Serial.print(",mas não detectado endereço. Verifique a conexão de alimentação.");
    }


    //******** Configuração dos alarmes
      Serial.println("Setando temperaturas de alarmes...");

      // Alarme para quando a temperatura é maior que 'x'
      sensors.setHighAlarmTemp(tempDeviceAddress, 30);
      
      // Alarme para quando a temperatura é menor que 'x'
      sensors.setLowAlarmTemp(tempDeviceAddress, -10);

      Serial.print("Novo alarme: ");
      Serial.print(i, DEC);
      Serial.print("Alarmes: ");
      printAlarms(tempDeviceAddress); // vai para a função printAlarms
      Serial.println();
  }

  
  //Cria a task que irá ser executada na função Taskcode(), com prioridade 1 e executada no núcleo 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Função Task. */
                    "Task1",     /* Nome da task. */
                    10000,       /* Stack size da task */
                    NULL,        /* parametro da task */
                    1,           /* prioridade da task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 
  
}

//     fim do void setup  ************************************************************************************************************* 
// ************************************************************************************************************************************ 



//Task1code
void Task1code( void * pvParameters ){

  for(;;){
      Serial.println();
      Serial.print("Verificando alarme");
      checkAlarm(tempDeviceAddress);
      delay(100);
    }
  } 


void loop(){ 
  sensors.requestTemperatures(); //Manda um comando para pegar temperaturas
  // Faz o loop por cada dispositivo, e mostra no monitor serial cada temperatura
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      // Output the device ID
      Serial.println();
      Serial.print("Temperatura para dispositivo: ");
      Serial.println(i,DEC);
      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print("Temp C: ");
      Serial.print(tempC);
      Serial.println();

      media += sensors.getTempC(tempDeviceAddress); // soma as temperaturas lidas ao longo loop na variavel media
          
    }
  }
  media = media/numberOfDevices; //calcula a média dos sensores
  
    // clear display - Mostra o resultado da media no display
    display.clearDisplay();
    
    // Mostra a temperatura no display
    display.setTextSize(1);
    display.setCursor(0,20);
    display.print("Temperatura: ");
    display.setTextSize(2);
    display.setCursor(0,30);
    display.print(media);
    display.print(" ");
    display.setTextSize(1);
    display.cp437(true);
    display.write(167);
    display.setTextSize(2);
    display.print("C");
    
    display.display();
    
    Serial.println();//mostra a temperatura média no monitor serial
    Serial.print("Temperatura média: ");
    Serial.print(media);
    Serial.println();
    
    media = 0; //zera o valor da media para a próxima média.
  
  Serial.println();
  Serial.print("********************************************");
  delay(1000); // intervalo de 5 segundas para mostra a próxima amostragem
}

// função para printar o endereço do dispositivo
void printAddress(DeviceAddress tempDeviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
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

  // função para checar se o sensor entrou na condição de disparar o alarme.
       void checkAlarm(DeviceAddress tempDeviceAddress){
        if (sensors.hasAlarm(tempDeviceAddress))
        {
          Serial.print("ALARME DISPOSITIVO: ");
          printAddress(tempDeviceAddress);
              // clear display
          display.clearDisplay();
          
          // Mostra a temperatura de alarme no display
          display.setTextSize(1);
          display.setCursor(0,20);
          display.print("!ALERTA!: ");
          display.setTextSize(2);
          display.setCursor(0,30);
          float tempC = sensors.getTempC(tempDeviceAddress);
          display.print(tempC);
          display.print(" ");
          display.setTextSize(1);
          display.cp437(true);
          display.write(167);
          display.setTextSize(2);
          display.print("C");
          
          display.display();

          for(int i=0;i<10;i++){
            digitalWrite(buzzer,HIGH);
            delay(200);
            digitalWrite(buzzer,LOW);
            delay(200);
          }
          
        }
      }
