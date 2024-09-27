#include <iostream>
#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include "HTTPClient.h"

using namespace std;

//============================== Class Color ==================================

class Colors {
  private:
  int red;
  int green;
  int blue;

  public:
  Colors(int r = 0, int g = 0, int b = 0) : red(r), green(g), blue(b) {}

  int getRed() {
    return red;
    }

  int getGreen() {
    return green;
    }

  int getBlue() {
    return blue;
    }

  void setColor(int r, int g, int b) {
    this->red = r;
    this->green = g;
    this->blue = b;
    }

  void getColor(int& r, int& g, int& b) const {
    r = this->red;
    g = this->green;
    b = this->blue;
    }

  // Método para exibir a cor
  void printColor() const {
    std::cout << "Red: " << red << "\nGreen: " << green << "\nBlue: " << blue << std::endl;
    }
  };

//============================== End Class ====================================

//============================== Class Esp32 ==================================

class Esp32 {
  public:

  #pragma region Variables

  //============================== Main Pins ==================================
  // Setting the main pins

  // Main Led Pins
  // const int RED_LED[3] = { 16, 28, 33 };
  // const int GREEN_LED[3] = { 13, 30, 36 };
  // const int BLUE_LED[3] = { 27, 31, 37 };

  // Main Sensor Pins
  // const int VCC_SENSOR_PIN = 12;

  //============================== End Main Pins ==============================

  //============================== WiFi Services ==============================

  const char* APSSID[2] = { "WIFI_EXT", "WIFI_INTERNAL" };
  const char* PASSWORD[2] = { "PASSWORD1", "PASSWORD2" };

  string host;
  string enpoint;

  WebServer server(80);

  //============================== End WiFi Services ==========================

  //============================== CPU Frequency ==============================
  // Setting the CPU frequency

  const int CPU_FREQ[6] = { 10, 20, 40, 80, 160, 240 };

  uint32_t Freq = 0;

  //============================== End CPU Frequency ==========================


  //============================== Color Scheme ===============================

  const Colors RED(255, 0, 0);
  const Colors BLUE(0, 0, 255);
  const Colors YELLOW(255, 255, 0);
  const Colors PURPLE(255, 0, 255);
  const Colors WHITE(255, 255, 255);

  const Colors FUNCTIONAL_LIGHTS[5] = { RED, BLUE, YELLOW, PURPLE, WHITE };

  //============================== End Colors =================================

  TaskHandle_t verifyWakeup; // setter core 0 to send data for central

  #pragma endregion

  // set API endpoint
  void setAPIEndpoint(string end) {
    host = "http:192.168.4.1"
      enpoint = end;
    host += endpoint;
    }

  // Change leds Color
  void setLedColor(Colors col, const int rgbLed[] = nullptr) {
    // if rgbLed was set will individually change the led pin color
    if (rgbLed != nullptr) {
      analogWrite(rgbLed[0], col.getRed());
      analogWrite(rgbLed[1], col.getGreen());
      analogWrite(rgbLed[2], col.getBlue());
      }
    else {
      for (int i : RED_LED) {
        analogWrite(RED_LED[i], col.getRed());
        analogWrite(RED_LED[i], col.getGreen());
        analogWrite(RED_LED[i], col.getBlue());
        }
      }

    }

  // Set ON/OFF the sensor pins
  void switchSensorPin(bool* manuallySet = nullptr) {
    // if manuallySet was set then admin can manually change ON/OFF
    if (manuallySet != nullptr) {
      digitalWrite(VCC_SENSOR_PIN, *manuallySet);
      }
    else {
      digitalWrite(VCC_SENSOR_PIN, !digitalRead(VCC_SENSOR_PIN));
      }
    }

  // Configure itself as Access Point
  void setAccessPoint() {

    WiFi.softAP(APSSID[0], PASSWORD[0]);

    IPAddress IP = WiFi.softAPIP();

    Serial.print("Access Point IP address: ");
    Serial.println(IP);
    }

  // Set CPU Frequency
  void powerControl(int mode) {
    // 10,20,40 - DO NOT HAVE WIFI/BT BUT LOWER ENERGY COSTS
    // 80,160,240 - HAVE WIFI/BT BUT HIGHER ENERGY COSTS

    setCpuFrequencyMhz(CPU_FREQ[mode]);
    }

  // Set WiFi connection
  void setupWifi() {
    int networkIndex = 0;
    int triesCounter = 0;

    // Board will try to connect to both networks
    while (WiFi.status() != WL_CONNECTED) {
      triesCounter = 0;

      Serial.print("Trying to  connect to: ");
      Serial.println(APSSID[networkIndex]);

      WiFi.begin(APSSID[networkIndex], PASSWORD[networkIndex]);
      while ((WiFi.status() != WL_CONNECTED) && (triesCounter <= 240)) {
        triesCounter++;
        delay(500);
        Serial.println("Connecting to WiFi..");

        if (triesCounter > 240) {
          break;
          }
        else {
          networkIndex = (networkIndex + 1) % 2; // switch between networks
          }
        }

      Serial.println("Connected to the WiFi network");
      }
    }

  // Time until next connection
  void sleepTimer(int seconds) {
    // By default will be used 2 minutes or 120 * 1000 milisseconds
    seconds *= 1000;
    if (millis() - prevStamp >= seconds) {
      prevStamp = millis();
      xTaskCreatePinnedToCore(
        wakingUp,      // Function to be performed the task is called
        "canWake",     // Name of the task in text
        2048,          // Stack size (Memory size assigned to the task)
        NULL,          // Pointer that will be used as the parameter for the task being created
        5,             // Task Priority
        &verifyWakeup, // The task handler
        1              // xCoreID (Core 1)
      );
      }
    }

  void setHibernate() {
    powerControl(0);
    sleepTimer(120);
    }

  void wakingUp() {
    powerControl(4);
    setupWifi();
    sendRequest();
    }

  // Send request to central
  void sendRequest() {
    if (Wifi.status() == WL_CONNECTED) {
      HTTPClient http;

      http.begin(host);

      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        string payload = http.getstring();
        Serial.println(httpResponseCode);
        Serial.println(payload);
        }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        }

      http.end();
      }
    }

  void inicializeProcess() {
    powerControl(4);
    switchSensorPin(true);
    }
  };

//============================== End Class ====================================