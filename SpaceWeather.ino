char swserver[24] = "services.swpc.noaa.gov";   // Up to 24 characters for the server name.
int kp;

// Call this to get the current Kp index which gives the current chance for aurora. Puts the temperature in global variable "kp" 
// Returns false if it hit a problem.

boolean get_space_weather() {
#if defined(DEBUG)
  Serial.println("connecting...");
#endif

  if (client.connect(swserver, 80)) {
    client.print("GET ");
    client.print("products/noaa-estimated-planetary-k-index-1-minute.json");
    client.println(" HTTP/1.0");
    client.print("Host: ");
    client.println(swserver);
    client.println();
#if defined(DEBUG)
    Serial.print("GET ");
    Serial.print("products/noaa-estimated-planetary-k-index-1-minute.json");
    Serial.println(" HTTP/1.0");
    Serial.print("Host: ");
    Serial.println(swserver);
#endif
  }
  else {
#if defined(DEBUG)
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
    return false;
#endif
  }



  // This is NOT a generic json parser, it is very specific to the space weather response format..

  kp = 0;

  bool jsonStarted = false;  // "[" detected => true, "]" detected => false
  bool readingStarted = false; // "[" detected => true, "]" detected => false
  bool kpStarted = false; // "," detected => true, "," detected => false
  bool kpRead = false;
  
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
#if defined(DEBUG)
      Serial.print(c);
#endif
      if (!jsonStarted && (c == '[')) {
        jsonStarted = true;
      }
      else if (jsonStarted && !readingStarted && (c == '[')) {
        readingStarted = true;
      }
      else if (readingStarted && !kpStarted && (c == ',')) {
        kpStarted = true;
      }
      else if (readingStarted && kpStarted && (c != ",")) {   // Kp 
        kp = c - '0';
        kpRead = true;
        kpStarted = false;  // Have our value, reset and start looking for next "["
        readingStarted = false;
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

  if (kpRead) return true;
  return false;
}


    

