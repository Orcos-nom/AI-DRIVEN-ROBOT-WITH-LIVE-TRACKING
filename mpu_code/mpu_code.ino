#ifndef MPU_CODE_H
#define MPU_CODE_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// WiFi Credentials
const char* ssid = "You want net nigga";
const char* password = "boxBOX09876";

// Create server instance
AsyncWebServer server(80);

// Create MPU6050 instance
Adafruit_MPU6050 mpu;

// Stores sensor data
float ax, ay, az; // Acceleration
float gx, gy, gz; // Gyroscope

// Initialize MPU6050
void initMPU6050() {
  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050!");
    while (1); // Halt if sensor fails
  }
  Serial.println("MPU6050 initialized!");
}

// Read sensor data
void readMPU6050() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;

  gx = g.gyro.x * 180 / PI; // Convert radians to degrees
  gy = g.gyro.y * 180 / PI;
  gz = g.gyro.z * 180 / PI;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Initialize I2C
  
  // Initialize MPU6050
  initMPU6050();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("Dashboard URL: http://");
  Serial.println(WiFi.localIP());

  // Serve Webpage
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>MPU6050 Orientation</title>
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
              options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: { y: { beginAtZero: false } }
              }
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

                // Update graph data
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
          * { margin: 0; padding: 0; box-sizing: border-box; font-family: Arial, sans-serif; }
          body { 
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            justify-content: center; 
            height: 100vh; 
            background: #111; 
            color: #fff; 
            text-align: center; 
          }
          h1 { font-size: 2.5em; margin-bottom: 20px; }
          .data-box { padding: 15px; margin: 10px; background: #222; border-radius: 10px; font-size: 1.5em; }
          .chart-container { width: 90vw; height: 50vh; }
        </style>
      </head>
      <body>
        <h1>MPU6050 Orientation</h1>
        <div class="data-box">Gyroscope (°/s): X=<span id="gx">0</span>, Y=<span id="gy">0</span>, Z=<span id="gz">0</span></div>
        <div class="chart-container">
          <canvas id="gyroChart"></canvas>
        </div>
      </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  // Serve sensor data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    readMPU6050();
    String data = String(ax) + "," + String(ay) + "," + String(az) + "," 
                + String(gx) + "," + String(gy) + "," + String(gz);
    request->send(200, "text/plain", data);
  });

  server.begin();
}

void loop() {
  readMPU6050();
  delay(100);
}
