#include <Ethernet.h>

// LED Driver Pins (Ethernet shield uses 10-13 for network stuff, so don't use those)
#define LED_RED 9
#define LED_GREEN 5
#define LED_BLUE 6

// Color definitions, using 0x0RGB.
#define RED 0x0F00
#define GREEN 0x00F0
#define BLUE 0x000F
#define WHITE 0x0FFF
#define YELLOW 0x0FF0
#define GREY 0x0444
#define BLACK 0x0000
#define PURPLE 0x0F0F
#define LIGHTGREEN 0x08F8
#define LIGHTBLUE 0x088F
#define LIGHTRED 0x0F88

// Timing intervals
#define QUERY_INTERVAL_SEC 600 //seconds between queries to get the forecast
#define COLOR_INTERVAL_SEC 5 //seconds to pause at each color

// Weather interpretation
#define TEMP_HOT 22 // 72 F
#define TEMP_WARM 10 // 50 F
#define TEMP_COOL 0 // 32 F
#define TEMP_COLD -12 // 10 F
#define ALWAYS_THUNDER 0  // Whether or not to always display lightning

// Debugging config
#define DEBUG 1        // Conditional Compilation

// Variables defined in other files
extern int conditionCount;  // Number of conditions found
extern int conditions[];    // Weather conditions found
extern float temp;          // Temperature

boolean have_network = false;  // Keep track of whether we have a network

void setup() {

  // Configure the LEDs and turn them off
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  LED_Display(BLACK, BLACK, false);

  // start the serial library:
#if defined(DEBUG)
  Serial.begin(9600);
#endif

  // Start our random number generator
  randomSeed(analogRead(0));

  // Start the Ethernet
  have_network = start_Ethernet();

#if defined(DEBUG)
  if (have_network) {
    Serial.print("Ethernet configured");
  } else {
    Serial.print("Network down, using random conditions");
  }
#endif


}

void loop() {


  // Query the weather if network connected
  if (have_network) {
    
    if ((!get_weather()) || (conditionCount < 1)) {
#if defined(DEBUG)
      Serial.println("No conditions received");
#endif
    }
  }

  // If no conditions found on network, make some up
  if (conditionCount < 1) {
    for (int i = 0; i < 2; i++) {
      conditions[i] = random(199, 804);  // Just pick some weather conditions. Get two so one might be thunder
    }
    conditionCount = 2;
    temp = random(0, 30);
  }


  // Parse the weather info and set the lights
  unsigned int color1 = PURPLE;  // To start assume we are between COOL and WARM
  if (temp > TEMP_HOT) {
    color1 = RED;
  }
  else if (temp > TEMP_WARM) {
    color1 = LIGHTRED;
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
      color2 = YELLOW; // Sun or partly cloudy
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


  // Color the cloud!
  boolean display1 = false;
  if (!have_network || conditionCount < 1) {
    LED_Display(RED, RED, false); // Signal fake weather
    delay(5000);
  }
  for (int i = 0; i < QUERY_INTERVAL_SEC;) { // Fade back and forth between color1 (temperature) and color2 (sky/precip)
    if (display1) {
      LED_Display(color2, color1, true); 
      display1 = false;
    } else {
      LED_Display(color1, color2, true);
      display1 = true;
    }
    for (int j = 0; j < COLOR_INTERVAL_SEC; j++, i++) {
      delay(1000);
    }
    if (thunder) {  // Flash some lightning
      if (random(1, 100) % 8 == 0) {
        int claps = random(1, 7);
        for (int k = 0; k < claps; k++) {
          LED_Display(WHITE, WHITE, false);
          delay(random(1, 3) * 100);
          LED_Display(BLACK, BLACK, false);
          delay(random(1, 5) * 300);
        }
      }
    }
  }

  // If disconnected, try connecting again
  if (!have_network) {
    have_network = start_Ethernet();
  }

}


