#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FastLED.h>
#include <WiFi.h>
#include <RTClib.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <time.h>

#include "confi_cred.h"

const char* ntpServer = "pool.ntp.org";
const char* timeZone = "GMT0BST,M3.5.0/01,M10.5.0/02"; // Example for Europe/London time zone

const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

WebServer server(80);

const int RELAYCH1 = 16;

RTC_DS3231 rtc;
const long RTCinterval = 5000; // Interval in milliseconds (1 second)
unsigned long previousRTCMillis = 0;
String lastTimeDisplayed = "";
String lastAlarmDisplayed = "";

bool alarmState = false;
int alarmStartFlag = 0;

#ifndef credentials // create a credentials.h file with your ssid u/name and p/word
	const char* ssid = "your-ssid";
	const char* password = "your-password";
#endif

const int ledButton = 19;
const int udpPort = 12345; // UDP port used for communication
IPAddress broadcastIP(255, 255, 255, 255); // Broadcast IP address

WiFiUDP udp;

int buttonState = HIGH; // Initialize button state

const uint8_t ledPin = 26;
#define NUM_LEDS 110 // Define the number of LEDs in your strip
short brightness;
unsigned long previousMillis = 0;
const long interval = 5000; // Interval in milliseconds (1 second)
float lastTempReading = 0.0;

CRGB leds[NUM_LEDS];

#define ONE_WIRE_BUS 5 // Pin connected to DS18B20 data line

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long previousLCDMillis = 0;
const long LCDinterval = 5000; // Interval in milliseconds (1 second)
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

byte alarmBell[8] = {
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B11111,
  B00000,
  B00100
};

// ------ ############################################################ ------ //

void handleRoot() {
  server.send(200, "text/html", "<html><head><script>function setAlarm() { var hour = document.getElementById('hour').value; var minute = document.getElementById('minute').value; var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { alert('Alarm Set Successfully'); } }; xhttp.open('GET', '/set-alarm?hour=' + hour + '&minute=' + minute, true); xhttp.send(); }</script></head><body><h2>Set Alarm Time:</h2><br>Hour: <input type='number' id='hour' min='0' max='23'><br>Minute: <input type='number' id='minute' min='0' max='59'><br><button onclick='setAlarm()'>Set Alarm</button></body></html>");
}

void handleSetAlarm() {
  int hours = server.arg("hour").toInt();
  int minutes = server.arg("minute").toInt();
  DateTime alarmTime = DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), hours, minutes, 0);
  rtc.setAlarm1(alarmTime, DS3231_A1_Hour);
  
  // Store alarm time in EEPROM
  EEPROM.write(0, hours);
  EEPROM.write(1, minutes);
  EEPROM.commit();
  
  server.send(200, "text/plain", "Alarm Set Successfully");
}


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

void TimeUpdate()
{
    DateTime now = rtc.now();
    
    // Format hours, minutes, and seconds with leading zeros
    String hourString = String(now.hour());
    String minuteString = String(now.minute());
    String secondString = String(now.second());
    
    if (now.hour() < 10) {
        hourString = "0" + hourString;
    }
    if (now.minute() < 10) {
        minuteString = "0" + minuteString;
    }
    if (now.second() < 10) {
        secondString = "0" + secondString;
    }
    
    String timeString = hourString + ":" + minuteString + ":" + secondString;

    // Check if the time has changed since the last update
    if (timeString != lastTimeDisplayed) {
        char timeCharArray[timeString.length() + 1];
        timeString.toCharArray(timeCharArray, timeString.length() + 1);
        LCDPrint(2, middle, timeCharArray);

        // Update the last displayed time
        lastTimeDisplayed = timeString;
    }

	// Get the next alarm time
    DateTime nextAlarmTime = rtc.getAlarm1();
    String alarmTimeStringH = String(nextAlarmTime.hour());
    String alarmTimeStringM = String(nextAlarmTime.minute());
	
    if (nextAlarmTime.hour() < 10) {
        alarmTimeStringH = "0" + alarmTimeStringH;
    }
    if (nextAlarmTime.minute() < 10) {
        alarmTimeStringM = "0" + alarmTimeStringM;
    }

    String alarmTimeString = " " + alarmTimeStringH + ":" + alarmTimeStringM;

	String amPm = (nextAlarmTime.hour() < 12) ? "AM" : "PM";

	String alarmText = alarmTimeString + amPm;

	// Check if the alarm time has changed since the last update
    if (alarmText != lastAlarmDisplayed) {
        char timeCharArray[timeString.length() + 1];
        LCDPrint(3, beginning, alarmText.c_str());

		lcd.createChar(0, alarmBell); // Define custom character at position 0
		lcd.setCursor(0, 3);
		lcd.write((byte)0); // Display the custom character (alarm bell)
        lastAlarmDisplayed = alarmText;
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
	LCDBacklight = false;
	lcd.clear();
	lcd.noBacklight();		
	lastTempReading = 0.0;
	lastAlarmDisplayed = "";
}

void LightsOn()
{
	Serial.println("Lights are Off, switching on now");
	LCDBacklight = true;
	SetColor(255, 0, 155);
	SetBrightness(255);
	lcd.backlight(); // Turn on the backlight
	delay(100);
	lastTempReading = 0.0;
	SensorUpdate();
	previousLCDMillis = millis();
}

void BroadcastButtonState(int state) {
    // Broadcast button state to other devices
    udp.beginPacket(broadcastIP, 12345);
    udp.write(state);
    udp.endPacket();
}

void ToggleLights() {
    if (FastLED.getBrightness() > 0) {
        LightsOff();
    	BroadcastButtonState(0);
    } else {
        LightsOn();
    	BroadcastButtonState(1);
    }
}

void toggleLightsUDP(int state) {
    if (state) {
        LightsOff();
    } else {
        LightsOn();
    }
}

void SunriseAlarm() {
	static int transitionTime = 60 * 1000 * 10;  // 10 minutes in milliseconds
	static CRGB startColor(155, 0, 0);  // Burnt Orange
	static CRGB endColor(255, 255, 0);   // Bright Yellow

	unsigned long currentTime = millis();

	// Check if the transition is complete
	if (currentTime - alarmStartFlag >= transitionTime) {
		// Transition is complete, set a flag or perform any post-transition actions
		alarmState = false;
	} else {
		// Calculate progress
		float progress = float(currentTime - alarmStartFlag) / float(transitionTime);

		// Calculate the current brightness (0 to 100)
		int currentBrightness = int(progress * 100);

		// Calculate the current color
		CRGB currentColor = CRGB(
		startColor.r + progress * (endColor.r - startColor.r),
		startColor.g + progress * (endColor.g - startColor.g),
		startColor.b + progress * (endColor.b - startColor.b)
		);

		FastLED.setBrightness(currentBrightness);  // Set the brightness
		fill_solid(leds, NUM_LEDS, currentColor);
		FastLED.show();
	}
}

// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
void printDateTime(const DateTime& dt) {
  char dateTimeString[20];
  sprintf(dateTimeString, "%04d-%02d-%02d %02d:%02d:%02d", dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  Serial.println(dateTimeString);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(); // Initialize the Wire interface

	LCDStart();
	LCDBootScreen();

    // Connect to WiFi network
    Serial.println();
    Serial.println("Setting up Access Point...");
    WiFi.begin(ssid, password); // Connect to your existing WiFi router
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Connecting to WiFi...");
	}
	Serial.print("WiFi connected, IP address: ");
  	Serial.println(WiFi.localIP()); // Print the obtained IP address
  	server.on("/", HTTP_GET, handleRoot);
	server.on("/set-alarm", HTTP_GET, handleSetAlarm);
	server.begin();
	Serial.println("Server started");

    // Begin UDP communication
    udp.begin(12345); // Choose a UDP port

	delay(1000);

    // Initialize RTC
	rtc.begin();

	  // Configure time synchronization with NTP server and set time zone
  configTzTime(timeZone, ntpServer);

  // Wait for time synchronization
  while (!time(nullptr)) {
    Serial.println("Waiting for time synchronization...");
    delay(1000);
  }

  // Obtain the current time from the NTP server
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time from NTP server");
    return;
  }

  // Set the RTC time using the obtained local time
  rtc.adjust(DateTime(timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
                      timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec));
  
  Serial.println("RTC time adjusted successfully");


	// Initialize EEPROM
	EEPROM.begin(2); // Reserve 2 bytes for alarm time (hour and minute)
	
	// Read alarm time from EEPROM
	int hours = EEPROM.read(0);
	int minutes = EEPROM.read(1);
	// Set the alarm time
	DateTime alarmTime = DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), hours, minutes, 0);
	rtc.setAlarm1(alarmTime, DS3231_A1_Hour);

    sensors.begin(); 

	delay(1000);
	
	pinMode(ledButton, INPUT_PULLUP);
	FastLED.addLeds<WS2812B, ledPin, GRB>(leds, NUM_LEDS);
	LightsOff();

	pinMode(RELAYCH1, OUTPUT);
}

// ------ ############################################################ ------ //

void loop() {
	server.handleClient();

	unsigned long currentMillis = millis();
	int newButtonState = digitalRead(ledButton);
	
	// Check if alarm has triggered
	if (rtc.alarmFired(1)) {
		alarmState = true;
		alarmStartFlag = millis();
		rtc.clearAlarm(1); // Clear the alarm flag
	}

	if(alarmState)
	{
		SunriseAlarm();
	} 
	
	if (newButtonState == LOW) {
        // Button state has changed
        buttonState = newButtonState;
		if(alarmState)
		{
			alarmState = false;
		} 
		else
		{
        	ToggleLights();
		}
        delay(250); // Debounce the button
    }
    
	//digitalWrite(RELAYCH1, !digitalRead(RELAYCH1));
	if(LCDBacklight)
	{
		if (currentMillis - previousMillis >= interval) {
        previousMillis = millis();
			SensorUpdate();
    	}
		TimeUpdate();
	}
    

	// Check if there are any UDP packets available
    int packetSize = udp.parsePacket();
    if (packetSize) {
        // Read the packet into a buffer
        char packetBuffer[255];
        int len = udp.read(packetBuffer, 255);
        if (len > 0) {
            packetBuffer[len] = 0;
        }
        // Print the received packet
        Serial.print("Received packet: ");
        Serial.println(packetBuffer);
        
        toggleLightsUDP(packetBuffer[0]);
    }
}

// ------ ############################################################ ------ //
// ------ ############################################################ ------ //
// ------ ############################################################ ------ //