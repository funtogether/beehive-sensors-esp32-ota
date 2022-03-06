#include <Update.h>
#include <Arduino.h>
#include <modem.h>
#include <otaupdate.h>

#include "FS.h"
#include "SPIFFS.h"

Modem modemWrapper;

const String server = __XSTRING(SERVER);
const String binaryPath = __XSTRING(BINARY_PATH);
const String versionPath = __XSTRING(VERSION_PATH);
const float version = (float)VERSION;

void setup()
{
    OtaUpdate updater = OtaUpdate(server, binaryPath, versionPath, version, &modemWrapper);
    if (updater.updateAvailable())
    {
        updater.update();
    }
    else
    {
        modemWrapper.teardown();
    }
}

void loop()
{
    Serial.println("loop()");
    delay(20000);
}