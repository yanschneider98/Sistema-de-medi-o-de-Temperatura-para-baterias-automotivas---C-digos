#include <Adafruit_GFX.h> // inclui biblioteca de gráficos Adafruit
#include <Adafruit_SSD1306.h> // inclui driver de exibição OLED Adafruit SSD1306
 
#define OLED_RESET 4 // define display reset pin
Adafruit_SSD1306 display(OLED_RESET);
 
#define DS18B20_PIN A0 // define a conexão do pino de dados DS18B20
 
void setup(void)
{
   delay(1000); // espere um segundo
 
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // inicializa o display OLED SSD1306 com endereço I2C 0x3C
   display.clearDisplay(); // limpa o buffer de exibição.
 
   display.setTextSize(2);
   display.setTextColor(WHITE, BLACK);
   display.setCursor(34, 5);
   display.print("CAPSISTEMA");
 
   display.setTextSize(1);
   display.setTextColor(WHITE, BLACK);
   display.setCursor(29, 33);
   display.print("Temperatura:");
   display.display();
   display.setTextSize(2);
}
 
unsigned int ds18b20_temp;
char _buffer[11];
 
void loop()
{
   display.setCursor(1, 50);
 
   if(ds18b20_read(&ds18b20_temp))
   {
      if (ds18b20_temp & 0x8000) // se a temperatura <0
      {
          ds18b20_temp = ~ds18b20_temp + 1; // altera o valor da temperatura para a forma positiva
          sprintf(_buffer, "-%02u.%04u C", (ds18b20_temp/16) % 100, (ds18b20_temp & 0x0F) * 625);
      } else { // caso contrário (temperatura> = 0)
        if (ds18b20_temp/16 > 100) // se a temperatura> = 100 ° C
           sprintf(_buffer, "%03u.%04u C", ds18b20_temp/16, (ds18b20_temp & 0x0F) * 625);
        else // caso contrário (0 <= temperatura <100)
           sprintf(_buffer, " %02u.%04u C", ds18b20_temp/16, (ds18b20_temp & 0x0F) * 625);
      }
 
      display.print(_buffer);
      display.drawCircle(103, 52, 2, WHITE); // colocar símbolo de grau ( ° )
 
   } else
     display.print(" ERROR ");
   
   display.display();
   delay(1000); // espere um segundo
}
 
bool ds18b20_start()
{
   bool ret = 0;
   digitalWrite(DS18B20_PIN, LOW); // envia pulso de redefinição para o sensor DS18B20
   pinMode(DS18B20_PIN, OUTPUT);
   delayMicroseconds(500); // espere 500 nós
   pinMode(DS18B20_PIN, INPUT);
   delayMicroseconds(100); // aguarde para ler a resposta do sensor DS18B20
   if (!digitalRead(DS18B20_PIN))
   {
      ret = 1; // Sensor DS18B20 está presente
      delayMicroseconds(400); // wait 400 us
   }
   return(ret);
}
 
void ds18b20_write_bit(bool value)
{
   digitalWrite(DS18B20_PIN, LOW);
   pinMode(DS18B20_PIN, OUTPUT);
   delayMicroseconds(2);
   digitalWrite(DS18B20_PIN, value);
   delayMicroseconds(80);
   pinMode(DS18B20_PIN, INPUT);
   delayMicroseconds(2);
}
 
void ds18b20_write_byte(byte value)
{
   byte i;
   for(i = 0; i < 8; i++)
      ds18b20_write_bit(bitRead(value, i));
}
 
bool ds18b20_read_bit(void)
{
   bool value;
   digitalWrite(DS18B20_PIN, LOW);    
   pinMode(DS18B20_PIN, OUTPUT);
   delayMicroseconds(2);
   pinMode(DS18B20_PIN, INPUT);
   delayMicroseconds(5);
   value = digitalRead(DS18B20_PIN);
   delayMicroseconds(100);
   return value;
}
 
byte ds18b20_read_byte(void)
{
   byte i, value;
   for(i = 0; i < 8; i++)
      bitWrite(value, i, ds18b20_read_bit());
   return value;
}
 
bool ds18b20_read(int *raw_temp_value)
{
   if (!ds18b20_start()) // enviar pulso inicial
   return(0);
   ds18b20_write_byte(0xCC); // enviar comando pular ROM
   ds18b20_write_byte(0x44); // enviar comando de conversão de início
   while(ds18b20_read_byte() == 0); // aguarde a conclusão da conversão
   if (!ds18b20_start()) // enviar pulso inicial
       return(0); // retorna 0 se houver erro
  
   ds18b20_write_byte(0xCC); // enviar comando pular ROM
   ds18b20_write_byte(0xBE); // enviar comando de leitura
 
   *raw_temp_value = ds18b20_read_byte(); // lê o byte LSB de temperatura e armazena-o no byte LSB raw_temp_value
   *raw_temp_value |= (unsigned int)(ds18b20_read_byte() << 8); // lê o byte MSB de temperatura e armazena-o no byte MSB raw_temp_value
 
   return(1);
}
