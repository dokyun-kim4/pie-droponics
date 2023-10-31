#include "api_client.h"
#include "set_keys.h"

String baseUrl = "https://joz9eef623.execute-api.us-east-2.amazonaws.com/dev/";

String getRequest(String Url) {
    String apiKey = getApiKey();
    String ReqBody, ResponseStr;
    WiFiClient Client;
    HTTPClient Http;
    int ResponseCode;
    Serial.printf("Url: %s\n", Url.c_str());
    Http.setTimeout(3000);
    Http.begin(Url);
    Http.addHeader("Content-Type", "application/json");
    Http.addHeader("x-api-key", apiKey);
    ResponseCode = Http.GET();
    ResponseStr = Http.getString();
    Serial.println("ResponseCode: " + String(ResponseCode));
    Serial.println("ResponseStr: " + ResponseStr);
    Http.end();

  return ResponseStr;
}

void postRequest(String route, String apiKey, String requestPayload)
{
    HTTPClient* http = createClient(route, apiKey);
    // Make the request based on the request type
    int httpCode;
    http->addHeader("Content-Type", "application/json");
    httpCode = http->POST(requestPayload);
    // Check the HTTP response code
    if (httpCode > 0)
    {
        // Success
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http->getString();
            Serial.println("HTTP request successful. Response:");
            Serial.println(payload);
        }
        else
        {
            Serial.print("HTTP request failed with error code: ");
            Serial.println(httpCode);
        }
    }
    else
    {
        Serial.println("HTTP request failed");
    }
    // End the request
    http->end();
}

HTTPClient* createClient(String route, String apiKey)
{
    HTTPClient* http = new HTTPClient();

    // Build the complete URL by combining the base URL and the route
    String url = baseUrl + route;

    // Specify the target URL
    http->begin(url);

    // Set the custom header for the API key
    http->addHeader("x-api-key", apiKey);
    http->addHeader("Content-Type", "application/json");

    return http;
}

JsonObject parseResponse(String response)
{
    // Parse the response into a JSON object
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return JsonObject();
    }
    JsonObject body = doc.as<JsonObject>();
    return body;
}

