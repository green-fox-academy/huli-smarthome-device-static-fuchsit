#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "jsmn.h"
#include "jsmn_extension.h"
#include "device_info.h"
#include "heartbeat.h"
#include "rgb_led_color.h"
#include "aircondi.h"
#include "google_iot.h"
#include "smart_plug.h"

/*
 * controls the main function, state updating, reporting
 *
 */
void main_control();

/*
 * publishing back device specific information back to ggl core's
 * state topic
 */
void report_status_color ();
void report_fan_state_and_temperature ();
void report_status_plug ();
