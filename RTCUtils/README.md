# RTCUtils

This library helps to setup the hardware RTC and work with timestamps. The motivation behind this library is that we need to know the actual time on the board to be able to connect to Google IoT Core API and MQTT broker.

# Usage

```c
#include "rtc_utils.h"

void main() {
    // setup the hardware
    RTCUtils_RTCInit();

    // set the UNIX timestamp
    uint32_t timestampToSet = 1515932870;
    RTCUtils_SetEpochTimestamp(timestamp);

    // get the unix timestamp
    uint32_t timestampFromRTC = RTCUtils_GetEpochTimestamp();

    // get RTC date and time in structure
    RTC_DateTypeDef dDate;
    RTC_TimeTypeDef dTime;
    RTCUtils_GetDateTime(&dTime, &dDate);
}

```