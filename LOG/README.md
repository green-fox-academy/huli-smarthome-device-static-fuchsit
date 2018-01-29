# LOG library

This library helps to log messages on a unified way and make it possible to turn the logging on and off when it's necessary. The concept is that every method logs an **ENTER** message when the execution enters in the function and logs an **EXIT** with the result code when you leave the method. Between these messages you can optionally log individual messages.

The library logs the messages using ```printf``` function.

# Usage

The **ENTER** and **EXIT** methods suport to append a module name as well in order to distinguish easily the log messages of the different modules.

So if you're working on a module, which works with json files, you can use the LOG as follows:

```c

#include "smarthome_log.h" // include the log module

// define helper macros to shorten the log statements
#define JSON_ENTER(fnc)             SHOME_LogEnter("json", fnc)
#define JSON_MSG(fnc, ...)          SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define JSON_EXIT(fnc, rc, fail)    SHOME_LogExit("json", fnc, rc, fail)

int json_Init() {
    JSON_ENTER("json_Init");

    int rc = ...; // execute some statement which may fail
    if (rc != 0) {
        JSON_MSG("ERROR - xxx returned error %d\r\n", rc);
        JSON_EXIT("json_Init", rc, 1);
        return rc;
    }

    JSON_EXIT("json_Init", 0, 0);
    return 0;
}

```

If you're redirecting printf to the serial port, you can read something like this on success:

```
json > json_Init() - ENTER
json > json_Init() - EXIT(rc=0)
```

And on failure:

```
json > json_Init() - ENTER
ERROR - xxx returned error -3
json > json_Init() - FAIL(rc=-3)
```