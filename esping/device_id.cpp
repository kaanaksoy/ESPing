#include "device_id.h"
#include "defines.h"

String DEVICE_ID;
String DEVICE_NAME;

String BUTTON_TOPIC;
String STATE_TOPIC;
String AVAILABILITY_TOPIC;
String COMMAND_TOPIC;
String BATTERY_TOPIC;

void initDeviceId()
{
    // Derive a stable, unique ID from the chip's factory MAC. Same chip ->
    // same ID across reboots and reflashes. Different chip -> different ID.
    uint64_t mac = ESP.getEfuseMac();
    char buf[24];
    snprintf(buf, sizeof(buf), "%s-%04x%08x",
             DEVICE_PREFIX,
             (uint16_t)(mac >> 32),
             (uint32_t)mac);
    DEVICE_ID = String(buf);

    // Friendly name. Tools like HA will show this; users can rename in HA UI.
    DEVICE_NAME = String(DEVICE_PREFIX_FRIENDLY) + " " + DEVICE_ID.substring(strlen(DEVICE_PREFIX) + 1);

    BUTTON_TOPIC       = DEVICE_ID + "/button";
    STATE_TOPIC        = DEVICE_ID + "/state";
    AVAILABILITY_TOPIC = DEVICE_ID + "/availability";
    COMMAND_TOPIC      = DEVICE_ID + "/command";
    BATTERY_TOPIC      = DEVICE_ID + "/battery";
}
