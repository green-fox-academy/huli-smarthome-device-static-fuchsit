#include <stdarg.h>
#include <stdio.h>
#include "smarthome_log.h"

static uint8_t SHOME_DebugEnabled = 0;

void SHOME_DebugEnable() {
	SHOME_DebugEnabled = 1;
}

void SHOME_DebugDisable() {
	SHOME_DebugEnabled = 0;
}

void SHOME_LogEnter(const char* module, const char *func) {
	if (!SHOME_DebugEnabled) {
		return;
	}
	printf("%s > %s() - ENTER\r\n", module, func);
}

void SHOME_LogExit(const char* module, const char *func, int rc, int failed) {
	if (!SHOME_DebugEnabled) {
		return;
	}
	const char *st = failed ? "FAIL" : "EXIT";
	printf("%s > %s() - %s(rc=%d)\r\n", module, func, st, rc);
}

void SHOME_LogMsg(const char *fmt, ...) {
	if (!SHOME_DebugEnabled) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
