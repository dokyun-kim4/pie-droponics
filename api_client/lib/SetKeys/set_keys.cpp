#include "set_keys.h"

Preferences preferences;

void setupKeys(String wifiKey, String apiKey)
{
  // Set the API key
  preferences.begin("api_client", false);
  preferences.putString("apiKey", apiKey);
  preferences.putString("wifiKey", wifiKey);
  preferences.end();
}

String getApiKey()
{
  // Get the API key
  preferences.begin("api_client", false);
  String apiKey = preferences.getString("apiKey", "");
  preferences.end();

  return apiKey;
}

String getWifiKey()
{
  // Get the Wi-Fi key
  preferences.begin("api_client", false);
  String wifiKey = preferences.getString("wifiKey", "");
  preferences.end();

  return wifiKey;
}

void inputKeys()
{
    String wifiKey;
    String apiKey;

    bool wifiKeyEntered = false;
    bool apiKeyEntered = false;
    char c;

    Serial.printf("Enter Wifi key and press Enter: ");
    while (!wifiKeyEntered)
    {
    if (Serial.available() > 0)
    {
        c = Serial.read();
        if (c == '\n')
        {
        wifiKeyEntered = true;
        }
        else
        {
        wifiKey += c;
        }
    }
    }

    Serial.printf("Enter API key and press Enter: ");
    while (!apiKeyEntered)
    {
    if (Serial.available() > 0)
    {
        c = Serial.read();
        if (c == '\n')
        {
        apiKeyEntered = true;
        }
        else
        {
        apiKey += c;
        }
    }
    }

    // Remove any leading/trailing spaces and newline characters
    wifiKey.trim();
    apiKey.trim();

    Serial.println("Setting API key and Wi-Fi key...");
    setupKeys(wifiKey, apiKey);

    Serial.println("API key and Wi-Fi key set");
    Serial.printf("API key: %s\n", getApiKey());
    Serial.printf("Wifi key: %s\n", getWifiKey());
}