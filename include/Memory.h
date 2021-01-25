#include <EEPROM.h>

class Memory
{
    private:
        struct Storage
        {
            uint32_t Brightness;
        } _Storage, _LastStorage;

        bool Equals(const Storage& first, const Storage& second)
        {
            return first.Brightness == second.Brightness;
        }

        Memory()
        {
            EEPROM.begin(sizeof(Storage));
            EEPROM.get(0, _Storage);
        }

    public:
        static Memory& Instance()
        {
            static Memory _Instance;
            return _Instance;
        }

        void Store()
        {
            Serial.printf("Storage %i, LastStorage %i\n", _Storage.Brightness, _LastStorage.Brightness);
            if(Equals(_Storage, _LastStorage) == false)
            {
                Serial.printf("Writing Storage\n");
                _LastStorage = _Storage;
                EEPROM.put(0, _Storage);
                EEPROM.commit();
            }
        }

        void SetBrightness(uint8_t brightness)
        {
            _Storage.Brightness = brightness;
            Store();
        }

        uint8_t GetBrightness()
        {
            return _Storage.Brightness;
        }

        void Print()
        {
            Serial.printf("Memory:\n");
            Serial.printf("  Brightness: %i\n", _Storage.Brightness);
        }
};
