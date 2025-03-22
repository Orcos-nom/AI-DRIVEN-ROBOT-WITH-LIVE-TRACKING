#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

// LCD Display (128x64 I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

// Initialize LCD Display
void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Corrected line
        Serial.println("SSD1306 allocation failed!");
        while (1);
    }
    display.clearDisplay();
    display.display();
}


// Update LCD Display with MPU Data
void updateDisplay() {
    display.clearDisplay();
    
    // Display Title "SKYNET"
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor((SCREEN_WIDTH - (6 * 6 * 2)) / 2, 0);
    display.println("SKYNET");

    // Display MPU6050 Data
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.printf("GX: %.2f d/s\n", gx);
    display.setCursor(0, 30);
    display.printf("GY: %.2f d/s\n", gy);
    display.setCursor(0, 40);
    display.printf("GZ: %.2f d/s\n", gz);

    // Refresh display
    display.display();
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    initMPU6050();
    initDisplay();
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.print("MPU Data URL: http://");
    Serial.println(WiFi.localIP());

    Serial.print("GPS Data URL: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":8080");
    
    // Start NTP Client
    timeClient.begin();
    
    // Initialize GPS
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
    
    // Serve MPU6050 Data on Port 80
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

    // Serve GPS Data on Port 8080
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
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }
    readMPU6050();   // Read MPU6050 Data
    updateDisplay(); // Update Data on OLED
    delay(500);
}
