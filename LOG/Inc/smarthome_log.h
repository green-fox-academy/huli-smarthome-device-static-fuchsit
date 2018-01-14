#ifndef SMARTHOME_LOG_H
#define SMARTHOME_LOG_H

#include <stdint.h>

/**
 * @brief 	Enables the logging, so all the messages will be printed
 * 			using printf. The logging is disabled by default.
 */
void SHOME_DebugEnable();

/**
 * @brief 	Disables the logging, so none of the messages will be printed
 * 			using printf. The logging is disabled by default.
 */
void SHOME_DebugDisable();

/**
 * @brief	Logs an ENTER message using the parameters
 *
 * @param	module	short name of the module, which will be included in
 * 					the log message
 * @param	func	the name of the function, which will be included in
 * 					the log message
 */
void SHOME_LogEnter(const char* module, const char *func);

/**
 * @brief	Logs an EXIT message using the parameters
 *
 * @param	module	short name of the module, which will be included in
 * 					the log message
 * @param	func	the name of the function, which will be included in
 * 					the log message
 * @param	rc		the return code of the function on exit
 * @param	failed	flag, which marks if the result of the function was error
 * 					or success. (1=error)
 */
void SHOME_LogExit(const char* module, const char *func, int rc, int failed);

/**
 * @brief	Logs a message using the parameters. This is just a wrapper function
 * 			for printf.
 *
 * @param	fmt		the string format to log
 * @param	vargs	the necessary parameters to assemble the log string
 */
void SHOME_LogMsg(const char *fmt, ...);

#endif // SMARTHOME_LOG_H
