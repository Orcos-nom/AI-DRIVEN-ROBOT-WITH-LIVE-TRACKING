#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>

// WiFi Credentials
const char* ssid = "You want net nigga";
const char* password = "boxBOX09876";

// Web Servers
AsyncWebServer serverMPU(80);
AsyncWebServer serverGPS(8080);

// MPU6050 Sensor
Adafruit_MPU6050 mpu;
float ax, ay, az, gx, gy, gz;

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// GPS Module (UART1)
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

// I2C LCD Initialization
LiquidCrystal_I2C lcd(0x27, 12, 2);  // Address 0x27 and 12x2 LCD size

// Initialize MPU6050
void initMPU6050() {
    if (!mpu.begin()) {
        Serial.println("Failed to initialize MPU6050!");
        while (1);
    }
    Serial.println("MPU6050 initialized!");
}

// Read MPU6050 Data
void readMPU6050() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    gx = g.gyro.x * 180 / PI;
    gy = g.gyro.y * 180 / PI;
    gz = g.gyro.z * 180 / PI;
}

// Display Data on I2C LCD
void updateLCD() {
    lcd.clear();

    // Display "SKYNET" in the center (12x2 display)
    lcd.setCursor(2, 0);  // Center text at top line
    lcd.print("SKYNET");

    // Display Accelerations on Bottom Line
    lcd.setCursor(0, 1);
    lcd.print("AX:");
    lcd.print(ax, 1);
    lcd.print(" AY:");
    lcd.print(ay, 1);
    lcd.print(" AZ:");
    lcd.print(az, 1);
}

void setup() {
    Serial.begin(115200);
    Wire.begin();  // Start I2C communication

    // Initialize LCD with I2C
    lcd.init();
    lcd.backlight();  // Turn on backlight
    lcd.print("Initializing...");
    delay(1000);

    initMPU6050();  // Initialize MPU6050

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");

    Serial.print("MPU Data URL: http://");
    Serial.println(WiFi.localIP());

    Serial.print("GPS Data URL: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":8080");

    // Start NTP Client
    timeClient.begin();

    // Initialize GPS
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

    // Serve MPU6050 Data on Port 80 (Unchanged Web Page)
    serverMPU.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <title>MPU6050 Data</title>
            <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
            <script>
                let gyroChart;
                let maxDataPoints = 50;
                function initChart() {
                    const ctx = document.getElementById('gyroChart').getContext('2d');
                    gyroChart = new Chart(ctx, {
                        type: 'line',
                        data: {
                            labels: Array(maxDataPoints).fill(''),
                            datasets: [
                                { label: 'GX (°/s)', borderColor: 'red', data: [], fill: false },
                                { label: 'GY (°/s)', borderColor: 'blue', data: [], fill: false },
                                { label: 'GZ (°/s)', borderColor: 'green', data: [], fill: false }
                            ]
                        },
                        options: { responsive: true, maintainAspectRatio: false, scales: { y: { beginAtZero: false } } }
                    });
                }
                function updateData() {
                    fetch('/data')
                        .then(response => response.text())
                        .then(data => {
                            const [ax, ay, az, gx, gy, gz] = data.split(',').map(Number);
                            document.getElementById('gx').textContent = gx.toFixed(2);
                            document.getElementById('gy').textContent = gy.toFixed(2);
                            document.getElementById('gz').textContent = gz.toFixed(2);
                            gyroChart.data.datasets[0].data.push(gx);
                            gyroChart.data.datasets[1].data.push(gy);
                            gyroChart.data.datasets[2].data.push(gz);
                            if (gyroChart.data.datasets[0].data.length > maxDataPoints) {
                                gyroChart.data.datasets.forEach(dataset => dataset.data.shift());
                            }
                            gyroChart.update();
                        });
                }
                window.onload = function() {
                    initChart();
                    setInterval(updateData, 500);
                };
            </script>
            <style>
                body { text-align: center; background: #111; color: #fff; }
                h1 { font-size: 2em; margin: 20px; }
                .data-box { padding: 15px; background: #222; border-radius: 10px; font-size: 1.5em; }
                .chart-container { width: 90vw; height: 50vh; }
            </style>
        </head>
        <body>
            <h1>MPU6050 Data</h1>
            <div class="data-box">Gyroscope (°/s): X=<span id="gx">0</span>, Y=<span id="gy">0</span>, Z=<span id="gz">0</span></div>
            <div class="chart-container"><canvas id="gyroChart"></canvas></div>
        </body>
        </html>
        )rawliteral";
        request->send(200, "text/html", html);
    });

    // Serve MPU Sensor Data
    serverMPU.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        readMPU6050();
        String data = String(ax) + "," + String(ay) + "," + String(az) + "," 
                    + String(gx) + "," + String(gy) + "," + String(gz);
        request->send(200, "text/plain", data);
    });

    // Serve GPS Data on Port 8080 (Unchanged)
    serverGPS.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        float lat = gps.location.isValid() ? gps.location.lat() : 0;
        float lon = gps.location.isValid() ? gps.location.lng() : 0;
        String html = "<!DOCTYPE html><html><head><title>GPS Location</title></head><body>";
        html += "<h1>GPS Location</h1>";
        html += "<p><a href='https://www.google.com/maps/place/" + String(lat, 6) + "," + String(lon, 6) + "' target='_blank'>View on Google Maps</a></p>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    serverMPU.begin();  // Start MPU Server
    serverGPS.begin();  // Start GPS Server
}

void loop() {
    timeClient.update();
    readMPU6050();  // Read MPU6050 data
    updateLCD();    // Update LCD with acceleration values

    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }
    delay(500);
}
