#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// ======== LED Matrix Setup =========
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define DATA_PIN 23
#define CLK_PIN 18
#define CS_PIN 5

MD_Parola display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ======== WiFi Credentials =========
const char* ssid = "Nam";
const char* password = "0389487155";

// ======== Adafruit IO Setup ========
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

const char* aio_username = "leexuaanbachs";
const char* aio_key      = "";

// Create WiFi and MQTT clients
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, aio_username, aio_key);

// Feed to subscribe to
Adafruit_MQTT_Subscribe textFeed = Adafruit_MQTT_Subscribe(&mqtt, "leexuaanbachs/feeds/text");

String scrollText = "Welcome!";
SemaphoreHandle_t textMutex;

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" connected!");
}

void connectToMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to Adafruit IO...");
    int8_t ret = mqtt.connect();
    if (ret == 0) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, retrying in 5 sec. Code: "); Serial.println(ret);
      delay(5000);
    }
  }
}

// ======= Display Task =======
void displayTask(void *parameter) {
  display.begin();
  display.setIntensity(1);
  display.setSpeed(50);
  display.displayClear();

  String currentText = "";

  for (;;) {
    // Copy shared string safely
    xSemaphoreTake(textMutex, portMAX_DELAY);
    String newText = scrollText;
    xSemaphoreGive(textMutex);

    if (newText != currentText) {
      currentText = newText;
      display.displayClear();
      display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 100);
    }

    if (display.displayAnimate()) {
      // Restart scroll
      display.displayScroll(currentText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 100);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // Avoid tight loop
  }
}

void setup() {
  Serial.begin(115200);

  textMutex = xSemaphoreCreateMutex();

  connectToWiFi();
  mqtt.subscribe(&textFeed);

  // Create display task
  xTaskCreatePinnedToCore(
    displayTask,     // Task function
    "LEDDisplay",    // Name
    4096,            // Stack size
    NULL,            // Parameters
    1,               // Priority
    NULL,            // Task handle
    1                // Core 1 (optional)
  );
}

void loop() {
  // Reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (!mqtt.connected()) {
    connectToMQTT();
  }

  mqtt.processPackets(10);
  mqtt.ping();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &textFeed) {
      String newMsg = (char *)textFeed.lastread;
      newMsg.trim();
      Serial.print("Received: "); Serial.println(newMsg);

      // Safely update scrollText
      xSemaphoreTake(textMutex, portMAX_DELAY);
      scrollText = newMsg;
      xSemaphoreGive(textMutex);
    }
  }
}
