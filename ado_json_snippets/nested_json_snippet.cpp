#include "ArduinoJson-v7.3.1.h"
#include <iostream>

int main()
{
    JsonDocument doc;

    JsonObject pin1Doc = doc["pin1"].to<JsonObject>();
    pin1Doc["value"] = 1500;
    pin1Doc["pin"] = 32;

    JsonObject pin2Doc = doc["pin2"].to<JsonObject>();
    pin2Doc["value"] = 2000;
    pin2Doc["pin"] = 33;

    char output[256];
    serializeJson(doc, output);

    std::cout << "Hello World!" << std::endl;
    std::cout << output << std::endl;

    return 0;
}
