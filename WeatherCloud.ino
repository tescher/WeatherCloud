
#include <SPI.h>
#include <Ethernet.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include <EEPROM.h>   ;*TWE++ 4/8/12 Read config from EEPROM rather than hard coded

// LED Driver Pins
#define LED_RED 9
#define LED_GREEN 10
#define LED_BLUE 11

// Color definitions, using 0x0RGB
#define RED 0x0F00
#define GREEN 0x00F0
#define BLUE 0x000F
#define WHITE 0x0FFF
#define YELLOW 0x0FF0
#define GREY 0x0888
#define BLACK 0x0000
#define PURPLE 0x0F0F
#define LIGHTGREEN 0x08F8

// Timing intervals
#define FADE_INTERVAL_MS 2000  //milliseconds (NOT USED CURRENTLY)
#define COLOR_INTERVAL_SEC 5 //seconds
#define QUERY_INTERVAL_SEC 600 //seconds

// Weather query config
#define FORECAST_LINE 1 //line 1 is 3 hours out, 2 is 6 hours out, etc. 
#define MAX_CONDITIONS 3 //Number of forecast conditions to accomodate
#define TEMP_HOT 295 // 71 F
#define TEMP_COLD 275 // 35 F
char city_code[8] = "7263016";  // Adjust to match openweather city code
char server[24] = "api.openweathermap.org";   // Up to 24 characters for the server name. 

// Watchdog and debugging config
#define WD_INTERVAL 500  //Milliseconds between each Watchdog Timer reset
#define CODE_LOG_LOC 200   //Location in the EEPROM where to store the current execution point
#define DEBUG         // Conditional Compilation
int last_code_log = 255;
byte current_code_log = 255;

// Network
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x30, 0x7D };
EthernetClient client;

// Structures to hold the weather (there can be multiple condition lines, but only one temp)
int conditions[MAX_CONDITIONS];
float temp;

// Utility functions

// Byte array string compare

bool bComp(char* a1, char* a2) {
  for(int i=0; ; i++) {
    if ((a1[i]==0x00)&&(a2[i]==0x00)) return true;
    if(a1[i]!=a2[i]) return false;
  }
}

// Delay function with watchdog resets

void delay_with_wd(int ms) {
  int loop_count = ms / WD_INTERVAL;
  int leftover = ms % WD_INTERVAL;

  for (int i=0; i < loop_count; i++) {
    delay(WD_INTERVAL);
    wdt_reset();
  }
  delay(leftover);
  wdt_reset();
}

// Initialize watchdog

void WDT_Init(void) {
  //disable interrupts
  cli();
  //reset watchdog
  wdt_reset();
  //set up WDT interrupt
  WDTCSR = (1<<WDCE)|(1<<WDE);
  //Start watchdog timer with 8s timeout, interrupt and system reset modes
  WDTCSR = (1<<WDIE)|(1<<WDE)|(1<<WDP3)|(1<<WDP0);
  //Enable global interrupts
  sei();
}

// WD Interrupt Vector - save log to EEPROM

ISR(WDT_vect) {
  EEPROM.write(CODE_LOG_LOC, current_code_log);
  while(1);
}
 
// Store a marker where we are so when we restart we can log where we hung up

void code_log(byte location) {
  current_code_log = location;
  // EEPROM.write(CODE_LOG_LOC, location);
}

int get_code_log() {
  return (EEPROM.read(CODE_LOG_LOC) & 0x00FF);
}

// Set up Ethernet connection

boolean start_Ethernet() {
  // start the Ethernet connection, try up to 5 times, otherwise lock up and let WD reset
#if defined(DEBUG)
  Serial.println("Attempting to configure Ethernet...");
#endif
  code_log(1);
  int eth_retry = 0;
  while ((Ethernet.begin(mac) == 0) && (eth_retry < 6)) {
#if defined(DEBUG)
    Serial.println("Failed to configure Ethernet using DHCP");
    Serial.println("retrying...");
#endif
    delay_with_wd(5000);
    eth_retry++;
  }
  if (eth_retry > 5) {
    code_log(24);
    for (;;)
      ;
   }
  
  // give the Ethernet shield a second to initialize:
  delay_with_wd(1000);
}

// Set LED intensities. "color" is 4 bits each for RGB. Duplicate to get a full byte
void LED_Display(unsigned int color) {
  int red = color >> 8 & 0xF;
  red |= red << 4;
  int green = color >> 4 & 0xF;
  green |= green << 4;
  int blue = color & 0xF;
  blue |= blue << 4;
  
  analogWrite(LED_RED,red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}
  
  

 
  

void setup() {
  // Find out where we crashed
  last_code_log = get_code_log();

  // start the serial library:
#if defined(DEBUG)
  Serial.begin(9600);
#endif  

  // Start the watchdog
  WDT_Init();
  
  // Start the Ethernet
  start_Ethernet();
  
  // Configure the LEDs and turn them off
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  LED_Display(BLACK);

}

void loop() {


#if defined(DEBUG)
  // Query the weather
  Serial.println("connecting...");
#endif

  int i = 0;

  if (client.connect(server, 80)) {
    code_log(2);
    // Serial.println("connected");

    client.print("GET ");
    client.print("/data/2.5/forecast/?id=");
    client.print(city_code);
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(server);
    client.println();
#if defined(DEBUG)
    Serial.print("GET ");
    Serial.print("/data/2.5/forecast/?id=");
    Serial.print(city_code);
    Serial.println(" HTTP/1.0");
    Serial.print("Host: ");
    Serial.println(server);
#endif
  } 
  else {
    code_log(3);
#if defined(DEBUG)
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
#endif
    for(;;)
      ;
  }
  
  // This is NOT a generic json parser, it is very specific to the openweather response format

  bool jsonStarted = false;
  bool listStarted = false;
  int listCount = 0;
  bool conditionStarted = false;
  int conditionCount = 0;
  bool keyStarted = false;
  bool haveKey = false;
  bool valueStarted = false;
  bool stringStarted = false;
  bool numStarted = false;
  char key[20];
  char value[20];
  while (client.connected()) {
    code_log(4);
    if (client.available()) {
      code_log(5);
      char c = client.read();
#if defined(DEBUG)
      Serial.print(c);
#endif
      if (!jsonStarted && (c == '{')) {
        jsonStarted = true;
      } 
      else if (jsonStarted && !listStarted && (c == '[')) {
        listStarted = true;
        listCount++;
        if (listCount != FORECAST_LINE) listStarted = false;  // Not our line, get out
      } 
      else if (listStarted && !conditionStarted && (c == '[')) {
        conditionStarted = true;
        conditionCount++;
        if (conditionCount > MAX_CONDITIONS) break;  // Got all our conditions, get out
      }
      else if (listStarted && !keyStarted && (c == '"')) {
        keyStarted = true;
        i = 0;
#if defined(DEBUG)
        Serial.println("keyStart");
#endif
      } 
      else if (keyStarted && !haveKey && (c != '"')) {
        key[i++] = c;
      } 
      else if (keyStarted && !haveKey && (c == '"')) {
        haveKey = true;
        key[i] = 0x00;
#if defined(DEBUG)
        Serial.println("haveKey");
#endif
      } 
      else if (haveKey && !valueStarted && (c != ' ') && (c != ':')) {
        valueStarted = true;
#if defined(DEBUG)
        Serial.println("valueStart");
#endif
        if (c != '"') {
          numStarted = true;
          i = 0;
          value[i++] = c;
        } 
        else {
          stringStarted = true;
          i = 0;
        }
      } 
      else if (valueStarted) {
        if ((stringStarted && (c == '"')) || (numStarted && ((c == ',') || (c == '}')))) {
          value[i] = 0x00;
          valueStarted = false;
          haveKey = false;
          stringStarted = false;
          // if (numStarted && (c == '}')) valueStarted = false;
          numStarted = false;
          keyStarted = false;
#if defined(DEBUG)
          Serial.println("valueDone");
#endif
          if (bComp(key,"temp")) temp = atof(value);
          else if (bComp(key,"id") && conditionStarted) conditions[conditionCount-1] = atoi(value);
        }
        else value[i++] = c;
      } 
      else if (conditionStarted & !keyStarted && (c == ']')) {
        conditionStarted = false;
      } 
      else if (listStarted && !keyStarted && (c == ']')) {
        listStarted = false;
      }
    } 
    else {
      code_log(6);
#if defined(DEBUG)
      Serial.println("No more data, waiting for server to disconnect");
#endif
      delay_with_wd(1000);
    }
  }

  while (client.available()) {
    code_log(7);
    char c = client.read();  //Just clean up anything left
#if defined(DEBUG)
    Serial.print(c);
#endif
  }
  
  code_log(8);
  client.stop();
  delay_with_wd(1000);
  
  if (conditionCount < 1) {
    code_log(9);
#if defined(DEBUG)
    Serial.println("No conditions received");
#endif
        for(;;)
      ;
  }
 

  // Parse the weather info and set the lights
  
  unsigned int color1 = PURPLE;
  if (temp > TEMP_HOT) {
    color1 = RED;
  }
  else if (temp < TEMP_COLD) {
    color1 = BLUE;
  }
  
  unsigned int color2 = 0;
  boolean thunder = false;
  for (int i = 0; i++; i < conditionCount) {
    if ((color2 == 0) && (conditions[i] > 499) && (conditions[i] < 600)) {
      color2 = GREEN; // Rain
    }
    if ((color2 == 0) && (conditions[i] > 599) && (conditions[i] < 700)) {
      color2 = BLUE; // Snow
    }
    if ((color2 == 0) && (conditions[i] > 799) && (conditions[i] < 804)) {
      color2 = YELLOW; // Sun
    }
    if ((color2 == 0) && (conditions[i] == 804)) {
      color2 = GREY; // Clouds
    }
    if ((color2 == 0) && (conditions[i] > 299) && (conditions[i] < 400)) {
      color2 = LIGHTGREEN; // Drizzle
    }
    if ((conditions[i] > 199) && (conditions[i] < 300)) {
      thunder == false; // Thunder
    }
  }

  
  
#if defined(DEBUG)
  Serial.println("");
  Serial.print("Color1: ");
  Serial.println(color1);
  Serial.print("Color2: ");
  Serial.print(color2);
  Serial.print("Thunder: ");
  Serial.println(thunder);
#endif

  code_log(13);
  for (int i=0; i < QUERY_INTERVAL_SEC;) {
    boolean display1 = false;
    for (int j=0; j < COLOR_INTERVAL_SEC; j++,i++) {
      if (display1) {
        LED_Display(color2);
      } else {
        LED_Display(color1);
      }
      delay_with_wd(1000);
    }
  }
}


