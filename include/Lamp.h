#include <Arduino.h>

class Lamp
{
    public:
        enum STATE {
            OFF,
            ON
        };

    private:
        const int Frequency = 500;
        const int Resolution = 8;
        const int MaxValue = (1 << Resolution);
        const int MinValue = MaxValue * 0.1;
        const int BrightnessStep = MaxValue / 20;
        const int FadeSteps = 100;
        const int FadeInDuration = 2000;
        STATE LampState = OFF;
        int Pin;
        int Channel;
        uint32_t Brightness;

    public:
        Lamp(int pin, int channel = 0)
        {
            Pin = pin;
            Channel = channel;
            Brightness = MaxValue;
            pinMode(pin, OUTPUT);
            ledcAttachPin(pin, Channel);
            ledcSetup(Channel, Frequency, Resolution);
        }

        void SetBrightness(uint32_t newVal, bool fade = false, bool force = false) {
            uint32_t value = newVal;
            if (value > 0) {
                if(fade == false && force == false)
                {
                    if(value > MaxValue)
                    {
                        value = MaxValue;
                    }
                    if(value < MinValue)
                    {
                        value = MinValue;
                    }
                    Brightness = value;
                }
                LampState = ON;
            }
            else
            {
                LampState = OFF;
                value = 0;
            }
            ledcWrite(Channel, value);
            Serial.printf("Lamp: %i\n", value);
        }

        uint32_t GetBrightness()
        {
            return Brightness;
        }

        void Fade(uint32_t start, uint32_t end)
        {
            float step = ((float)start - (float)end) / FadeSteps;
            for(int i = 1; i <= FadeSteps; i++)
            {
                SetBrightness(start - i * step, true);
                delay(FadeInDuration / FadeSteps);
            }
            SetBrightness(end);            
        }

        void TurnOn()
        {
            Fade(0, Brightness);
        }

        void TurnOff()
        {
            Fade(Brightness, 0);
        }

        void Toggle()
        {
            if(LampState == ON)
            {
                TurnOff();
            }
            else
            {
                TurnOn();
            }
        }

        void Brighter()
        {
            SetBrightness(Brightness + BrightnessStep);
        }

        void Darker()
        {
            SetBrightness(Brightness - BrightnessStep);
        }

        const STATE GetLampState()
        {
            return LampState;
        }

        void Print()
        {
            Serial.printf("Lamp at Pin %i connected to channel %i @%iHz, %i bit Max=%i, Min=%i\n", Pin, Channel, Frequency, Resolution, MaxValue, MinValue);
        }
};
