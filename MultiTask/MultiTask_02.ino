/*********
  Projeto sistema de medição de temperatura para baterias de automóveis. https://sites.google.com/view/sistema-de-medicao-de-temp/introdu%C3%A7%C3%A3o
  Esse código lê 'n' sensores DS18B20 printa no monitor serial a temperatura medida por cada sensor e mostra o valor médio em um display OLED 0,96" 128x64 pixels.
  Além disso permite setar dois valores de temperatura máxima e mínima para acionar um alarme.
  O código trabalha com Multicores
  
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // Display OLED comprimento, em pixels
#define SCREEN_HEIGHT 64 // Display OLED altura, em pixels

// Declaração para um Display SSD1306 que está conectado nas portas I2C(SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//variaveis que indicam o núcleo
static uint8_t taskCoreZero = 0;
static uint8_t taskCoreOne  = 1;

float media = 0; //cria uma variavél tipo float para calcular e armazenar o valor da média posteriormente.

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
 
/////////////////////////// Inicializa o display /////////////////////////////////////

   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);}
    delay(5000);
    display.clearDisplay();
    display.setTextColor(WHITE, BLACK);
    
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
        //printAddress(tempDeviceAddress); //vai para a função printAddress
        Serial.println();
        sensors.setResolution(tempDeviceAddress, 9); //seta resolução dos sensores, padrão é 12 bits
    } else {
        Serial.print("Encontrado dispositivo fantasma em ");
        Serial.print(i, DEC);
        Serial.print(",mas não detectado endereço. Verifique a conexão de alimentação.");
    }


    //******** Configuração dos alarmes
     /* Serial.println("Setando temperaturas de alarmes...");

      // Alarme para quando a temperatura é maior que 'x'
      sensors.setHighAlarmTemp(tempDeviceAddress, 31);
      
      // Alarme para quando a temperatura é menor que 'x'
      sensors.setLowAlarmTemp(tempDeviceAddress, -10);

      Serial.print("Novo alarme: ");
      Serial.print(i, DEC);
      Serial.print("Alarmes: ");
      printAlarms(tempDeviceAddress); // vai para a função printAlarms
      Serial.println(); */
  }

  //cria uma tarefa que será executada na função coreTaskZero, com prioridade 3 e execução no núcleo 0
  //coreTaskZero: Ler os sensores
  xTaskCreatePinnedToCore(
                    coreTaskZero,   /* função que implementa a tarefa */
                    "coreTaskZero", /* nome da tarefa */
                    10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    3,          /* prioridade da tarefa (0 a N) */
                    NULL,       /* referência para a tarefa (pode ser NULL) */
                    taskCoreZero);         /* Núcleo que executará a tarefa */
                    
  delay(500); //tempo para a tarefa iniciar

  //cria uma tarefa que será executada na função coreTaskOne, com prioridade 2 e execução no núcleo 1
  //coreTaskOne: atualizar as informações do display
  xTaskCreatePinnedToCore(
                    coreTaskOne,   /* função que implementa a tarefa */
                    "coreTaskOne", /* nome da tarefa */
                    10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    2,          /* prioridade da tarefa (0 a N) */
                    NULL,       /* referência para a tarefa (pode ser NULL) */
                    taskCoreOne);         /* Núcleo que executará a tarefa */

    delay(500); //tempo para a tarefa iniciar

   //cria uma tarefa que será executada na função coreTaskTwo, com prioridade 2 e execução no núcleo 0
   //coreTaskTwo: vigiar o botão para detectar quando pressioná-lo
   xTaskCreatePinnedToCore(
                    coreTaskTwo,   /* função que implementa a tarefa */
                    "coreTaskTwo", /* nome da tarefa */
                    10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    2,          /* prioridade da tarefa (0 a N) */
                    NULL,       /* referência para a tarefa (pode ser NULL) */
                    taskCoreZero);         /* Núcleo que executará a tarefa */
   
}

void loop() {}

//essa função ficará lendo os sensores
//e a cada piscada (ciclo acender e apagar) incrementará nossa variável blinked
void coreTaskZero( void * pvParameters ){
 
    String taskMessage = "Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    Serial.println(taskMessage);  //log para o serial monitor
 
    while(true){
      
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
            //Serial.print(" Temp F: ");
            //Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
            Serial.println();
            //checkAlarm(tempDeviceAddress);
      
            media += sensors.getTempC(tempDeviceAddress); // soma as temperaturas lidas ao longo loop na variavel media
        }
      }
      media = media/numberOfDevices; //calcula a média dos sensores
      delay(1000);
      media = 0; //zera o valor da media para a próxima média.
    } 
}

//essa função será responsável apenas por atualizar as informações no 
//display a cada 100ms
void coreTaskOne( void * pvParameters ){
     while(true){

      display.setTextColor(WHITE, BLACK);
      
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

      delay(50);
    } 
}

//essa função será responsável por ler o estado do botão
//e atualizar a variavel de controle.
void coreTaskTwo( void * pvParameters ){
     while(true){
      
      delay(10);
    } 
}
