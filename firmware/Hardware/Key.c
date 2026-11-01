#include "Board_Config.h"
#include "Key.h"

#define KEY_DEBOUNCE_SAMPLES    (2U)


static KeyValue_t Key_ReadRawValue(void)
{
    if (DL_GPIO_readPins(
            BSP_KEY_SW2_PORT,
            BSP_KEY_SW2_PIN) == 0U)
    {
        return KEY_VALUE_SW2;
    }

    if (DL_GPIO_readPins(
            BSP_KEY_SW3_PORT,
            BSP_KEY_SW3_PIN) == 0U)
    {
        return KEY_VALUE_SW3;
    }

    if (DL_GPIO_readPins(
            BSP_KEY_SW4_PORT,
            BSP_KEY_SW4_PIN) == 0U)
    {
        return KEY_VALUE_SW4;
    }

    if (DL_GPIO_readPins(
            BSP_KEY_SW5_PORT,
            BSP_KEY_SW5_PIN) == 0U)
    {
        return KEY_VALUE_SW5;
    }

    return KEY_VALUE_NONE;
}


/*
 * 每20ms调用一次。
 * 连续检测两次相同状态后确认按键状态。
 */
KeyEvent_t Key_Update20ms(void)
{
    static KeyValue_t candidate_value =
        KEY_VALUE_NONE;
    static KeyValue_t stable_value =
        KEY_VALUE_NONE;
    static uint8_t stable_count = 0U;

    KeyValue_t raw_value;
    KeyValue_t previous_stable_value;

    raw_value = Key_ReadRawValue();

    if (raw_value == candidate_value)
    {
        if (stable_count < KEY_DEBOUNCE_SAMPLES)
        {
            stable_count++;
        }
    }
    else
    {
        candidate_value = raw_value;
        stable_count = 1U;
    }

    if (stable_count < KEY_DEBOUNCE_SAMPLES)
    {
        return KEY_EVENT_NONE;
    }

    if (stable_value == candidate_value)
    {
        return KEY_EVENT_NONE;
    }

    previous_stable_value = stable_value;
    stable_value = candidate_value;

    if ((previous_stable_value == KEY_VALUE_NONE) &&
        (stable_value != KEY_VALUE_NONE))
    {
        return (KeyEvent_t)stable_value;
    }

    return KEY_EVENT_NONE;
}