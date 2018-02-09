/*
 * Hold definition relating to the device's opreation and configuration
 *
 */

#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H


/*
 * This enum holds the different states of the operation
 */
typedef enum State_Of_Operation {
	STATE_SSDP_DISCOVERY, // waiting for the correct ssdp discovery packet to arrive
	STATE_HTTPS_SERVER,   // on receiving GET /getDeviceParams returns device params, on POST updates device config
	STATE_GGL_CORE,     // connects and subscribes to ggl iot core in MQTT SSL mode
	STATE_MQTT,           // listens to commands from the subscribed iot core topics in test mode
	STATE_ERROR         // in case ther is an error
} State_Of_Operation;

typedef enum Device_Type {
	LED_CONTROLLER,
	COFFEE_MAKER,
	SMART_PLUG,
	AIR_CONDITIONER
} Device_Type;

/*
 * This struct will carry all configuration information for the given IoT device
 * For all device!
 */
typedef struct device_config{
    char *device_name;
    char *device_id;
    char *device_ip;
    int device_port[12];
    char *color;
    char *temperature;
    char *plug;
    Device_Type device_type;
    State_Of_Operation state_of_device;
} device_config_t;

#endif //DEVICE_INFO_H
