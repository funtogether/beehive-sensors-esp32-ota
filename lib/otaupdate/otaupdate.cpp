#include <Arduino.h>
#include <otaupdate.h>
#include <blink.h>
#include <Update.h>

#include "FS.h"
#include "SPIFFS.h"

OtaUpdate::OtaUpdate(
    String server, 
    String resourceToBinary, 
    String resourceToVersion, 
    float currentVersion,
    Modem *modemWrapper)
{
    _server = server;
    _resourceToBinary = resourceToBinary;
    _resoureToVersion = resourceToVersion;
    _currentVersion = currentVersion;
    _modem = modemWrapper;
}

bool OtaUpdate::updateAvailable()
{
    _modem->setup();
    _modem->connectGprs();
    _modem->connectClient(_server);

    String header = _modem->readString(_server, _resoureToVersion);
    if(header.isEmpty())
    {
        Serial.println("Header is empty. No update available!");
        return false;
    }

    String versionOnServerStr = header.substring(header.length() - 6);    
    float versionOnServer = versionOnServerStr.toFloat();
    Serial.print("server version=");
    Serial.println(versionOnServer, 3);
    Serial.print("device version=");
    Serial.println(_currentVersion, 3);
    
    bool updateAvailable = versionOnServer > _currentVersion;
    if (updateAvailable)
    {
        Serial.println("A new update is available!");        
    }
    else
    {
        Serial.println("The current version is the most recent one, no update needed!");
    }

    return updateAvailable;
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("APOK");
    }
    else
    {
        Serial.println("APX");
    }
}

void readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available())
    {
        Serial.write(file.read());
        delayMicroseconds(100);
    }
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

void performUpdate(Stream &updateSource, size_t updateSize)
{
    if (Update.begin(updateSize))
    {
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize)
        {
            Serial.println("Writes : " + String(written) + " successfully");
        }
        else
        {
            Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        if (Update.end())
        {
            Serial.println("OTA finished!");
            if (Update.isFinished())
            {
                Serial.println("Restart ESP device!");
                ESP.restart();
            }
            else
            {
                Serial.println("OTA not fiished");
            }
        }
        else
        {
            Serial.println("Error occured #: " + String(Update.getError()));
        }
    }
    else
    {
        Serial.println("Cannot beggin update");
    }
}

void updateFromFS(String localFilePath)
{
    File updateBin = SPIFFS.open(localFilePath);
    if (updateBin)
    {
        if (updateBin.isDirectory())
        {
            Serial.println("Directory error");
            updateBin.close();
            return;
        }

        size_t updateSize = updateBin.size();

        if (updateSize > 0)
        {
            Serial.println("Starting update");
            performUpdate(updateBin, updateSize);
        }
        else
        {
            Serial.println("Binary size is 0 -> skipping update!");
        }

        updateBin.close();

        // whe finished remove the binary from sd card to indicate end of the process
        SPIFFS.remove("/update.bin");
        Serial.println("update was removed!");
    }
    else
    {
        Serial.println("no such binary");
    }
}

void OtaUpdate::update()
{
    Serial.println("Starting update...");
    Blinker(300, 300, 6, LED).blink();
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    SPIFFS.format();
    listDir(SPIFFS, "/", 0);
    
    String localFilePath = "/update.bin";
    bool downloadSucceeded = _modem->downloadResource(_server, _resourceToBinary, localFilePath);
    _modem->teardown();
    if(!downloadSucceeded)
    {        
        return;
    }
    
    updateFromFS(localFilePath);    
}