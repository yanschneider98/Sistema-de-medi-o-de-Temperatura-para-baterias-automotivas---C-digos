// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Data wire is connected to GPIO15
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with a OneWire device
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

DeviceAddress sensor1 = { 0x28, 0xFF, 0x64, 0x1, 0xB7, 0xF3, 0xB2, 0x4D };
DeviceAddress sensor2 = { 0x28, 0xFF, 0x64, 0x1, 0xB0, 0x1D, 0x68, 0xB7 };
DeviceAddress sensor3= { 0x28, 0xFF, 0x64, 0x1, 0xB7, 0xCD, 0x39, 0x10 };

void setup(void){
  Serial.begin(115200);
  sensors.begin();
  
   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
   for(;;);}
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
}

void loop(void){ 
  delay(5000);
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  Serial.print("Sensor 1(*C): ");
  Serial.print(sensors.getTempC(sensor1)); 
  Serial.print(" Sensor 1(*F): ");
  Serial.println(sensors.getTempF(sensor1)); 
 
  Serial.print("Sensor 2(*C): ");
  Serial.print(sensors.getTempC(sensor2)); 
  Serial.print(" Sensor 2(*F): ");
  Serial.println(sensors.getTempF(sensor2)); 
  
  Serial.print("Sensor 3(*C): ");
  Serial.print(sensors.getTempC(sensor3)); 
  Serial.print(" Sensor 3(*F): ");
  Serial.println(sensors.getTempF(sensor3)); 

  float media = (sensors.getTempC(sensor1) + sensors.getTempC(sensor2) + sensors.getTempC(sensor3 ))/3;
  Serial.print("Media(*C): ");
  Serial.print(media); 
  Serial.println();
  
  // clear display
  display.clearDisplay();
  
  // display temperature
  display.setTextSize(1);
  display.setCursor(0,20);
  display.print("Temperature: ");
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
}
