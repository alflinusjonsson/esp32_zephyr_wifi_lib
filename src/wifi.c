#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/zbus/zbus.h>
#include "wifi.h"
#include "wifi_events.h"

BUILD_ASSERT(sizeof(CONFIG_WIFI_SSID) > 0,
             "CONFIG_WIFI_SSID is empty. Please provide it to west using "
             "-DCONFIG_WIFI_SSID=<your-ssid> or set it in prj.conf.");
BUILD_ASSERT(sizeof(CONFIG_WIFI_PSK) > 0,
             "CONFIG_WIFI_PSK is empty. Please provide it to west using "
             "-DCONFIG_WIFI_PSK=<your-psk> or set it in prj.conf.");

LOG_MODULE_REGISTER(WIFI_LIB, CONFIG_LOG_DEFAULT_LEVEL);

#define NET_EVENT_WIFI_MASK (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)
#define NET_EVENT_IPV4_MASK (NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL)

static struct net_if *sta_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_callback;
static struct k_work_delayable wifi_reconnect_work;
static enum network_status network_status = NETWORK_DISCONNECTED;

static void wifi_reconnect(struct k_work *work) {
    wifi_connect();
}

bool wifi_init(void) {
    net_mgmt_init_event_callback(&wifi_cb, on_wifi_mgmt_event, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&wifi_cb);

    net_mgmt_init_event_callback(&ipv4_callback, on_ipv4_mgmt_event, NET_EVENT_IPV4_MASK);
    net_mgmt_add_event_callback(&ipv4_callback);

    sta_iface = net_if_get_wifi_sta();
    if (!sta_iface) {
        LOG_ERR("STA interface not initialized");
        return false;
    }

    sta_config.ssid = (const uint8_t *)CONFIG_WIFI_SSID;
    sta_config.ssid_length = sizeof(CONFIG_WIFI_SSID) - 1;
    sta_config.psk = (const uint8_t *)CONFIG_WIFI_PSK;
    sta_config.psk_length = sizeof(CONFIG_WIFI_PSK) - 1;
    sta_config.security = WIFI_SECURITY_TYPE_PSK;
    sta_config.channel = WIFI_CHANNEL_ANY;
    sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

    k_work_init_delayable(&wifi_reconnect_work, wifi_reconnect);

    return wifi_connect();
}

bool wifi_connect(void) {
    if (!sta_iface) {
        LOG_ERR("STA interface not initialized, wifi_init() must be called first");
        return false;
    }

    LOG_INF("Connecting to SSID: %s", CONFIG_WIFI_SSID);

    const int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT,
                             sta_iface,
                             &sta_config,
                             sizeof(struct wifi_connect_req_params));
    if (ret < 0) {
        LOG_ERR("Failed to connect: %s", strerror(errno));
        return false;
    }

    return true;
}

void on_wifi_mgmt_event(struct net_mgmt_event_callback *cb,
                        uint64_t mgmt_event,
                        struct net_if *iface) {
    enum network_status new_network_status;

    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT: {
        LOG_INF("Connected to %s. Waiting for IP address...", CONFIG_WIFI_SSID);
        return;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT: {
        LOG_INF("Disconnected from %s", CONFIG_WIFI_SSID);
        new_network_status = NETWORK_DISCONNECTED;
        k_work_schedule(&wifi_reconnect_work, K_MSEC(CONFIG_WIFI_LIB_RECONNECT_DELAY_MS));
        break;
    }
    default:
        LOG_WRN("Unhandled Wi-Fi event: %llu", mgmt_event);
        return;
    }

    if (new_network_status == network_status) {
        return;
    }

    network_status = new_network_status;
    const int ret = zbus_chan_pub(&network_chan, &network_status, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to publish network status, error: %s", strerror(ret));
    }
}

void on_ipv4_mgmt_event(struct net_mgmt_event_callback *cb,
                        uint64_t mgmt_event,
                        struct net_if *iface) {
    enum network_status new_network_status;

    switch (mgmt_event) {
    case NET_EVENT_IPV4_ADDR_ADD: {
        LOG_INF("IPv4 address acquired");
        new_network_status = NETWORK_CONNECTED;
        break;
    }
    case NET_EVENT_IPV4_ADDR_DEL: {
        LOG_INF("IPv4 address removed");
        new_network_status = NETWORK_DISCONNECTED;
        break;
    }
    default:
        LOG_WRN("Unhandled IPv4 event: %llu", mgmt_event);
        return;
    }

    if (new_network_status == network_status) {
        return;
    }

    network_status = new_network_status;
    const int ret = zbus_chan_pub(&network_chan, &network_status, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to publish network status, error: %s", strerror(ret));
    }
}
