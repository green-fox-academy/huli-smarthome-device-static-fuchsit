#ifndef SMARTHOME_LOG_H
#define SMARTHOME_LOG_H

#include <stdint.h>

void SHOME_DebugEnable();

void SHOME_DebugDisable();

void SHOME_LogEnter(const char* module, const char *func);

void SHOME_LogExit(const char* module, const char *func, int rc, int failed);

void SHOME_LogMsg(const char *fmt, ...);

#endif // SMARTHOME_LOG_H
