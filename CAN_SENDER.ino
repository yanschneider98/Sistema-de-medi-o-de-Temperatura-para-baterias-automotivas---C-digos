// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>
#include <OneWire.h>
#include <DallasTemperature.h>

float media = 0;
bool watch_media = true;

#define ONE_WIRE_BUS 23



// Setup para a instância oneWire se comunicar com qualquer dispositivo OneWire (não apenas para Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Passa nossa referência oneWire para Dallas temperature. 
DallasTemperature sensors(&oneWire);

// Número(quantidade) de dispositivos de temperatura encontrados
int numberOfDevices;

// Usaremos essa variavel para armazenar o endereço do dispositivo encontrado. 
DeviceAddress tempDeviceAddress; 


void setup() {
  Serial.begin(9600);
  
   sensors.begin();
 

  //******** Configuração dos alarmes
      Serial.println("Setando temperaturas de alarmes...");

      // Alarme para quando a temperatura é maior que 'x'
      sensors.setHighAlarmTemp(tempDeviceAddress, 34);
      
      // Alarme para quando a temperatura é menor que 'x'
      sensors.setLowAlarmTemp(tempDeviceAddress, -10);
  
 
  

  Serial.println("CAN Sender");

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  numberOfDevices = sensors.getDeviceCount();
}


void loop() {

       sensors.requestTemperatures(); //Manda um comando para pegar temperaturas
      // Faz o loop por cada dispositivo, e mostra no monitor serial cada temperatura
        Serial.println(numberOfDevices);
        
      for(int i=0;i<numberOfDevices; i++){
        // Search the wire for address
      
        if(sensors.getAddress(tempDeviceAddress, i)){
            // Output the device ID
            Serial.println();
            Serial.print("Temperatura para dispositivo: ");
            Serial.println(i,DEC);
            // Print the data
            double tempC = sensors.getTempC(tempDeviceAddress);
            Serial.print("Temp C: ");
            Serial.print(tempC);
             CAN.beginExtendedPacket(0xabcdef);
             CAN.write(tempC);
             CAN.endPacket();

            Serial.println("Enviado");
            delay(1000);
            
            //Serial.print(" Temp F: ");
            //Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
            Serial.println();
          
      
            media += sensors.getTempC(tempDeviceAddress); // soma as temperaturas lidas ao longo loop na variavel media
        }
        }

}

void printAddress(DeviceAddress tempDeviceAddress) {
  delay(200);
  for (uint8_t i = 0; i < 8; i++){
    if (tempDeviceAddress[i] < 16) Serial.print("0");
          Serial.print(tempDeviceAddress[i], HEX);
  }
}
