// Include libraries
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Setup wifi and server
const String ssid = "WIFI SSID HERE";
const String password = "WIFI PASSWORD HERE";
WebServer server(80);

const String prusalinkURL = "http://PRINTER LOCAL IP HERE/api/printer";
const String prusalinkKey = "PRUSALINK API KEY OF YOUR PRINTER HERE";


// Define pinout
constexpr int LED_PIN = 48;
constexpr int thermopin0 = 5;
constexpr int thermopin1 = 6;
constexpr int thermopin2 = 7;

constexpr int pinHeaterFan = 12;
constexpr int pinHeaterPTC = 13;

// Decleare globally defined variables
float V0stable = 0;
float V1stable = 0;
float V2stable = 0;
bool isPrinting = 0;
bool isAborted = 0;
String material = "";
float Ttarget = 0;
float Tcase = 0;
float ToutputAir = 0;
float Tchamber = 30;  //This will eventually be replaced by a sensor
int pwmDutyFan = 0;
int pwmDutyPTC = 0;

// Declare parameters
int resolution = 8;  // 8-bit resolution (0–255)
int freqPTC = 1000;
int freqFan = 10000;
bool manualMode = false;

// Setup task timing
unsigned long getPrinterStatusLast = 0;
const unsigned long getPrinterStatusInterval = 10000;

unsigned long serialPrintLast = 0;
const unsigned long serialPrintInterval = 1000;

unsigned long controlLast = 0;
const unsigned long controlInterval = 100;

// Safety parameters
float TtargetMax = 60.;
float TcaseSoftCap = 69.;
float TcaseHardCap = 72.;
float DeltaToutputAir = 15.;

//////////// Setup ////////////
void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Chamber heater control starting");

  // Setup wifi and server
  wifiAndServerConfig();

  // Initialize averaged voltage readings ;
  V0stable = getV(thermopin0);
  V1stable = getV(thermopin1);
  V2stable = getV(thermopin2);

  // Initalize output pins
  ledcAttach(pinHeaterFan, freqFan, resolution);
  ledcAttach(pinHeaterPTC, freqPTC, resolution);
}

unsigned long last = millis();
unsigned long now = millis();
unsigned long tstart = millis();
unsigned long loopTime = 0;
unsigned long controlTime = 0;
unsigned long checkTime = 0;

//////////// Main loop ////////////
void loop() {

  //// Find current time ////
  last = now;
  now = millis();
  loopTime = 0.99 * loopTime + 0.01 * (now - last);

  //// Read sensors and control PWM output every 100 ms ////
  if (now - controlLast >= controlInterval) {
    controlLast = now;
    tstart = millis();

    // isPrinting = true; //////// REMOVE LATER!!! ////////

    // Update sensor readings // This is the slowest part of the algorithm ~ 10 ms
    Tcase = getT(thermopin1);
    ToutputAir = getT(thermopin0);
    Tchamber = getT(thermopin2);

    // Run the PI control algorithm
    if (isPrinting && !isAborted) {

      // Only update PI algorithm if actually heating
      PIDControl();
    } else {

      //Shutdown mode: PTC off and fan on until cool
      pwmDutyPTC = 0;
      if ((Tcase > 40.0f) || (ToutputAir > 40.0f)) {
        pwmDutyFan = 255;
      } else {
        pwmDutyFan = 0;
      }
    }

    // Safty first!
    safetyChecks();

    // Execute PWM control
    ledcWrite(pinHeaterFan, pwmDutyFan);
    ledcWrite(pinHeaterPTC, pwmDutyPTC);

    // Time the speed
    controlTime = millis() - tstart;
  }

  //// Print status to serial every 1000 ms ////
  if (now - serialPrintLast >= serialPrintInterval) {
    serialPrintLast = now;

    serialPrint();
  }

  // Check if printing every 5 seconds ////
  if (now - getPrinterStatusLast >= getPrinterStatusInterval) {
    getPrinterStatusLast = now;

    tstart = millis();
    getPrinterStatus();
    checkTime = millis() - tstart;
  }

  //// Handle clients as soon as possible ////
  server.handleClient();

  //// Blink to indicate a loop has finished ////
  digitalWrite(LED_PIN, LOW);
  delay(5);
  digitalWrite(LED_PIN, HIGH);
}


//////////// Functions below ////////////

// PI parameters
float kP = 10.0;
float kI = 0.05;
float kD = 100.0;
float integral = 0;
float derivative = 0;
float dt = controlInterval / 1000.0;
float lastError = 0.;
unsigned long PIDControlLastActive = now;
void PIDControl() {

  // Record that PID control was active
  // This variable is used for safety checks
  PIDControlLastActive = now;

  // Control only from the smaller differential the primary temps
  float errorChamber = Ttarget - Tchamber;
  float errorOutput = (Ttarget + DeltaToutputAir) - ToutputAir;
  float errorPrimary = min(errorOutput, errorChamber);

  // Add in secondary control from the Tcase soft cap
  float errorCase = TcaseSoftCap - Tcase;  // Negative when too hot
  errorCase = min(errorCase, 0.0f);        // Only worry about over-temperature
  errorCase = 3 * errorCase;               // Amplify because this is really bad
  float error = errorPrimary + errorCase;  // Add up the control signal

  // Integral accumulation
  integral += error * dt;

  // Derivative (with moving average)
  derivative = 0.98 * derivative + 0.02 * (error - lastError) / dt;
  lastError = error;

  // PI output
  float output = kP * error + kI * integral + kD * derivative;

  // Clamup
  pwmDutyPTC = constrain(output, 0, 255);

  // Anti-windup
  if (pwmDutyPTC == 0 || pwmDutyPTC == 255) {
    integral -= error * dt;
  }

  // Control the fan
  if (pwmDutyPTC > 0) {
    pwmDutyFan = 255;
  } else if (Tcase > 40 || ToutputAir > 40) {
    pwmDutyFan = 255;
  } else {
    pwmDutyFan = 0;
  }
}

void serialPrint() {

  // This is printed to serial
  // Nothing here has an effect on the rest of the program
  Serial.print("Tchamber:");
  Serial.print(Tchamber);
  Serial.print(",");
  Serial.print("Tcase:");
  Serial.print(Tcase);
  Serial.print(",");
  Serial.print("Toutput:");
  Serial.print(ToutputAir);
  Serial.print(",");
  Serial.print("Ttarget:");
  Serial.print(Ttarget);
  Serial.print(",");
  Serial.print("ptc:");
  Serial.print((pwmDutyPTC * 100) / 255);
  Serial.print(",");
  Serial.print("fan:");
  Serial.print((pwmDutyFan * 100) / 255);
  Serial.print(",");
  Serial.print("int:");
  Serial.print((kI * integral * 100) / 255);

  Serial.println();

  if (isAborted) {
    Serial.println("Warning: Abort is in effect");
  }
}

void safetyChecks() {

  // The case should never be hotter than the output air.
  // This indicates fan failure, which must trigger an abort.
  if (
    (Tcase > ToutputAir + 5.0f)
    && (pwmDutyPTC > 0)  //Only worry if the heater is active. Cooldown is safe.
  ) {
    isAborted = true;
  }

  // Disable the PTC entirely if the case exceeds the hard cap
  if (Tcase > TcaseHardCap) {
    pwmDutyPTC = 0;
  }

  // The target should never be above TtargetMax
  if (Ttarget > TtargetMax) {
    Ttarget = TtargetMax;
  }

  // If the PID control loop is dormant for too long then the integral value
  // will be invalidated. Therefore, I reset it here to prevent overshoot.
  // The time out is set to be a bit longer than two pinter checkins.
  if (now - PIDControlLastActive >= 2 * getPrinterStatusInterval + 1000) {
    integral = 0;
  }
}

void wifiAndServerConfig() {
  // Setup wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connection succesful");

  // Setup server
  server.on("/isPrinting", HTTP_GET, handleIfPrinting);
  server.on("/setDutyFan", HTTP_GET, handleSetDutyFan);
  server.on("/setDutyPTC", HTTP_GET, handleSetDutyPTC);
  server.on("/abort", HTTP_GET, handleAbort);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/setTtarget", HTTP_GET, handleSetTtarget);
  server.on("/unabort", HTTP_GET, handleUnabort);
  server.on("/modeAuto", HTTP_GET, handleModeAuto);
  server.on("/modeManual", HTTP_GET, handleModeManual);
  server.begin();
}

void handleStatus() {
  StaticJsonDocument<256> doc;

  doc["sensors"]["Tchamber"] = Tchamber;
  doc["sensors"]["ToutputAir"] = ToutputAir;
  doc["sensors"]["Tcase"] = Tcase;
  doc["PWM"]["pwmDutyFan"] = pwmDutyFan;
  doc["PWM"]["pwmDutyPTC"] = pwmDutyPTC;
  doc["PWM"]["error"] = lastError;
  doc["PWM"]["proportional"] = kP * lastError;
  doc["PWM"]["integral"] = integral * kI;
  doc["PWM"]["derivative"] = derivative * kD;
  doc["variables"]["Ttarget"] = Ttarget;
  doc["variables"]["ToutputMax"] = Ttarget + DeltaToutputAir;
  doc["variables"]["TcaseMax"] = TcaseSoftCap;
  doc["variables"]["isAborted"] = isAborted;
  doc["variables"]["isPrinting"] = isPrinting;
  doc["variables"]["manualMode"] = manualMode;

  String output;
  serializeJson(doc, output);

  server.send(200, "application/json", output);
}

void handleAbort() {
  isAborted = true;
  server.send(200, "text/plain", "Aborted!\n");
  Serial.println("Aborted!!!!");
}

void handleUnabort() {
  isAborted = false;
  server.send(200, "text/plain", "Abort aborted.\n");
  Serial.println("Abort aborted.");
}

void handleIfPrinting() {
  String response = String("{\"isPrinting\": ") + String(isPrinting ? "true" : "false") + "}";
  server.send(200, "application/json", response);
  Serial.println("Client taken care off!");
}

void handleSetTtarget() {
  if (server.hasArg("value")) {
    float T = server.arg("value").toInt();

    Serial.println(T);
    Serial.println(T <= TtargetMax);
    Serial.println((0.0f < T) && (T <= TtargetMax));

    if ((0.0f <= T) && (T <= TtargetMax)) {

      Ttarget = T;
      server.send(200, "text/plain", "OK\n");
      Serial.printf("Ttarget set to %f\n", Ttarget);

    } else if ((TtargetMax < T) && (T < 100)) {

      Ttarget = TtargetMax;
      server.send(206, "text/plain", "Ttarget set to max\n");

    } else {

      server.send(400, "text/plain", "Invalid target temperature\n");
      Serial.println("Invalid Ttarget request recieved");
    }

  } else {
    server.send(400, "text/plain", "Missing value\n");
  }
}

// This currently serves no purpose
void handleSetDutyFan() {
  if (server.hasArg("value")) {
    int duty = server.arg("value").toInt();

    // constrain to valid PWM range
    duty = constrain(duty, 0, 255);

    // apply it (example)
    pwmDutyFan = duty;

    server.send(200, "text/plain", "OK\n");
    Serial.printf("Fan duty set to %d\n", duty);

  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

void handleSetDutyPTC() {
  if (server.hasArg("value")) {
    int duty = server.arg("value").toInt();

    // constrain to valid PWM range
    duty = constrain(duty, 0, 255);

    // apply it (example)
    pwmDutyPTC = duty;

    server.send(200, "text/plain", "OK\n");
    Serial.printf("PTC duty set to %d\n", duty);

  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

void handleModeManual() {
  manualMode = true;
  Ttarget = 0;

  server.send(200, "text/plain", "Controller is now in manual mode.\n");
  Serial.println("Controller is now in manual mode.");
}

void handleModeAuto() {
  manualMode = false;

  server.send(200, "text/plain", "Controller is now in auto mode.\n");
  Serial.println("Controller is now in manual mode.");
}

bool getPrinterStatus() {
  HTTPClient http;

  http.begin(prusalinkURL);
  http.addHeader("X-Api-Key", prusalinkKey);
  http.setConnectTimeout(500);
  http.setTimeout(2000);
  int httpCode = http.GET();


  if (httpCode == HTTP_CODE_OK) {

    // Get payload string
    String payload = http.getString();

    // Parse payload into JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("Parsing failed");
    }

    if (manualMode) {

      isPrinting = true;
      material = "ignored";

    } else {

      // Extract the printing bool
      isPrinting = doc["state"]["flags"]["printing"].as<bool>();
      material = doc["telemetry"]["material"].as<String>();

      // Set target temperature by material
      if (material == "ABS") {
        Ttarget = 60;
      } else if (material == "ASA") {
        Ttarget = 60;
      } else if (material == "PC") {
        Ttarget = 60;
      } else if (material == "PA") {
        Ttarget = 0;
      } else {
        Ttarget = 0;
      }
    }

    // Serial.println(isPrinting ? "true" : "false");
  } else {
    Serial.print("Printer not responding: HTTP error: ");
    Serial.println(httpCode);

    Serial.println("Assuming that the printer is not printing");
    isPrinting = false;
  }

  http.end();
  return isPrinting;
}

float getV(int thermopin) {

  // Use the built-in calibration
  return analogReadMilliVolts(thermopin) / 1000.0f;
}

float getVStable(int thermopin) {

  // Load the moving average
  float Vstable;
  if (thermopin == thermopin0) {
    Vstable = V0stable;
  } else if (thermopin == thermopin1) {
    Vstable = V1stable;
  } else if (thermopin == thermopin2) {
    Vstable = V2stable;
  } else {
    Serial.println("Error! Vstable not found!");
  }


  // Add 10 steps to the moving average
  float Vunstable;
  for (int i = 0; i < 100; i++) {
    Vunstable = getV(thermopin);
    Vstable = 0.999 * Vstable + 0.001 * Vunstable;
    // Vstable = 0.995 * Vstable + 0.005 * Vunstable;
  }


  // Update the moving average
  if (thermopin == thermopin0) {
    V0stable = Vstable;
  } else if (thermopin == thermopin1) {
    V1stable = Vstable;
  } else if (thermopin == thermopin2) {
    V2stable = Vstable;
  } else {
    Serial.println("Error! Vstable not found!");
  }

  return Vstable;
}

float thermistorResistanceOfVoltage(float Vout) {

  float Rfixed = 10.;
  float Vin = 3.28;

  float Rthermistor = Rfixed * (Vin - Vout) / Vout;

  return Rthermistor;
}

float tempOfV(float V) {

  float a = 0.00396173;
  float b = -0.000271617;
  float c = 7.32481 * pow(10, -7);

  float R = thermistorResistanceOfVoltage(V);

  float T = 1 / (a + b * log(R) + c * pow(log(R), 3)) - 273.15;

  return T;
}

float getT(int thermopin) {
  float V = getVStable(thermopin);
  float T = tempOfV(V);
  return T;
}