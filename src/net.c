#include "net.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <string.h>

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG_NET, "station connected mac: " MACSTR ", aid: %d",
             MAC2STR(event->mac), event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG_NET,
             "station disconnected mac: " MACSTR ", aid: %d, reason: %d",
             MAC2STR(event->mac), event->aid, event->reason);
  }
}

void wifi_init_softap(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_t *espNetifAP = esp_netif_create_default_wifi_ap();

  esp_netif_ip_info_t IPSettingsAP;
  IPSettingsAP.ip.addr = 0x0104a8c0;      // 192.168.4.1
  IPSettingsAP.netmask.addr = 0x00ffffff; // 255.255.255.0
  IPSettingsAP.gw.addr = 0x0104a8c0;      // 192.168.4.1

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(espNetifAP));
  esp_netif_set_ip_info(espNetifAP, &IPSettingsAP);
  ESP_ERROR_CHECK(esp_netif_dhcps_option(
      espNetifAP, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI,
      "http://192.168.4.1", strlen("http://192.168.4.1")));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(espNetifAP));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifiConfig = {
      .ap =
          {
              .ssid = WIFI_SSID,
              .ssid_len = strlen(WIFI_SSID),
              .channel = WIFI_CHANNEL,
              .password = WIFI_PASS,
              .max_connection = MAX_STATION_CONNECTIONS,
              .authmode = WIFI_AUTH_WPA2_PSK,
              .pmf_cfg =
                  {
                      .required = true,
                  },
              .gtk_rekey_interval = GTK_REKEY_INTERVAL,
          },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG_NET,
           "wifi_init_softap finished. SSID: %s, password: %s, channel: %d.",
           WIFI_SSID, WIFI_PASS, WIFI_CHANNEL);
}