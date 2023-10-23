#include <HTTPClient.h>
#include <Arduino.h>

void getRequest(String route, String apiKey);
void postRequest(String route, String apiKey, String requestPayload);
HTTPClient* createClient(String route, String apiKey);

