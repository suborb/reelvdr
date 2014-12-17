#include "menuPercItem.h"
#include <math.h>


cMenuEditIntPercItem::cMenuEditIntPercItem(const char *Name, int *Value, int Min, int Max, const char *MinString, const char *MaxString, int Delta):cMenuEditItem(Name)
{
    value = Value;
    min = Min;
    max = Max;
    minString = MinString;
    maxString = MaxString;
    delta = Delta;
    if (*value < min)
        *value = min;
    else if (*value > max)
        *value = max;
    Set();
}

void cMenuEditIntPercItem::Set(void)
{
    if (minString && *value == min)
        SetValue(minString);
    else if (maxString && *value == max)
        SetValue(maxString);
    else
    {
        char buf[24];
        int percentage = (int)round(100 - (*value * 100) / max);
        printf("val: %i\n", *value);
        snprintf(buf, sizeof(buf), "%d%%", percentage);
        SetValue(buf);
    }
}

eOSState cMenuEditIntPercItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);

    if (state == osUnknown)
    {
        int newValue = *value;
        bool IsRepeat = Key & k_Repeat;
        Key = NORMALKEY(Key);
        switch (Key)
        {
        case kNone:
            break;
        case kRight:           // TODO might want to increase the delta if repeated quickly?
            newValue = *value - delta;
            fresh = true;
            if (!IsRepeat && newValue < min && max != INT_MAX)
                newValue = max;
            break;
        case kLeft:
            newValue = *value + delta;
            fresh = true;
            if (!IsRepeat && newValue > max && min != INT_MIN)
                newValue = min;
            break;
        default:
            if (*value < min)
            {
                *value = min;
                Set();
            }
            if (*value > max)
            {
                *value = max;
                Set();
            }
            return state;
        }
        if (newValue != *value && (!fresh || min <= newValue) && newValue <= max)
        {
            *value = newValue;
            Set();
        }
        state = osContinue;
    }
    return state;
}
