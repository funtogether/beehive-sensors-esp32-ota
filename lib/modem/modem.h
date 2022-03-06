#ifndef Modem_H
#define Modem_H

#include <Arduino.h>

class Modem
{
private:
    bool _isSetup;
    bool _isGprsConnected;
    bool _isClientConnected;

public:
    Modem();

    void setup();

    bool connectGprs();

    bool connectClient(String server);

    String readString(String server, String resource);

    uint32_t readContentLength(String server,  String resource);

    bool downloadResource(String server, String resource, String localFilePath);

    void disconnectClient();

    void teardown();
};

#endif