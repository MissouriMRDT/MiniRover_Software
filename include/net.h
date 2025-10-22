#ifndef _NET_H_
#define _NET_H_

static const char *TAG_NET = "net.c";

#define WIFI_SSID "MiniRover"
#define WIFI_PASS "nandgate"
#define WIFI_CHANNEL 4
#define MAX_STATION_CONNECTIONS 10
#define GTK_REKEY_INTERVAL 0

void wifi_init_softap(void);

#endif