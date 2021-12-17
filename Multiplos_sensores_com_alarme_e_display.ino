/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com  
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

///////////////////////////////////////////////////////////////////////////////////////

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

float media = 0;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

///////////////////////////////////////////////////////////////////////////////////////

// Data wire is plugged TO GPIO 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Number of temperature devices found
int numberOfDevices;

// We'll use this variable to store a found device address
DeviceAddress tempDeviceAddress; 

void setup(){
  // start serial port
  Serial.begin(115200);
  
  // Start up the library
  sensors.begin();

////////////////////////////////////display///////////////////////////////////////////////////

   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);}
    delay(5000);
    display.clearDisplay();
    display.setTextColor(WHITE);
///////////////////////////////////////////////////////////////////////////////////////

  
  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
      Serial.println("Setting alarm temps...");

      // alarm when temp is higher than 30C
      sensors.setHighAlarmTemp(tempDeviceAddress, 30);
      
      // alarm when temp is lower than -10C
      sensors.setLowAlarmTemp(tempDeviceAddress, -10);

      Serial.print("New Device Alarms: ");
      Serial.print(i, DEC);
      Serial.print("Alarms: ");
      printAlarms(tempDeviceAddress);
      Serial.println();
  }

  
}

void loop(){ 
  sensors.requestTemperatures(); // Send the command to get temperatures
  
  // Loop through each device, print out temperature data
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      // Output the device ID
      Serial.println();
      Serial.print("Temperature for device: ");
      Serial.println(i,DEC);
      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print("Temp C: ");
      Serial.print(tempC);
      Serial.print(" Temp F: ");
      Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
      checkAlarm(tempDeviceAddress);

      media += sensors.getTempC(tempDeviceAddress);
          
    }
  }
  media = media/numberOfDevices; //calcula a média dos sensores
  
    // clear display - Mostra o resultado da media no display
    display.clearDisplay();
    
    // display temperature
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
    
    media = 0; //zera o valor da media para a proxima media.
  
  Serial.println();
  Serial.print("********************************************");
  delay(5000);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
          Serial.print(deviceAddress[i], HEX);
  }
  
}


void printAlarms(uint8_t tempDeviceAddress[]) /// deviceAddress
{
  char temp;
  temp = sensors.getHighAlarmTemp(tempDeviceAddress);
  Serial.print("High Alarm: ");
  Serial.print(temp, DEC);
  Serial.print("C/");
  Serial.print(DallasTemperature::toFahrenheit(temp));
  Serial.print("F | Low Alarm: ");
  temp = sensors.getLowAlarmTemp(tempDeviceAddress);
  Serial.print(temp, DEC);
  Serial.print("C/");
  Serial.print(DallasTemperature::toFahrenheit(temp));
  Serial.print("F");
}


 void checkAlarm(DeviceAddress tempDeviceAddress)
{
  if (sensors.hasAlarm(tempDeviceAddress))
  {
    Serial.print("ALARM: ");
    printAddress(tempDeviceAddress);
  }
}
