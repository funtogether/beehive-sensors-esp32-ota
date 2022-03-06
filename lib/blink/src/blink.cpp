#include <Arduino.h>
#include <blink.h>

Blinker::Blinker(
    unsigned int ledOffMillis, 
    unsigned int ledOnMillis, 
    unsigned int cycles, 
    uint8_t ledPin, 
    uint8_t ledMode,
    uint8_t high,
    uint8_t low)
{
    _ledOff = ledOffMillis;
    _ledOn = ledOnMillis;
    _cycles = cycles;
    _ledPin = ledPin;
    _high = high;
    _low = low;
    pinMode(ledPin, ledMode);
}

void Blinker::blink()
{
    
    for (int i = _cycles; i > 0; i--)
    {
        digitalWrite(_ledPin, _high);
        delay(_ledOff);
        digitalWrite(_ledPin, _low);
        delay(_ledOn);
    }
}