
#include <Arduino.h>
#include <memory>
#include <TaskManager.h>
#include <MQTTAutoConnection.h>
#include <MQTTPingSender.h>
#include <ESP8266WiFi.h>
#include <PeriodicTask.h>
#include <Log64.h>
#include <BlinkingLed.h>
#include <WifiBlinkLed.h>
#include <PeriodicRestart.h>

TaskManager task_manager;
std::unique_ptr<PubSubClient> mqtt_client;
std::unique_ptr<MQTTPingSender> mqtt_ping_sender;
std::unique_ptr<WifiBlinkLed> wifi_blink_led;
std::unique_ptr<WiFiClient> wifi_client;
logger_t logger = LOGGER("main");

void setup_ping() {
  logger(String("Setting up MQTT client to ") + MQTT_SERVER + ":" + MQTT_PORT);
  mqtt_client->setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_ping_sender.reset(new MQTTPingSender(mqtt_client.get(), RP_CLIENT_ID, "iot-dev-ping", RP_CLIENT_ID));
  auto ping_task = new PeriodicTask(10000L, [] { mqtt_ping_sender->send(); });
  task_manager.push_back(ping_task);
}

void setup_mqtt_auto_conn() {
  auto mqtt_auto_conn = new MQTTAutoConnection(mqtt_client.get(), [] { return WiFi.isConnected(); },
                                               MQTT_USERNAME, MQTT_PASSWORD, RP_CLIENT_ID, 10000L);
  task_manager.push_back(mqtt_auto_conn);
}

void setup() {
  LOG64_INIT();

  BlinkingLed *blinking_led = new BlinkingLed(D4);
  wifi_blink_led.reset(new WifiBlinkLed(blinking_led));

  WifiNetworkTask *wifi_network_task = new WifiNetworkTask(RP_WIFI_SSID, RP_WIFI_PASSWORD, 30000L);
  wifi_network_task->addListener(wifi_blink_led.get());
  task_manager.push_back(blinking_led);
  task_manager.push_back(wifi_network_task);

  wifi_client.reset(new WiFiClient());

  mqtt_client.reset(new PubSubClient(*wifi_client));
  setup_mqtt_auto_conn();
  setup_ping();

  task_manager.push_back(new PeriodicRestart(1000*3600*6));
  task_manager.setup();
}

void loop() {
  task_manager.run();
  delay(100L);
}
