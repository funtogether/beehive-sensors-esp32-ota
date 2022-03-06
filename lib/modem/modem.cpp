#include <modem.h>
#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#define SerialAT Serial1

#define MODEM_RST 5
#define MODEM_PWRKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define LED_GPIO 13
#define LED_ON HIGH
#define LED_OFF LOW

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1030

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient modemClient(modem);

const char apn[] = "TM";
const char user[] = "";
const char pass[] = "";

Modem::Modem() {}

void Modem::setup()
{
    if (_isSetup)
    {
        return;
    }

    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    Serial.begin(BAUD_RATE);    

#ifdef MODEM_RST
    // Keep reset high
    pinMode(MODEM_RST, OUTPUT);
    digitalWrite(MODEM_RST, HIGH);
#endif

    pinMode(MODEM_PWRKEY, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);

    // Turn on the Modem power first
    digitalWrite(MODEM_POWER_ON, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);

    // Initialize the indicator as an output
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);

    delay(3000);
    Serial.println("Initializing modem...");
    modem.restart();

    String modemInfo = modem.getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    _isSetup = true;
}

bool Modem::connectGprs()
{
    if(!_isSetup)
    {
        return false;
    }

    if(_isGprsConnected)
    {
        return true;
    }

    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork())
    {
        Serial.println(" fail");
        delay(10000);
        return false;
    }
    Serial.println(" OK");
    Serial.print("Connecting to ");
    Serial.print(apn);
    if (!modem.gprsConnect(apn, user, pass))
    {
        Serial.println(" fail");
        delay(10000);
        return false;
    }
    Serial.println(" OK");
    _isGprsConnected = true;
    return _isGprsConnected;
}

bool Modem::connectClient(String server)
{
    if(!_isSetup || !_isGprsConnected)
    {
        Serial.println("_isSetup=false || _isGprsConnected=false");
        return false;
    }

    Serial.print("Connecting to ");
    Serial.print(server);
   
    char serverAsChars[server.length() + 1];
    server.toCharArray(serverAsChars, server.length() + 1);
    
    if (!modemClient.connect(serverAsChars, 80))
    {
        Serial.println(" fail");
        delay(10000);
        _isClientConnected = false;
        return false;
    }
    Serial.println(" OK");
    _isClientConnected = true;
    return _isClientConnected;
}

void Modem::disconnectClient()
{
    if(!_isClientConnected)
    {
        return;
    }

    modemClient.stop();
    _isClientConnected = false;
}

String Modem::readString(String server, String resource)
{
    String s;
    if(!_isClientConnected)
    {
        return s;
    }

    modemClient.print(String("GET ") + resource + " HTTP/1.0\r\n");
    modemClient.print(String("Host: ") + server + "\r\n");
    modemClient.print("Connection: close\r\n\r\n");

    long timeout = millis();
    while (modemClient.available() == 0)
    {
        if (millis() - timeout > 5000L)
        {
            Serial.println(">>> Client Timeout !");
            disconnectClient();
            Serial.println("Could not establish connection to resource!");
            delay(10000L);
            return s;
        }
    }

    while (modemClient.available())
    {
        s = modemClient.readString();        
    }

    return s;
}

void Modem::teardown()
{
    if (_isSetup)
    {
        if (_isClientConnected)
        {
            disconnectClient();
        }

        if (_isGprsConnected)
        {
            modem.gprsDisconnect();
        }
    }

    _isSetup = false;
    _isGprsConnected = false;
    _isClientConnected = false;

    Serial.println("Teardown modem and client successful!");
}

uint32_t Modem::readContentLength(String server, String resource)
{
    if (!_isClientConnected)
    {
        return 0;
    }

    Serial.println("Resolving contentLength from " + server + resource);

    modemClient.print(String("GET ") + resource + " HTTP/1.0\r\n");
    modemClient.print(String("Host: ") + server + "\r\n");
    modemClient.print("Connection: close\r\n\r\n");    

    long timeout = millis();
    while (modemClient.available() == 0)
    {
        if (millis() - timeout > 5000L)
        {
            Serial.println(">>> Client Timeout !");
            disconnectClient();
            delay(10000L);
            return 0;
        }
    }

    uint32_t contentLength = 0;
    while (modemClient.available())
    {
        String line = modemClient.readStringUntil('\n');
        line.trim();
        Serial.println(line);   
        line.toLowerCase();
        if (line.startsWith("content-length:"))
        {
            contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
        }
        else if (line.length() == 0)
        {
            break;
        }
    }

    return contentLength;
}

void printPercent(uint32_t readLength, uint32_t contentLength)
{
    // If we know the total length
    if (contentLength != -1)
    {
        Serial.print("\r ");
        Serial.print((100.0 * readLength) / contentLength);
        Serial.print('%');
    }
    else
    {
        Serial.println(readLength);
    }
}

bool Modem::downloadResource(String server, String resource, String localFilePath)
{       
    connectClient(server);
    uint32_t contentLength = readContentLength(server, resource);    
    
    Serial.print("Content length=");
    Serial.println(contentLength);

    File file = SPIFFS.open(localFilePath, FILE_APPEND);
    long timeout = millis();
    uint32_t readLength = 0;

    unsigned long timeElapsed = millis();
    printPercent(readLength, contentLength);

    while (readLength < contentLength && modemClient.connected() && millis() - timeout < 10000L)
    {
        while (modemClient.available())
        {
                // read file data to spiffs
            if (!file.print(char(modemClient.read())))
            {
                Serial.println("Appending file");
            }

            readLength++;

            if (readLength % (contentLength / 13) == 0)
            {
                printPercent(readLength, contentLength);
            }
            timeout = millis();
        }
    }

    file.close();

    printPercent(readLength, contentLength);
    timeElapsed = millis() - timeElapsed;
    Serial.println();
    return true;
}