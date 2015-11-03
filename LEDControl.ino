#define FADE_INTERVAL_MS 3  //milliseconds for each step of the fade sequence (full fade takes 255 * this number to complete)


// Set LED intensities for a 3-color LED or strip. "color" is 4 bits each for RGB. "0" for off, "F" for full on. For example, 
// Red is 0x0F00 (in hex), Green is 0x00F0, Blue is 0x000F, Yellow is 0x0FF0, etc.
// fade_color is the color we are fading from; fade = true turns on fading, fade = false skips it

void LED_Display(unsigned int color, unsigned int fade_color, boolean fade) {
  unsigned int red = (color >> 8) & 0xF;
  red |= (red << 4);
  unsigned int green = (color >> 4) & 0xF;
  green |= (green << 4);
  unsigned int blue = color & 0xF;
  blue |= (blue << 4);
#if defined(DEBUG)
  Serial.print("Red: ");
  Serial.println(red, HEX);
  Serial.print("Green: ");
  Serial.println(green, HEX);
  Serial.print("Blue: ");
  Serial.println(blue, HEX);
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

    for (int i = 0; i < 255; i++) {
      if (fade_red != red) fade_red += red_inc;
      if (fade_green != green) fade_green += green_inc;
      if (fade_blue != blue) fade_blue += blue_inc; \
      analogWrite(LED_RED, fade_red);
      analogWrite(LED_GREEN, fade_green);
      analogWrite(LED_BLUE, fade_blue);
      delay(FADE_INTERVAL_MS);
    }
  }
  analogWrite(LED_RED, red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}

