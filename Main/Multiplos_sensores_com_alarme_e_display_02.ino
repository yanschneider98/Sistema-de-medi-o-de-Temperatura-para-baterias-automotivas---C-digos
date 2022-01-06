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

void setup(){

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
      sensors.setHighAlarmTemp(tempDeviceAddress, 31);
      
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


void loop(){ 
  sensors.requestTemperatures(); //Manda um comando para pegar temperaturas
  
  for(int i=0;i<numberOfDevices; i++){ //loop faz a leitura de temperatura em todos os sensores e verifica se algum sensor está detectando um alarme
    if(sensors.hasAlarm(tempDeviceAddress)){
      Serial.println();
      Serial.print("ALARM: ");
      printAddress(tempDeviceAddress); //printa o endereço do sensor no monitor serial
      printBuzzerEDisplay(tempDeviceAddress); //mostra a temperatura de alarme no display
    }
    if(!sensors.hasAlarm(tempDeviceAddress)){
      temperaturaAtual(tempDeviceAddress, i); //mostra a temperatura atual (média)
    }
    
  }
      
}


void temperaturaAtual(DeviceAddress tempDeviceAddress, int i){

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
      media += sensors.getTempC(tempDeviceAddress); // soma as temperaturas lidas ao longo loop na variavel media
      //checkAlarm(tempDeviceAddress);
    }
 
    
    if(i==2){
      media = media/numberOfDevices; //calcula a média dos sensores
      Serial.println();//mostra a temperatura média no monitor serial
      Serial.print("Temperatura média: ");
      Serial.print(media);
      Serial.println();
      printTela(media);
      media = 0; //zera o valor da media para a próxima média.
    }
    
  
    Serial.println();
    Serial.print("********************************************");
    delay(250); // intervalo de 5 segundas para mostra a próxima amostragem
    
}

void printTela(float media){
      
      // clear display - Mostra o resultado da media no display
      display.clearDisplay();
      
      // Mostra a temperatura no display
      display.setTextSize(1);
      display.setCursor(0,0);
      display.print("Temperatura: ");
      display.setTextSize(2);
      display.setCursor(0,10);
      display.print(media);
      display.print(" ");
      display.setTextSize(1);
      display.cp437(true);
      display.write(167);
      display.setTextSize(2);
      display.print("C");
      
      display.display();
  
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

//print de alertas no display e no buzzer
void printBuzzerEDisplay(DeviceAddress tempDeviceAddress){


      float tempC = sensors.getTempC(tempDeviceAddress);

      // clear display - Mostra o resultado da media no display
      //display.clearDisplay();
      display.clearDisplay();
      // Mostra a temperatura no display
      display.setTextSize(1);
      display.setCursor(0,30);
      display.print("!!! ALARME !!! ");
      display.setTextSize(2);
      display.setCursor(0,40);
      display.print(tempC);
      display.print(" ");
      display.setTextSize(1);
      display.cp437(true);
      display.write(167);
      display.setTextSize(2);
      display.print("C");
      
      display.display();
      
      digitalWrite(buzzer,HIGH);
      delay(200);
      digitalWrite(buzzer,LOW);
      delay(200);
      
}
