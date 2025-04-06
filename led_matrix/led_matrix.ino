#include <WiFi.h>
#include <AdafruitIO_WiFi.h>
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
#define AIO_USERNAME      "leexuaanbachs"
#define AIO_KEY           ""

AdafruitIO_WiFi io(AIO_USERNAME, AIO_KEY, ssid, password);
AdafruitIO_Feed *textFeed = io.feed("text");

String lastMessage = "";
unsigned long lastPoll = 0;
const unsigned long pollInterval = 4000; // 1 seconds

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setIntensity(5);
  display.displayClear();

  Serial.print("Connecting to WiFi / Adafruit IO");
  io.connect();
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" connected!");

  // Subscribe to feed responses
  textFeed->onMessage(handleMessage);
}

void loop() {
  io.run();

  unsigned long now = millis();
  if (now - lastPoll >= pollInterval) {
    lastPoll = now;

    Serial.println("Polling Adafruit IO...");
    textFeed->get();  // Triggers handleMessage() when data is received
  }

  if (display.displayAnimate()) {
    display.displayScroll(lastMessage.c_str(), PA_LEFT, PA_SCROLL_LEFT, 100);
  }
}

// Callback when feed responds
void handleMessage(AdafruitIO_Data *data) {
  String newText = data->toString();
  newText.trim();

  Serial.print("Received from Adafruit IO: ");
  Serial.println(newText);

  if (newText.length() > 0 && newText != lastMessage) {
    lastMessage = newText;
    display.displayClear();
    display.displayScroll(lastMessage.c_str(), PA_LEFT, PA_SCROLL_LEFT, 100);
  }
}