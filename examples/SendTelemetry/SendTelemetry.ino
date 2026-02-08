/*
    Meshtastic send/receive client

    Connects to a Meshtastic node via WiFi or Serial (and maybe one day Bluetooth),
    and instructs it to send a text message every SEND_PERIOD milliseconds.
    The destination and channel to use can be specified.

    If the Meshtastic nodes receives a text message, it will call a callback function,
    which prints the message to the serial console.
*/

// Uncomment the line below to enable debugging 
// #define MT_DEBUGGING

#include <Meshtastic.h>

// Pins to use for WiFi; these defaults are for an Adafruit Feather M0 WiFi.
#define WIFI_CS_PIN 8
#define WIFI_IRQ_PIN 7
#define WIFI_RESET_PIN 4
#define WIFI_ENABLE_PIN 2

// Pins to use for SoftwareSerial. Boards that don't use SoftwareSerial, and
// instead provide their own Serial1 connection through fixed pins
// will ignore these settings and use their own.
#define SERIAL_RX_PIN 13
#define SERIAL_TX_PIN 15
// A different baud rate to communicate with the Meshtastic device can be specified here
#define BAUD_RATE 38400

// Send a text message every this many seconds
#define SEND_PERIOD 300

uint32_t next_send_time = 0;
bool not_yet_connected = true;

struct envMetrics {
  bool has_temperature = false;
  float temperature;

  bool has_humidity = false;
  float humidity;

  bool has_wind = false;
  float wind;

  bool has_pressure = false;
  float pressure;
};

// This callback function will be called whenever the radio connects to a node
void connected_callback(mt_node_t *node, mt_nr_progress_t progress) {
  if (not_yet_connected) 
    Serial.println("Connected to Meshtastic device!");
  not_yet_connected = false;
}

void setup() {
  // Try for up to five seconds to find a serial port; if not, the show must gox on
  Serial.begin(115200);
  while(true) {
    if (Serial) break;
    if (millis() > 5000) {
      Serial.print("Couldn't find a serial port after 5 seconds, continuing anyway");
      break;
    }
  }

  Serial.print("Booted Meshtastic send/receive client in ");

// Change to 1 to use a WiFi connection
#if 0
  #include "arduino_secrets.h"
  Serial.print("wifi");
  mt_wifi_init(WIFI_CS_PIN, WIFI_IRQ_PIN, WIFI_RESET_PIN, WIFI_ENABLE_PIN, WIFI_SSID, WIFI_PASS);
#else
  Serial.print("serial");
  mt_serial_init(SERIAL_RX_PIN, SERIAL_TX_PIN, BAUD_RATE);
#endif
  Serial.println(" mode");

  randomSeed(micros());

  // Initial connection to the Meshtastic device
  mt_request_node_report(connected_callback);
}

void read_sensors(envMetrics& e) { 
  // Populate the EnvMetrics struct from sensors
  e.has_wind = true; 
  e.wind = 4.8; 
  
  e.has_pressure = true; 
  e.pressure = 1013.2; 
  
  e.has_temperature = true; 
  e.temperature = 16.5; 
}

void send_telemetry(const EnvMetrics& e) {

  meshtastic_Telemetry m = meshtastic_Telemetry_init_zero;
  m.which_variant  = meshtastic_Telemetry_environment_metrics_tag;
  
  auto& em = m.variant.environment_metrics;

  if (e.has_temperature) {
    em.has_temperature = true;
    em.temperature = e.temperature;
  }

  if (e.has_humidity) {
    em.has_relative_humidity = true;
    em.relative_humidity = e.humidity;
  }

  if (e.has_wind) {
    em.has_wind_speed = true;
    em.wind_speed = e.wind;
  }

  if (e.has_pressure) {
    em.has_barometric_pressure = true;
    em.barometric_pressure = e.pressure;
  }

  mt_send_telemetry(m);
}

void loop() {
  // Record the time that this loop began (in milliseconds since the device booted)
  uint32_t now = millis();

  // Run the Meshtastic loop, and see if it's able to send requests to the device yet
  bool can_send = mt_loop(now);

  // If we can send, and it's time to do so, send a text message and schedule the next one.
  if (can_send && now >= next_send_time) {
    
    envMetrics weather;
    // Populate the telemetry metrics
    read_sensors(weather);

    send_telemetry(weather);

    next_send_time = now + SEND_PERIOD * 1000;
  }
}
