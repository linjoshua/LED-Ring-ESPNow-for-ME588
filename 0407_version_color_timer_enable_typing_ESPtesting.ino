/*
=====================================================
LED Ring + ESP-NOW Controller (By: [Your Name])
Version: 2025-04-07
=====================================================

📘 USER GUIDE:
1️⃣ Color Button (GPIO 14):
    - Press to cycle through predefined team colors on the LED ring.

2️⃣ Enable Button (GPIO 26):
    - Press to start the robot system.
    - Sends an ESP-NOW "ENABLE" signal to other ESP32 modules.
    - Starts a 15-second timer.
    - After the timer, sends a "DISABLE" signal and stops the LED effects.

3️⃣ FSM State Control:
    - Receives FSM state (0–6) from other ESP32 modules via ESP-NOW.
    - Changes LED display mode accordingly.

4️⃣ Manual Debug Option:
    - You can type numbers 0 to 6 in the Serial Monitor (115200 baud)
      to manually override FSM display for debugging.

🧠 FSM State Mapping:
    0: Drive Forward
    1: Drive Backward
    2: Launch Left
    3: Launch Right
    4: Stop
    5: To Home
    6: At Home
=====================================================
*/

#include <Adafruit_NeoPixel.h>
#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 13
#define COLOR_BUTTON 14
#define NUM_LEDS 8
#define ENABLE_BUTTON 26
#define MY_ID 1  // Identifier for this ESP32 board

Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ----- Global Variables -----
bool enabled = false;
bool lastEnableButtonState = HIGH;
int colorIndex = 0;
int motionCommand = 0; // FSM state (0–6)
int lastColorButtonState = HIGH;
int startTime;
int duration = 150000; // Duration = 2 min 30 sec

uint32_t colors[] = {
  ring.Color(255, 0, 0),     // Red
  ring.Color(0, 255, 0),     // Green
  ring.Color(0, 0, 255),     // Blue
  ring.Color(255, 255, 0),   // Yellow
  ring.Color(255, 255, 255)  // White
};
int totalColors = sizeof(colors) / sizeof(colors[0]);

// ----- ESP-NOW Setup -----
typedef struct struct_message {
  uint8_t senderID;
  uint8_t command;   // 1 = ENABLE, 2 = DISABLE, 3 = FSM State
  uint8_t state;     // FSM state value (0–6)
} struct_message;

struct_message outgoingData;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Can be replaced with specific MAC

void sendCommand(uint8_t cmd, uint8_t state = 0) {
  outgoingData.senderID = MY_ID;
  outgoingData.command = cmd;
  outgoingData.state = state;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
  Serial.println(result == ESP_OK ? "ESP-NOW: Sent!" : "ESP-NOW: Send Failed");
}

// ESP-NOW receive callback (new ESP32 v3+ format)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  struct_message incoming;
  memcpy(&incoming, incomingData, sizeof(incoming));
  if (incoming.command == 3) {
    motionCommand = incoming.state;
    Serial.print("Received FSM state: ");
    Serial.println(motionCommand);
  }
}

void setup() {
  pinMode(COLOR_BUTTON, INPUT_PULLUP);
  pinMode(ENABLE_BUTTON, INPUT_PULLUP);
  Serial.begin(115200);

  ring.begin();
  ring.setBrightness(8);
  ring.show();

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {
  // Handle color selection button
  int colorButtonState = digitalRead(COLOR_BUTTON);
  if (colorButtonState == LOW && lastColorButtonState == HIGH) {
    delay(50); // Debounce
    colorIndex = (colorIndex + 1) % totalColors;
    alwaysBright(colors[colorIndex]);
  }
  lastColorButtonState = colorButtonState;

  // Handle ENABLE button
  bool enableState = digitalRead(ENABLE_BUTTON);
  if (enableState == LOW && lastEnableButtonState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(ENABLE_BUTTON) == LOW) {
      enabled = true;
      startTime = millis();
      Serial.println("Robot Enabled!");
      sendCommand(1); // Send ENABLE signal
    }
  }
  lastEnableButtonState = enableState;

  // Optional manual typing input for FSM state (0–6)
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input >= '0' && input <= '6') {
      motionCommand = input - '0';
      Serial.print("Manual Mode: ");
      Serial.println(motionCommand);
    }
  }

  // Main execution loop
  if (enabled) {
    switch (motionCommand) {
      case 0: rotatingEffectCW(colors[colorIndex]); Serial.println("DF"); break; //Drive Forward
      case 1: rotatingEffectCCW(colors[colorIndex]); Serial.println("DB"); break; //Drive Backward
      case 2: slowBlinkingEffect(colors[colorIndex]); Serial.println("LL"); break; //Launch Left
      case 3: fastBlinkingEffect(colors[colorIndex]); Serial.println("LR"); break; //Launch Right
      case 4: alwaysBright(colors[colorIndex]); Serial.println("STOP"); break; //STOP
      case 5: wipeFillEffect(colors[colorIndex]); Serial.println("TH"); break; //To Home
      case 6: pulseForward(colors[colorIndex]); Serial.println("AT"); break; //At Home
    }

    if ((millis() - startTime) > duration) {
      enabled = false;
      Serial.println("Timer expired. Sending DISABLE signal.");
      sendCommand(2); // Send DISABLE signal
    }
  } else {
    alwaysBright(colors[colorIndex]);
  }
}

// ================= LED Display Effects =================
void rotatingEffectCW(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.clear();
    ring.setPixelColor(i, color);
    ring.show();
    delay(100);
  }
}
void rotatingEffectCCW(uint32_t color) {
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    ring.clear();
    ring.setPixelColor(i, color);
    ring.show();
    delay(100);
  }
}
void slowBlinkingEffect(uint32_t color) {
  ring.fill(color);
  ring.show();
  delay(300);
  ring.clear();
  ring.show();
  delay(300);
}
void fastBlinkingEffect(uint32_t color) {
  ring.fill(color);
  ring.show();
  delay(40);
  ring.clear();
  ring.show();
  delay(40);
}
void alwaysBright(uint32_t color) {
  ring.fill(color);
  ring.show();
}
void wipeFillEffect(uint32_t color) {
  ring.clear();
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.setPixelColor(i, color);
    ring.show();
    delay(80);
  }
  delay(300);
  ring.clear();
  ring.show();
  delay(200);
}
void pulseForward(uint32_t color) {
  ring.clear();
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.clear();
    ring.setPixelColor(i, color);
    if (i > 0) ring.setPixelColor(i - 1, dimColor(color, 100));
    if (i > 1) ring.setPixelColor(i - 2, dimColor(color, 40));
    ring.show();
    delay(80);
  }
}
uint32_t dimColor(uint32_t color, uint8_t brightness) {
  uint8_t r = (uint8_t)(color >> 16);
  uint8_t g = (uint8_t)(color >> 8);
  uint8_t b = (uint8_t)color;
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;
  return ring.Color(r, g, b);
}

