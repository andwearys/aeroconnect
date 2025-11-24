// AeroConnect - Finalized .ino for ESP32 (cleaned + user rules)
// Implements:
//  - Ultrasonic tank level (user's exact getDistance)
//  - pH sensor (GPIO32) reading and continuous dosing rule (A):
//      -> start nutritank peristaltic pump when pH < 5.5
//      -> stop when pH >= 6.5
//      -> safety max dosing timeout applied to prevent runaway
//  - mixedwtank_relay (GPIO16): 5s ON every 5 minutes
//  - returnwater_relay (GPIO18): 5s ON every 5 minutes
//  - plainwtank_relay (GPIO19): fill when water height < 5 cm, stop when >= 35 cm
//  - TDS (GPIO35) read for monitoring (no auto-dosing based on TDS in this version)
//
// NOTES / IMPORTANT:
//  - Calibrate PH_SLOPE / PH_OFFSET and TDS_MULTIPLIER for your hardware.
//  - Relay logic assumes active LOW modules. Change RELAY_ACTIVE_LOW if needed.
//  - We set MAX_WATER_DEPTH_CM to 35cm to match the user's "stop at 35 cm" requirement.

#include <Arduino.h>

// -------------------- PIN ASSIGNMENTS --------------------
const int PIN_TDS_EC = 35;        // TDS meter v1.0 (Analog Out)
const int PIN_PH = 32;            // pH sensor PH-4502C (Analog Out)

// Ultrasonic sensor (exact user code)
#define TRIG_PIN 14
#define ECHO_PIN 34

// Relays
const int RELAY_NUTRIENT_PIN = 17;   // nutritank (peristaltic)
const int RELAY_PLAIN_WATER_PIN = 19;// plain water pump (fill)
const int RELAY_MIXEDW_PUMP = 16;    // mixedwtank pump (5s every 5min)
const int RELAY_RETURNW_PUMP = 18;   // returnwater pump (5s every 5min)

// -------------------- TANK / ULTRASONIC SETTINGS --------------------
const float SENSOR_TO_BOTTOM_DISTANCE_CM = 40.0; // distance from sensor to bottom of empty tank
const float MAX_WATER_DEPTH_CM = 35.0;          // set to 35 cm so "stop at 35cm" is reachable

// Ultrasonic timeout for pulseIn (microseconds)
const unsigned long ULTRASONIC_TIMEOUT_US = 30000UL;

// -------------------- TIMING / CYCLIC PUMPS --------------------
const unsigned long PERIOD_5_MIN = 5UL * 60UL * 1000UL; // 5 minutes
const unsigned long PUMP_5SEC = 5UL * 1000UL;           // 5 seconds

unsigned long lastMixCycle = 0;
unsigned long lastReturnCycle = 0;
bool mixedRunning = false;
bool returnRunning = false;
unsigned long mixedStart = 0;
unsigned long returnStart = 0;

// -------------------- AUTO-DOSING SETTINGS (PH-based A)
bool AUTO_DOSING_ENABLED = true;
const float PH_START_DOSE = 5.5; // start dosing when pH < 5.5
const float PH_STOP_DOSE  = 6.5; // stop dosing when pH >= 6.5

// Safety maximum for continuous dosing (prevents stuck pump). If dosing longer than this, it will stop and log.
const unsigned long MAX_CONTINUOUS_DOSING_MS = 5UL * 60UL * 1000UL; // 5 minutes safety timeout

bool dosingInProgress = false;
unsigned long dosingStartMillis = 0;

// -------------------- PLAIN WATER FILL SETTINGS --------------------
// Plain pump turns ON when water height < PLAIN_FILL_START_CM and stops when >= PLAIN_FILL_STOP_CM
const float PLAIN_FILL_START_CM = 5.0;
const float PLAIN_FILL_STOP_CM  = 35.0;

bool fillingInProgress = false;
unsigned long fillingStartMillis = 0;
const unsigned long MAX_PLAIN_PUMP_ON_MS = 5UL * 60UL * 1000UL; // safety cutoff 5 min

// -------------------- SENSOR CALIBRATION PLACEHOLDERS --------------------
float PH_SLOPE = -3.2;     // placeholder slope for pH calibration (pH = slope*V + offset)
float PH_OFFSET = 14.0;    // placeholder offset
float TDS_MULTIPLIER = 0.5; // placeholder for ADC->ppm
float TDS_TO_EC_DIVIDER = 500.0; // ppm -> mS/cm
const int ADC_SAMPLES = 12;

// -------------------- STATE --------------------
float g_ph = 0.0;
float g_tds = 0.0;   // ppm
float g_ec = 0.0;    // mS/cm
float g_water_height = 0.0; // cm
float g_water_percent = 0.0; // 0-100

// Relay active logic (change if your module is active HIGH)
bool RELAY_ACTIVE_LOW = true;

// -------------------- HELPERS --------------------
void relayWrite(int pin, bool on) {
  if (RELAY_ACTIVE_LOW) digitalWrite(pin, on ? LOW : HIGH);
  else digitalWrite(pin, on ? HIGH : LOW);
}

float readAnalogAverage(int pin, int samples) {
  unsigned long sum = 0;
  for (int i = 0; i < samples; ++i) {
    sum += analogRead(pin);
    delay(3);
  }
  return (float)sum / (float)samples;
}

float adcToVoltage(float adcValue) {
  return (adcValue / 4095.0) * 3.3;
}

// -------------------- SENSOR READS --------------------
float readPH() {
  float raw = readAnalogAverage(PIN_PH, ADC_SAMPLES);
  float voltage = adcToVoltage(raw);
  float pH = PH_SLOPE * voltage + PH_OFFSET; // placeholder linear mapping
  return pH;
}

float readTDSppm() {
  float raw = readAnalogAverage(PIN_TDS_EC, ADC_SAMPLES);
  float ppm = raw * TDS_MULTIPLIER;
  return ppm;
}

float tdsToEc(float tds_ppm) {
  return (tds_ppm / TDS_TO_EC_DIVIDER);
}

// -------------------- ULTRASONIC (user exact) --------------------
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);
  float distance = duration * 0.034 / 2.0;
  return distance;
}

void computeWaterMetrics(float read_distance) {
  // Compute water height and percentage using exact formulas provided by user
  float waterheight = SENSOR_TO_BOTTOM_DISTANCE_CM - read_distance;
  float percentage = (waterheight / MAX_WATER_DEPTH_CM) * 100.0;

  // clamp to physical bounds
  if (waterheight < 0.0) waterheight = 0.0;
  if (waterheight > MAX_WATER_DEPTH_CM) waterheight = MAX_WATER_DEPTH_CM;
  if (percentage < 0.0) percentage = 0.0;
  if (percentage > 100.0) percentage = 100.0;

  g_water_height = waterheight;
  g_water_percent = percentage;
}

// -------------------- DOSING --------------------
void startDosing() {
  if (dosingInProgress) return;
  dosingInProgress = true;
  dosingStartMillis = millis();
  relayWrite(RELAY_NUTRIENT_PIN, true);
  Serial.println("[DOSE] ON (nutritank)");
}

void stopDosing() {
  if (!dosingInProgress) return;
  dosingInProgress = false;
  relayWrite(RELAY_NUTRIENT_PIN, false);
  Serial.println("[DOSE] OFF (nutritank)");
}

// -------------------- CYCLIC PUMP CONTROL (5s ON every 5min) --------------------
void handleMixedPump(unsigned long now) {
  if (!mixedRunning && (now - lastMixCycle >= PERIOD_5_MIN)) {
    mixedRunning = true;
    mixedStart = now;
    relayWrite(RELAY_MIXEDW_PUMP, true);
    Serial.println("[MIX] 5s ON");
  }
  if (mixedRunning && (now - mixedStart >= PUMP_5SEC)) {
    mixedRunning = false;
    lastMixCycle = now;
    relayWrite(RELAY_MIXEDW_PUMP, false);
    Serial.println("[MIX] OFF");
  }
}

void handleReturnPump(unsigned long now) {
  if (!returnRunning && (now - lastReturnCycle >= PERIOD_5_MIN)) {
    returnRunning = true;
    returnStart = now;
    relayWrite(RELAY_RETURNW_PUMP, true);
    Serial.println("[RETURN] 5s ON");
  }
  if (returnRunning && (now - returnStart >= PUMP_5SEC)) {
    returnRunning = false;
    lastReturnCycle = now;
    relayWrite(RELAY_RETURNW_PUMP, false);
    Serial.println("[RETURN] OFF");
  }
}

// -------------------- PLAIN WATER FILL CONTROL --------------------
void startFillingPump() {
  if (fillingInProgress) return;
  fillingInProgress = true;
  fillingStartMillis = millis();
  relayWrite(RELAY_PLAIN_WATER_PIN, true);
  Serial.println("[FILL] ON (plain water)");
}

void stopFillingPump() {
  if (!fillingInProgress) return;
  fillingInProgress = false;
  relayWrite(RELAY_PLAIN_WATER_PIN, false);
  Serial.println("[FILL] OFF (plain water)");
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("AeroConnect - Finalized (PH continuous dosing)");

  // pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_NUTRIENT_PIN, OUTPUT);
  pinMode(RELAY_PLAIN_WATER_PIN, OUTPUT);
  pinMode(RELAY_MIXEDW_PUMP, OUTPUT);
  pinMode(RELAY_RETURNW_PUMP, OUTPUT);

  // ensure relays are off
  relayWrite(RELAY_NUTRIENT_PIN, false);
  relayWrite(RELAY_PLAIN_WATER_PIN, false);
  relayWrite(RELAY_MIXEDW_PUMP, false);
  relayWrite(RELAY_RETURNW_PUMP, false);

  lastMixCycle = millis();
  lastReturnCycle = millis();
}

// -------------------- LOOP --------------------
void loop() {
  unsigned long now = millis();

  // Read sensors
  g_ph = readPH();
  g_tds = readTDSppm();
  g_ec = tdsToEc(g_tds);

  float dist = getDistance();
  computeWaterMetrics(dist);

  // Print status
  Serial.print("pH: "); Serial.print(g_ph, 3);
  Serial.print(" | TDS(ppm): "); Serial.print(g_tds, 1);
  Serial.print(" | EC(mS/cm): "); Serial.print(g_ec, 3);
  Serial.print(" | Height(cm): "); Serial.print(g_water_height, 2);
  Serial.print(" | Level(%): "); Serial.println(g_water_percent, 1);

  // -------------------- PH-BASED CONTINUOUS DOSING (A) --------------------
  if (AUTO_DOSING_ENABLED) {
    if (g_ph < PH_START_DOSE) {
      // start dosing if not already
      if (!dosingInProgress) startDosing();
    } else if (g_ph >= PH_STOP_DOSE) {
      // stop dosing when reached or exceeded stop threshold
      if (dosingInProgress) stopDosing();
    }
  }

  // Safety: force stop if dosing too long
  if (dosingInProgress && (now - dosingStartMillis >= MAX_CONTINUOUS_DOSING_MS)) {
    Serial.println("[SAFETY] Max continuous dosing time reached, stopping dosing");
    stopDosing();
  }

  // -------------------- PLAIN WATER FILL CONTROL --------------------
  if (g_water_height < PLAIN_FILL_START_CM && !fillingInProgress) {
    startFillingPump();
  }
  if (g_water_height >= PLAIN_FILL_STOP_CM && fillingInProgress) {
    stopFillingPump();
  }
  // Safety cutoff
  if (fillingInProgress && (now - fillingStartMillis >= MAX_PLAIN_PUMP_ON_MS)) {
    Serial.println("[SAFETY] Max plain pump runtime reached, stopping fill pump");
    stopFillingPump();
  }

  // -------------------- CYCLIC PUMPS --------------------
  handleMixedPump(now);
  handleReturnPump(now);

  delay(700);
}