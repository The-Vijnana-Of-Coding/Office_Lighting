#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 5 // Pin connected to DS18B20 data line

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int LCD_ADDR = 0x27;
int LCD_COLS=  20;
int LCD_ROWS = 4;
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

enum START_ROW
{
	beginning,
	middle,
	end,
	other
};

// ------ ############################################################ ------ //

void LCDPrint(uint8_t line, uint8_t textStart, const char* text)
{
	uint8_t textLength = strlen(text);
	uint8_t screenLength = LCD_COLS;
	uint8_t startPosition = 0;

	lcd.setCursor(0, line);
	lcd.print("                    ");

	if(textStart == beginning)
	{
		lcd.setCursor(0, line);
    	lcd.print(text);
	}
	else if(textStart == middle)
	{
		// Calculate start position to center text
		startPosition = (screenLength - textLength) / 2;
		lcd.setCursor(startPosition, line);	
    	lcd.print(text);
	}
	else if(textStart == end)
	{
		startPosition = screenLength - textLength;
		lcd.setCursor(startPosition, line);	
    	lcd.print(text);
	}
	else if(textStart == other)
	{

	}
}

// ------ ############################################################ ------ //

void LCDBootScreen()
{
	LCDPrint(1, middle, "...Booting...");
	LCDPrint(2, middle, "Please Wait");
}

// ------ ############################################################ ------ //

void LCDStart()
{
	lcd.init();
    lcd.backlight();
    lcd.clear();
}

// ------ ############################################################ ------ //

void SensorUpdate()
{
    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0); // Read temperature in Celsius

    String temperatureString = String(temperatureC, 1);
    String output = "Temp: " + temperatureString + " C";
    char charArray[output.length() + 1];
    output.toCharArray(charArray, output.length() + 1);
    LCDPrint(0, end, charArray);
}

// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
// ------ ############################################################ ------ //


void setup() {
    Wire.begin();
    Serial.begin(115200);

	LCDStart();
	LCDBootScreen();

    sensors.begin(); 

	delay(1000);
    lcd.clear();
}

// ------ ############################################################ ------ //


void loop() {
	SensorUpdate();
    delay(1000); 
}

// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
// ------ ############################################################ ------ //