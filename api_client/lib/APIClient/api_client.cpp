#include "api_client.h"

String baseUrl = "https://joz9eef623.execute-api.us-east-2.amazonaws.com/dev/";

void getRequest(String route, String apiKey)
{
    HTTPClient* http = createClient(route, apiKey);
    // Make the request based on the request type
    int httpCode;
    httpCode = http->GET();
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

    return http;
}