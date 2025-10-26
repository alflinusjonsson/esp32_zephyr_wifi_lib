#include <zephyr/zbus/zbus.h>
#include "wifi_events.h"

ZBUS_CHAN_DEFINE(network_chan,
                 enum network_status,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(NETWORK_DISCONNECTED));
