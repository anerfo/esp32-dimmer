#include <Arduino.h>

class PushButton
{
    public:
        enum State
        {
            Idle,
            Wait,
            Ready,
            LongPress,
            Press,
            DoublePressWait,
            DoublePress
        };
    private:
        gpio_num_t _Pin;
        uint32_t _PressCycles;
        
        uint32_t _PressedLastCycle;
        uint32_t _LastChangeCycle;

        State _CurrentState;

        void ManageState(bool up, bool down, bool pressCycle)
        {
            State oldState = _CurrentState;
            switch(_CurrentState)
            {
                case Idle:
                    if(down)
                    {
                        _CurrentState = Wait;
                    }
                    break;
                case Wait:
                    if(up)
                    {
                        _CurrentState = DoublePressWait;
                    }
                    else if(pressCycle)
                    {
                        _CurrentState = Ready;
                    }
                    break;
                case Ready:
                    if(up)
                    {
                        _CurrentState = Press;
                        PressAction();
                    }
                    else if(pressCycle)
                    {
                        _CurrentState = LongPress;
                    }
                    break;
                case LongPress:
                    if(up)
                    {
                        _CurrentState = Idle;
                        LongPressFinishAction();
                    }
                    break;
                case Press:
                    if(down)
                    {
                        _CurrentState = Wait;
                    }
                    else if(pressCycle)
                    {
                        _CurrentState = Idle;
                    }
                    break;
                case DoublePressWait:
                    if(down)
                    {
                        _CurrentState = DoublePress;
                        DoublePressAction();
                    }
                    else if(pressCycle)
                    {
                        _CurrentState = Press;
                        PressAction();
                    }
                    break;
                case DoublePress:
                    if(up || pressCycle)
                    {
                        _CurrentState = Idle;
                    }
                    break;
            }
            if(_CurrentState == LongPress)
            {
                LongPressAction();
            }
            if(oldState != _CurrentState)
            {
                StateChangeAction(oldState, _CurrentState);
            }
        }

    public:

        PushButton(gpio_num_t pin, uint8_t mode = INPUT_PULLUP, uint32_t pressCycles = 10)
        {
            _Pin = pin;
            _PressCycles = pressCycles;
            _CurrentState = Idle;
            pinMode(_Pin, mode);
            LongPressAction = []{};
            LongPressFinishAction = []{};
            PressAction = []{};
            DoublePressAction = []{};
            StateChangeAction = [](State oldState, State newState){};
        }

        void Evaluate()
        {
            int pressed = digitalRead(_Pin) == 0;
            bool up = _PressedLastCycle == 1 && pressed == 0;
            bool down = _PressedLastCycle == 0 && pressed == 1;
            if(up || down)
            {
                _LastChangeCycle = 0;
            }
            else
            {
                _LastChangeCycle++;
            }
            ManageState(up, down, _LastChangeCycle >= _PressCycles);
            _PressedLastCycle = pressed;
        }

        void SleepUntilButtonPressed()
        {
            if(_CurrentState == Idle)
            {
                esp_sleep_enable_ext0_wakeup(_Pin, 0);
                Serial.printf("Going to sleep\n");
                esp_light_sleep_start();
                Serial.printf("Wake Up\n");
                ManageState(false, true, false);
                _LastChangeCycle = 0;
                _PressedLastCycle = true;
            }
        }

        void (*LongPressAction)();
        void (*LongPressFinishAction)();
        void (*PressAction)();
        void (*DoublePressAction)();
        void (*StateChangeAction)(State oldState, State newState);
};
