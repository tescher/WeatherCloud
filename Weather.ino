char city_code[8] = "7263016";  // Adjust to match openweather city code Lodi, WI = 5260694, Sutton, AK = 7263016. List at http://bulk.openweathermap.org/sample/
char server[24] = "api.openweathermap.org";   // Up to 24 characters for the server name.
char APPID[33] = "6b60e46edbf44b3623b37785004e9731";  //OpenWeather API key

// Weather query config
#define FORECAST_LINE 1 //line 1 is 3 hours out, 2 is 6 hours out, etc. 
#define MAX_CONDITIONS 3 //Number of forecast conditions to accomodate. Sometimes we get more than one, and some don't make sense

// Structures to hold the weather (there can be multiple condition lines, but only one temp). Condition codes at http://openweathermap.org/weather-conditions
int conditions[MAX_CONDITIONS];
int conditionCount;
float temp;  // Temperature in Celsius

boolean get_weather() {
  conditionCount = 0;
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
    return false;
#endif
  }



  // This is NOT a generic json parser, it is very specific to the openweather response format

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
          if (bComp(key, "temp")) {
            temp = atof(value) - 273.15;
#if defined(DEBUG)
            Serial.print("Temperature: ");
            Serial.print(temp);
#endif
          }
          else if (bComp(key, "id") && conditionStarted) {
            conditions[conditionCount - 1] = atoi(value);
#if defined(DEBUG)
            Serial.print("Id ");
            Serial.print(conditionCount);
            Serial.print(": ");
            Serial.print(conditions[conditionCount - 1]);
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

  return true;
}

