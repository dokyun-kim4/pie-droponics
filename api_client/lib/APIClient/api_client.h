#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

String getRequest(String Url);
void postRequest(String route, String apiKey, String requestPayload);
HTTPClient* createClient(String route, String apiKey);
JsonObject parseResponse(String response);


struct Pod {
    String pod_id;
    String name;
    String garden_id;
    String location;
    String plant;
    String created_at;
    String updated_at;
};

struct Garden {
    String id;
    String name;
    String location;
    String config_id;
    String pods;
    String created_at;
    String updated_at;
};
struct Command {
    String id;
    String ref_id;
    int cmd;
    String type;
    String executed;
    String garden_id;
    String created_at;
    String updated_at;
};

struct ReactiveActuator {
    String id;
    String name;
    String sensor_id;
    String created_at;
    String updated_at;
};

struct ScheduledActuator {
    String id;
    String name;
    String garden_id;
    String created_at;
    String updated_at;
};

struct Sensor {
    String id;
    String name;
    String garden_id;
    String created_at;
    String updated_at;
};