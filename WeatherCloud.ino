
#include <SPI.h>
#include <Ethernet.h>
#include <stdlib.h>
#include <avr/wdt.h>

// LED Driver Pins
#define LED_RED 9
#define LED_GREEN 10
#define LED_BLUE 6

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
#define LIGHTBLUE 0x088F
#define LIGHTRED 0x0F88

// Timing intervals
#define FADE_INTERVAL_MS 10  //milliseconds 
#define COLOR_INTERVAL_SEC 5 //seconds
#define QUERY_INTERVAL_SEC 600 //seconds

// Weather query config
#define FORECAST_LINE 1 //line 1 is 3 hours out, 2 is 6 hours out, etc. 
#define MAX_CONDITIONS 3 //Number of forecast conditions to accomodate
#define TEMP_HOT 295 // 71 F
#define TEMP_WARM 283 // 50 F
#define TEMP_COOL 273 // 32 F
#define TEMP_COLD 261 // 10 F
#define ALWAYS_THUNDER 0  // Whether or not to always display lightning
char city_code[8] = "7263016";  // Adjust to match openweather city code Lodi, WI = 5260694, Sutton, AK = 7263016
char server[24] = "api.openweathermap.org";   // Up to 24 characters for the server name. 
char APPID[33] = "6b60e46edbf44b3623b37785004e9731";  //OpenWeather API key

// Watchdog and debugging config
#define WD_INTERVAL 500  //Milliseconds between each Watchdog Timer reset
#define DEBUG 1        // Conditional Compilation

// Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE1 };
EthernetClient client;
boolean have_network = false;

// Structures to hold the weather (there can be multiple condition lines, but only one temp)
int conditions[MAX_CONDITIONS];
float temp;

// Store the previous color to allow fade to work
unsigned int prev_color;

// Utility functions

// Byte array string compare

bool bComp(char* a1, char* a2) {
  for(int i=0; ; i++) {
    if ((a1[i]==0x00)&&(a2[i]==0x00)) return true;
    if(a1[i]!=a2[i]) return false;
  }
}

// Set up Ethernet connection

boolean start_Ethernet() {
  // start the Ethernet connection, try up to 5 times, otherwise lock up and let WD reset
  #if defined(DEBUG)
  Serial.println("Attempting to configure Ethernet...");
  #endif
  int eth_retry = 0;   // Going to retry twice
  while ((Ethernet.begin(mac) == 0) && eth_retry < 2) {
    #if defined(DEBUG)
    Serial.println("Failed to configure Ethernet using DHCP");
    Serial.println("retrying...");
    #endif
    delay(5000);
    eth_retry++;
  }
  
  if (eth_retry >= 2) {
    return false;
  } else {
    // give the Ethernet shield a second to initialize:
    delay(1000);
    return true;
  }  
  
}

// Set LED intensities. "color" is 4 bits each for RGB. Duplicate to get a full byte
void LED_Display(unsigned int color, unsigned int fade_color, boolean fade) {
  unsigned int red = (color >> 8) & 0xF;
  red |= (red << 4);
  unsigned int green = (color >> 4) & 0xF;
  green |= (green << 4);
  unsigned int blue = color & 0xF;
  blue |= (blue << 4);
  #if defined(DEBUG)
  Serial.print("Red: ");
  Serial.println(red,HEX);
  Serial.print("Green: ");
  Serial.println(green,HEX);
  Serial.print("Blue: ");
  Serial.println(blue,HEX);
  #endif
  if (fade) {
    unsigned int fade_red = (fade_color >> 8) & 0xF;
    fade_red |= (fade_red << 4);
    unsigned int fade_green = (fade_color >> 4) & 0xF;
    fade_green |= (fade_green << 4);
    unsigned int fade_blue = fade_color & 0xF;
    fade_blue |= (fade_blue << 4);
 
    int red_inc = 0;
    if (fade_red > red) red_inc = -1;
    else if (fade_red < red) red_inc = 1;
    int green_inc = 0;
    if (fade_green > green) green_inc = -1;
    else if (fade_green < green) green_inc = 1;
    int blue_inc = 0;
    if (fade_blue > blue) blue_inc = -1;
    else if (fade_blue < blue) blue_inc = 1;
    
    for (int i=0; i<255; i++) {
      if (fade_red != red) fade_red += red_inc;
      if (fade_green != green) fade_green += green_inc;
      if (fade_blue != blue) fade_blue += blue_inc;\
      analogWrite(LED_RED,fade_red);
      analogWrite(LED_GREEN, fade_green);
      analogWrite(LED_BLUE, fade_blue);
      delay(FADE_INTERVAL_MS);
    }
  }
  analogWrite(LED_RED,red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}

void setup() {

  // Configure the LEDs and turn them off
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  LED_Display(BLACK,BLACK,false);

  // start the serial library:
  #if defined(DEBUG)
  Serial.begin(9600);
  #endif  
  
  randomSeed(analogRead(0));

  // Start the Ethernet
  have_network = start_Ethernet();
  
  #if defined(DEBUG)
  Serial.print("Ethernet configured");
  #endif
  

}

void loop() {

  
  // Query the weather if network connected
  if (have_network) {
    #if defined(DEBUG)
    Serial.println("connecting...");
    #endif
  
    if (client.connect(server, 80)) {
      client.print("GET ");
      client.print("/data/2.5/forecast/?id=");
      client.print(city_code);
      client.print("&APPID=");
      client.print(APPID);
      client.println(" HTTP/1.0");
      client.print("Host: ");
      client.println(server);
      client.println();
      #if defined(DEBUG)
      Serial.print("GET ");
      Serial.print("/data/2.5/forecast/?id=");
      Serial.print(city_code);
      Serial.print("&APPID=");
      Serial.print(APPID);
      Serial.println(" HTTP/1.0");
      Serial.print("Host: ");
      Serial.println(server);
      #endif
    } 
    else {
      #if defined(DEBUG)
      // if you didn't get a connection to the server:
      Serial.println("connection failed");
      have_network = false;
      #endif
    }
  }
  
  int conditionCount = 0;
  
  // This is NOT a generic json parser, it is very specific to the openweather response format
  if (have_network) {
    int listCount = 0;
    temp = 0;
  
    bool jsonStarted = false;
    bool listStarted = false;
    bool conditionStarted = false;
    bool keyStarted = false;
    bool haveKey = false;
    bool valueStarted = false;
    bool stringStarted = false;
    bool numStarted = false;
    int i = 0;
    char key[20];
    char value[20];
    while (client.connected()) {
      if (client.available()) {
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
          haveKey = false;   // Ignore "weather" key
          keyStarted = false;
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
        else if (haveKey && !valueStarted && ((c == '{') || (c == '['))) {   // Starting a new level, reset
          haveKey = false;
          keyStarted = false;
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
            if (bComp(key,"temp")) {
              temp = atof(value);
              #if defined(DEBUG)
              Serial.print("Temperature: ");
              Serial.print(temp);
              #endif
            }
            else if (bComp(key,"id") && conditionStarted) {
              conditions[conditionCount-1] = atoi(value);
              #if defined(DEBUG)
              Serial.print("Id ");
              Serial.print(conditionCount);
              Serial.print(": ");
              Serial.print(conditions[conditionCount-1]);
              #endif
            }
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
        #if defined(DEBUG)
        Serial.println("No more data, waiting for server to disconnect");
        #endif
        delay(1000);
      }
    }
  
    while (client.available()) {
      char c = client.read();  //Just clean up anything left
      #if defined(DEBUG)
      Serial.print(c);
      #endif
    }
    
    client.stop();
    delay(1000);
    
    if (conditionCount < 1) {
      #if defined(DEBUG)
      Serial.println("No conditions received");
      #endif
    }
  }
 
  // If no conditions found on network, make some up
  if (conditionCount < 1) {
    for (int i=0; i< MAX_CONDITIONS; i++) {
      conditions[i] = random(199, 804);
    }
    temp = random(200, 300);
  }  

 
  // Parse the weather info and set the lights
  unsigned int color1 = PURPLE;
  if (temp > TEMP_HOT) {
    color1 = RED;
  }
  else if (temp > TEMP_WARM) {
    color1 = LIGHTRED;
  }  
  else if (temp == 0) {
    color1 = BLACK;
  }
  else if (temp < TEMP_COOL) {
    color1 = LIGHTBLUE;
  }  
  else if (temp < TEMP_COLD) {
    color1 = BLUE;
  }
  
  unsigned int color2 = BLACK;
  boolean thunder = false;
  for (int i = 0; i < conditionCount; i++) {
    #if defined(DEBUG)
    Serial.print("Condition ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(conditions[i]);
    #endif
    if ((color2 == 0) && (conditions[i] > 499) && (conditions[i] < 600)) {
      color2 = GREEN; // Rain
    }
    if ((color2 == 0) && (conditions[i] > 599) && (conditions[i] < 700)) {
      color2 = WHITE; // Snow
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
    if ((conditions[i] > 199) && (conditions[i] < 300) || ALWAYS_THUNDER) {
      thunder = true; // Thunder
    }
  }

  
  
  #if defined(DEBUG)
  Serial.println("");
  Serial.print("Color1: ");
  Serial.println(color1);
  Serial.print("Color2: ");
  Serial.println(color2);
  Serial.print("Thunder: ");
  Serial.println(thunder);
  #endif

  // Testing
  // thunder = true;
  
  // Color the cloud!
  boolean display1 = false;
  if (!have_network || conditionCount < 1) {
    LED_Display(RED,RED,false);  // Signal fake weather
    delay(2000);
  }  
  for (int i=0; i < QUERY_INTERVAL_SEC;) {
    if (display1) {
        LED_Display(color2, color1, true);
        display1 = false;
    } else {
        LED_Display(color1, color2, true);
        display1 = true;
    }
    for (int j=0; j < COLOR_INTERVAL_SEC; j++,i++) {
       delay(1000);
    }
    if (thunder) {
      if (random(1,100) % 8 == 0) {
        int claps = random(1,7);
        // unsigned int base_color = (display1) ? color1 : color2;
        unsigned int base_color = BLACK;
        for (int k=0; k < claps; k++) {
          LED_Display(WHITE,WHITE,false);
          delay(random(1,3) * 100);
          LED_Display(base_color, base_color,false);
          delay(random(1,5) * 300);
        }
      }
    }
  }
  
    // If disconnected, try connecting again
  if (!have_network) {
    have_network = start_Ethernet();
  }

}


