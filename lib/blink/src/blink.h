class Blinker
{
private:
    unsigned int _ledOff;
    unsigned int _ledOn;
    unsigned int _cycles;
    uint8_t _ledPin;
    uint8_t _ledMode;
    uint8_t _high;
    uint8_t _low;


public:
    Blinker(
        unsigned int ledOffMillis, 
        unsigned int ledOnMillis, 
        unsigned int cycles = 1, 
        uint8_t ledPin = BUILTIN_LED, 
        uint8_t pinMode = OUTPUT,
        uint8_t high = HIGH,
        uint8_t low = LOW);
    void blink();
};