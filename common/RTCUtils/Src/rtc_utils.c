#include "rtc_utils.h"

#define JULIAN_DATE_BASE     2440588   // Unix epoch time in Julian calendar (UnixTime = 00:00:00 01.01.1970 => JDN = 2440588)

RTC_HandleTypeDef rtcHandle;

void RTCUtils_RTCInit() {
	RCC_OscInitTypeDef RtcOscInit;
	RtcOscInit.OscillatorType = RCC_OSCILLATORTYPE_LSI;
	RtcOscInit.PLL.PLLState = RCC_PLL_NONE;
	RtcOscInit.LSIState = RCC_LSI_ON;
	RtcOscInit.LSEState = RCC_LSE_OFF;
	if (HAL_RCC_OscConfig(&RtcOscInit) != HAL_OK) {
		printf("Error while configuring LSI\r\n");
	}

	RCC_PeriphCLKInitTypeDef RtcPeriphClkInit;
	RtcPeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	RtcPeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	if (HAL_RCCEx_PeriphCLKConfig(&RtcPeriphClkInit) != HAL_OK) {
		printf("Error while configuring RTC source to LSI\r\n");
	}
	__HAL_RCC_RTC_ENABLE();

	rtcHandle.Instance = RTC;
	rtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
	rtcHandle.Init.AsynchPrediv = 124;                  // 124 for 1 second
	rtcHandle.Init.SynchPrediv = 255;                   // 255 for 1 second
	rtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	rtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	rtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	HAL_RTC_Init(&rtcHandle);
}

void RTCUtils_GetDateTime(RTC_TimeTypeDef *dTime, RTC_DateTypeDef *dDate) {
	HAL_RTC_GetTime(&rtcHandle, dTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&rtcHandle, dDate, RTC_FORMAT_BIN);
}

uint32_t RTCUtils_GetEpochTimestamp() {
	uint8_t a;
	uint16_t y;
	uint8_t m;
	uint32_t JDN;

	RTC_DateTypeDef dDate;
	RTC_TimeTypeDef dTime;
	RTCUtils_GetDateTime(&dTime, &dDate);

	// These hardcore math's are taken from http://en.wikipedia.org/wiki/Julian_day

	// Calculate some coefficients
	a = (14 - dDate.Month) / 12;
	y = (dDate.Year + 2000) + 4800 - a; // years since 1 March, 4801 BC
	m = dDate.Month + (12 * a) - 3; // since 1 March, 4801 BC

	// Gregorian calendar date compute
	JDN = dDate.Date;
	JDN += (153 * m + 2) / 5;
	JDN += 365 * y;
	JDN += y / 4;
	JDN += -y / 100;
	JDN += y / 400;
	JDN = JDN - 32045;
	JDN = JDN - JULIAN_DATE_BASE;    // Calculate from base date
	JDN *= 86400;                     // Days to seconds
	JDN += dTime.Hours * 3600;    // ... and today seconds
	JDN += dTime.Minutes * 60;
	JDN += dTime.Seconds;

	return JDN;
}

void RTCUtils_SetEpochTimestamp(uint32_t timestamp) {
	timestamp -= 3600;

	uint32_t tm;
	uint32_t t1;
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t m;
	int16_t year = 0;
	int16_t month = 0;
	int16_t dow = 0;
	int16_t mday = 0;
	int16_t hour = 0;
	int16_t min = 0;
	int16_t sec = 0;
	uint64_t JD = 0;
	uint64_t JDN = 0;
	RTC_TimeTypeDef dTime;
	RTC_DateTypeDef dDate;

	// These hardcore math's are taken from http://en.wikipedia.org/wiki/Julian_day

	JD = ((timestamp + 43200) / (86400 >> 1)) + (2440587 << 1) + 1;
	JDN = JD >> 1;

	tm = timestamp;
	t1 = tm / 60;
	sec = tm - (t1 * 60);
	tm = t1;
	t1 = tm / 60;
	min = tm - (t1 * 60);
	tm = t1;
	t1 = tm / 24;
	hour = tm - (t1 * 24);

	dow = JDN % 7;
	a = JDN + 32044;
	b = ((4 * a) + 3) / 146097;
	c = a - ((146097 * b) / 4);
	d = ((4 * c) + 3) / 1461;
	e = c - ((1461 * d) / 4);
	m = ((5 * e) + 2) / 153;
	mday = e - (((153 * m) + 2) / 5) + 1;
	month = m + 3 - (12 * (m / 10));
	year = (100 * b) + d - 4800 + (m / 10);

	dDate.Year = year - 2000;
	dDate.Month = month;
	dDate.Date = mday;
	dDate.WeekDay = dow;
	dTime.Hours = hour;
	dTime.Minutes = min;
	dTime.Seconds = sec;

	HAL_RTC_SetDate(&rtcHandle, &dDate, RTC_FORMAT_BIN);
	HAL_RTC_SetTime(&rtcHandle, &dTime, RTC_FORMAT_BIN);
}
