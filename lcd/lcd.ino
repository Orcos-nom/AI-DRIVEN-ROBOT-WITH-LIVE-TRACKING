#include <Wire.h>
#include <U8g2lib.h>

// Initialize the OLED display for I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  u8g2.begin();  // Start communication
}

void loop() {
  u8g2.clearBuffer();                  // Clear internal buffer
  u8g2.setFont(u8g2_font_6x10_tf);     // Choose a font
  u8g2.drawStr(0, 10, "Hello World!"); // Draw a string
  u8g2.sendBuffer();                   // Transfer buffer to display
  delay(1000);
}
