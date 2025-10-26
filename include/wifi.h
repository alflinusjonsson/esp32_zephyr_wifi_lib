#ifndef WIFI_H
#define WIFI_H

#include <zephyr/net/wifi_mgmt.h>

/**
 * @brief Initialize Wi-Fi and register event callbacks
 * @return true if initialization was successful, false otherwise
 */
bool wifi_init(void);

/**
 * @brief Connect to the configured Wi-Fi network
 * @return true if the connection attempt was initiated successfully, false otherwise
 */
bool wifi_connect(void);

/**
 * @brief Disconnect from the current Wi-Fi network
 * @return true if the disconnection was successful, false otherwise
 */
bool wifi_disconnect(void);

/**
 * @brief Handle Wi-Fi management events
 *
 * @param cb Pointer to the net_mgmt_event_callback structure
 * @param mgmt_event The management event type
 * @param iface Pointer to the network interface
 */
void on_wifi_mgmt_event(struct net_mgmt_event_callback *cb,
                        uint64_t mgmt_event,
                        struct net_if *iface);

/**
 * @brief Handle IPv4 management events
 * @param cb Pointer to the net_mgmt_event_callback structure
 * @param mgmt_event The management event type
 * @param iface Pointer to the network interface
 */
void on_ipv4_mgmt_event(struct net_mgmt_event_callback *cb,
                        uint64_t mgmt_event,
                        struct net_if *iface);

#endif /* WIFI_H */
