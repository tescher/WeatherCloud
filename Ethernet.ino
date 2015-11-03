// Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE1 };
EthernetClient client;
int network_retry = 1;  // Number of times to try to get a network connection.

// Set up Ethernet connection

boolean start_Ethernet() {    // Call this to set up a connection
#if defined(DEBUG)
  Serial.println("Attempting to configure Ethernet...");
#endif
  int eth_retry = 0;
  while ((Ethernet.begin(mac) == 0) && eth_retry < network_retry) {
#if defined(DEBUG)
    Serial.println("Failed to configure Ethernet using DHCP");
    Serial.println("retrying...");
#endif
    delay(5000);
    eth_retry++;
  }

  if (eth_retry >= network_retry) {
    return false;
  } else {
    // give the Ethernet shield a second to initialize:
    delay(1000);
    return true;
  }

}
