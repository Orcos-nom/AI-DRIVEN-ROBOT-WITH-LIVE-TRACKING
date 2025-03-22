#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// WiFi Credentials
const char* ssid = "You want net nigga";
const char* password = "boxBOX09876";

// NTP Time Sync
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// GPS Module (UART1)
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);  // Neo-6M on TX=17, RX=16

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi Connected!");

    // Start NTP Client
    timeClient.begin();
}

void loop() {
    // Step 1: Get Time from NTP and Send to GPS
    timeClient.update();
    
    int hh = timeClient.getHours();
    int mm = timeClient.getMinutes();
    int ss = timeClient.getSeconds();
    int dd = 9;   // Example Day
    int mo = 3;   // Example Month (March)
    int yyyy = 2025; // Example Year

    char gpzda[50];
    sprintf(gpzda, "$GPZDA,%02d%02d%02d,%02d,%02d,%04d,00", hh, mm, ss, dd, mo, yyyy);
    
    Serial.println(gpzda);  // Print for Debugging
    gpsSerial.println(gpzda);  // Send to Neo-6M GPS

    // Step 2: Read GPS Data & Generate Google Maps Link
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
        if (gps.location.isUpdated()) {
            float lat = gps.location.lat();
            float lon = gps.location.lng();
            Serial.printf("Lat: %.6f, Lon: %.6f\n", lat, lon);
            
            // Create Google Maps Link
            String map_url = "https://www.google.com/maps/place/" + String(lat, 6) + "," + String(lon, 6);
            Serial.println("Google Maps Link: " + map_url);
            
            delay(5000);  // Update every 5 sec
        }
    }
    
    delay(60000);  // Update NTP time every 1 min
}
