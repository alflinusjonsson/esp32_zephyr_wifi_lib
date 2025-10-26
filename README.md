# ESP32 Zephyr Wi-Fi Helper Library

Lightweight helper module for Zephyr on ESP32 that:

* Brings up the ESP32 station interface (STA) and maintains the connection (simple reconnect loop).
* Publishes network connectivity state changes (connected / disconnected) on a zbus channel: `network_chan`.
* Provides a small, direct API (`wifi_init()`, `wifi_connect()`) so the application can start Wi‑Fi early in `main()`.
* Lets other subsystems (e.g. UDP socket handler, sensors) react to connectivity using either static listeners or dynamic observers.

> The library does **not** enable the core networking stack for you; the application controls global Kconfig symbols (e.g. `CONFIG_WIFI`, `CONFIG_WIFI_ESP32`). The library only depends on them via `depends on` to stay non‑intrusive.

---

## 1. Features

| Capability | Details |
|------------|---------|
| Wi‑Fi init/connect | Uses Zephyr net_mgmt to connect with SSID/PSK at boot |
| IPv4 status tracking | Listens for address add/remove; treats address presence as CONNECTED |
| Reconnect logic | Delay work item (`WIFI_LIB_RECONNECT_DELAY_MS`) reschedules connect after disconnect |
| Event publication | Publishes `enum network_status` over `network_chan` via zbus |
| zbus friendly | Channel defined with `ZBUS_OBSERVERS_EMPTY` so application can attach dynamic observers |
| Minimal surface | No background thread beyond what net_mgmt uses |

---

## 3. Integration Methods

### 3.1 One-off / Local Path (ZEPHYR_EXTRA_MODULES)

From your application root:

```
west build -p -b esp32_devkitc/esp32/procpu \
	-- -DZEPHYR_EXTRA_MODULES="/absolute/path/to/esp32_zephyr_wifi_lib" \
		 -DCONFIG_WIFI_SSID="YourSSID" \
		 -DCONFIG_WIFI_PSK="YourPSK"
```

### 3.2 west manifest (preferred for teams)

Add to your `west.yml` (at the same level as `zephyr`):

```yaml
manifest:
	projects:
		- name: esp32_zephyr_wifi_lib
			path: modules/esp32_zephyr_wifi_lib
			url: <your-git-url>
```

After `west update`, CMake auto-discovers the module via `module.yml`.

### 3.3 As a Git submodule

Place under (example) `modules/esp32_zephyr_wifi_lib` and set `ZEPHYR_EXTRA_MODULES=modules/esp32_zephyr_wifi_lib` if not using west manifest.

---

## 4. Kconfig Options (Library)

| Symbol | Type | Default | Purpose |
|--------|------|---------|---------|
| `WIFI_LIB` | bool | y | Master enable for the library code |
| `WIFI_LIB_RECONNECT_DELAY_MS` | int | 5000 | Delay before reconnect after disconnect (ms) |
| `WIFI_SSID` | string | (none) | SSID (app may override on command line) |
| `WIFI_PSK` | string | (none) | PSK (app may override on command line) |

### Application Responsibility
The application must enable (in `prj.conf` or its own `Kconfig`):

```
CONFIG_NETWORKING=y
CONFIG_NET_IPV4=y
CONFIG_NET_MGMT=y
CONFIG_WIFI=y
CONFIG_WIFI_ESP32=y
CONFIG_NET_L2_WIFI_MGMT=y
CONFIG_ZBUS=y
```

If you use dynamic observer registration (recommended):
```
CONFIG_ZBUS_RUNTIME_OBSERVERS=y
```

Credentials can be provided either in `prj.conf`, a private overlay, or via the build command `-DCONFIG_WIFI_SSID=... -DCONFIG_WIFI_PSK=...`.

> Best practice: keep real credentials out of version control; use a `wifi_secrets.conf` ignored by git.

---

## 5. Public Events (`wifi_events.h`)

### zbus Channel (``)

```c
enum network_status { NETWORK_CONNECTED, NETWORK_DISCONNECTED };
ZBUS_CHAN_DECLARE(network_chan);
```

Message payload: a single `enum network_status` value (published only on state change).

---
