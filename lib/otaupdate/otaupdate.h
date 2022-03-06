#ifndef OtaUpdate_H
#define OtaUpdate_H

#include <Arduino.h>
#include <modem.h>

class OtaUpdate
{
    private:
        
        const uint32_t _port = 80;
        String _server;
        String _resourceToBinary;
        String _resoureToVersion;
        float _currentVersion;
        Modem *_modem;

    public:
        OtaUpdate(
            String server, 
            String resourceToBinary, 
            String resourceToVersion, 
            float currentVersion, 
            Modem *modem);
        bool updateAvailable();
        void update();
};
#endif