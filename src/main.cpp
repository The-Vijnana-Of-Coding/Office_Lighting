#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FastLED.h>
#include <WiFi.h>

const char* ssid = "ESP32-AP";
const char* password = "password"; // Change to suit your needs!

WiFiServer server(80);

const uint8_t ledPin = 26;
const uint8_t ledButton = 19;
#define NUM_LEDS 110 // Define the number of LEDs in your strip
short brightness;
unsigned long previousMillis = 0;
const long interval = 5000; // Interval in milliseconds (1 second)
float lastTempReading = 0.0;

CRGB leds[NUM_LEDS];

#define ONE_WIRE_BUS 5 // Pin connected to DS18B20 data line

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int LCD_ADDR = 0x27;
int LCD_COLS=  20;
int LCD_ROWS = 4;
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

bool LCDBacklight = false;
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
	LCDBacklight = true;
    lcd.clear();
}

// ------ ############################################################ ------ //

void SensorUpdate()
{
    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0); // Read temperature in Celsius
	if(lastTempReading != temperatureC)
	{
		String temperatureString = String(temperatureC, 1);
		String output = "Temp: " + temperatureString + " C";
		char charArray[output.length() + 1];
		output.toCharArray(charArray, output.length() + 1);
		LCDPrint(0, end, charArray);
		lastTempReading = temperatureC;
	}
    
}

void SetColor(short r, short g, short b)
{
    // Fill the LED strip with a color (e.g., red)
    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.show();
}

void SetBrightness(short brightnessLevel)
{
    // Set the brightness level (0-255, where 0 is off and 255 is full brightness)
    FastLED.setBrightness(brightnessLevel); // Adjust this value to set the brightness level
    FastLED.show();
}

void LightsOff()
{
	Serial.println("Lights are On, switching off now");
	SetBrightness(0);
	lcd.clear();
	lcd.noBacklight();		
	LCDBacklight = false;
}

void LightsOn()
{
	Serial.println("Lights are Off, switching on now");
	SetColor(255, 0, 155);
	SetBrightness(255);
	lcd.backlight(); // Turn on the backlight
	LCDBacklight = true;
	delay(100);
	SensorUpdate();
}

void toggleLights() {
    if (FastLED.getBrightness() > 0) {
        LightsOff();
    } else {
        LightsOn();
    }
}

// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
// ------ ############################################################ ------ //

void setup() {
    Wire.begin();
    Serial.begin(115200);

	LCDStart();
	LCDBootScreen();

	// Set up WiFi access point
	WiFi.softAP(ssid, password);
	
	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);
	
	server.begin();

    sensors.begin(); 

	delay(1000);
	
	pinMode(ledButton, INPUT_PULLUP);
	FastLED.addLeds<WS2812B, ledPin, GRB>(leds, NUM_LEDS);
	LightsOff();
}

// ------ ############################################################ ------ //

void loop() {
	// Wait for a client to connect
	WiFiClient client = server.available();
	
	if (client) {
		Serial.println("Client connected");
		
		// Send data to the client
		client.print("Hello from ESP32 AP");
		
		// Close the connection
		client.stop();
		Serial.println("Client disconnected");
	}
	
	unsigned long currentMillis = millis();

    // Check if it's time to update the sensor readings
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
		if(LCDBacklight)
		{
        	SensorUpdate();
		}
    }

    // Check if the LED button is pressed
    if (digitalRead(ledButton) == LOW) {
        toggleLights();
        delay(250); // Add a small delay to debounce the button
    }
}

// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
// ------ ############################################################ ------ //