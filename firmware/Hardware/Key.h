#ifndef KEY_H
#define KEY_H

#include <stdint.h>

typedef enum
{
    KEY_VALUE_NONE = 0,
    KEY_VALUE_SW2  = 2,
    KEY_VALUE_SW3  = 3,
    KEY_VALUE_SW4  = 4,
    KEY_VALUE_SW5  = 5
} KeyValue_t;

typedef enum
{
    KEY_EVENT_NONE      = KEY_VALUE_NONE,
    KEY_EVENT_SW2_PRESS = KEY_VALUE_SW2,
    KEY_EVENT_SW3_PRESS = KEY_VALUE_SW3,
    KEY_EVENT_SW4_PRESS = KEY_VALUE_SW4,
    KEY_EVENT_SW5_PRESS = KEY_VALUE_SW5
} KeyEvent_t;

KeyEvent_t Key_Update20ms(void);

#endif