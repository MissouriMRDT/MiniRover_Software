#include <esp_check.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <string.h>

#define WIFI_SSID "MiniRover"
#define WIFI_PASS "nandgate"
#define WIFI_CHANNEL 4
#define MAX_STATION_CONNECTIONS 10
#define GTK_REKEY_INTERVAL 0

static const char *TAG = "MiniRover";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station connected mac: " MACSTR ", aid: %d",
             MAC2STR(event->mac), event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station disconnected mac: " MACSTR ", aid: %d, reason: %d",
             MAC2STR(event->mac), event->aid, event->reason);
  }
}

void wifi_init_softap(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

  esp_netif_ip_info_t IP_settings_ap;
  IP_settings_ap.ip.addr = 0x0104a8c0;      // 192.168.4.1
  IP_settings_ap.netmask.addr = 0x00ffffff; // 255.255.255.0
  IP_settings_ap.gw.addr = 0x0104a8c0;      // 192.168.4.1

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
  esp_netif_set_ip_info(esp_netif_ap, &IP_settings_ap);
  ESP_ERROR_CHECK(esp_netif_dhcps_option(
      esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI,
      "http://192.168.4.1", strlen("http://192.168.4.1")));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = WIFI_SSID,
              .ssid_len = strlen(WIFI_SSID),
              .channel = WIFI_CHANNEL,
              .password = WIFI_PASS,
              .max_connection = MAX_STATION_CONNECTIONS,
              .authmode = WIFI_AUTH_WPA3_PSK,
              .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
              .pmf_cfg =
                  {
                      .required = true,
                  },
              .gtk_rekey_interval = GTK_REKEY_INTERVAL,
          },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG,
           "wifi_init_softap finished. SSID: %s, password: %s, channel: %d.",
           WIFI_SSID, WIFI_PASS, WIFI_CHANNEL);
}

extern const uint8_t FILE_FONT_START[] asm("_binary_B612Mono_wolff2_start");
extern const uint8_t FILE_FONT_END[] asm("_binary_B612Mono_wolff2_end");
extern const uint8_t FILE_HTML_START[] asm("_binary_index_html_start");
extern const uint8_t FILE_HTML_END[] asm("_binary_index_html_end");
extern const uint8_t FILE_JS_START[] asm("_binary_main_js_start");
extern const uint8_t FILE_JS_END[] asm("_binary_main_js_end");
extern const uint8_t FILE_CSS_START[] asm("_binary_style_css_start");
extern const uint8_t FILE_CSS_END[] asm("_binary_style_css_end");

#define SEND_CHUNK_SIZE 8192
static esp_err_t root_handler(httpd_req_t *req) {
  size_t chunksize;
  uint8_t *fileStart = NULL;
  uint8_t *fileEnd = NULL;
  if (strcmp(req->uri, "/") == 0 || strcmp(req->uri, "/index.html") == 0) {
    fileStart = FILE_HTML_START;
    fileEnd = FILE_HTML_END;
  } else if (strcmp(req->uri, "/B612Mono.woff2") == 0) {
    fileStart = FILE_FONT_START;
    fileEnd = FILE_FONT_END;
  } else if (strcmp(req->uri, "/main.js") == 0) {
    fileStart = FILE_JS_START;
    fileEnd = FILE_JS_END;
  } else if (strcmp(req->uri, "/style.css") == 0) {
    fileStart = FILE_CSS_START;
    fileEnd = FILE_CSS_END;
  } else {
    ESP_LOGE(TAG, "Failed to find %s", req->uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
    return ESP_FAIL;
  }

  while (fileStart < fileEnd) {
    if (httpd_resp_send_chunk(req, fileStart,
                              fileEnd - fileStart < SEND_CHUNK_SIZE
                                  ? fileEnd - fileStart
                                  : SEND_CHUNK_SIZE) != ESP_OK) {
      ESP_LOGE(TAG, "File sending failed!");
      httpd_resp_sendstr_chunk(req, NULL);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to send file");
      return ESP_FAIL;
    }
    fileStart += SEND_CHUNK_SIZE;
  }
  ESP_LOGI(TAG, "File sending complete");
  return ESP_OK;
}

static const httpd_uri_t root_config = {
    .uri = "/*", .method = HTTP_GET, .handler = root_handler, .user_ctx = NULL};

// id: union
// 0: off
// 1: on
// 2: drive [bool override, i16 left, i16 right]
// 3: arm target angles [bool override, u16 x, u16 j1, u16 j2, u16 j3]
// 4: arm IK [bool override, u16 x, u16 y, u16 z]
typedef struct __attribute__((__packed__, scalar_storage_order("big-endian")))
command {
  uint8_t id;
  union {
    struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
      bool override;
      int16_t left;
      int16_t right;
    } drive;
    struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
      bool override;
      uint16_t x;
      uint16_t j1;
      uint16_t j2;
      uint16_t j3;
    } armAngles;
    struct __attribute__((__packed__, scalar_storage_order("big-endian"))) {
      bool override;
      uint16_t x;
      uint16_t y;
      uint16_t z;
    } armIK;
  };
} command;

typedef struct __attribute__((__packed__, scalar_storage_order("big-endian")))
telemetry {
  float batteryVoltage;
  float batteryCurrent;
  float cell1;
  float cell2;
  float cell3;
  float cell4;
  int16_t l;
  int16_t r;
  uint16_t ax;
  uint16_t j1;
  uint16_t j2;
  uint16_t j3;
  uint16_t x;
  uint16_t y;
  uint16_t z;
} telemetry;

static esp_err_t websocket_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "Handshake done, the new connection was opened");
    return ESP_OK;
  }

  while (true) {
    httpd_ws_frame_t pkt;
    command rxData;
    telemetry txData;

    pkt.type = HTTPD_WS_TYPE_BINARY;
    pkt.payload = (uint8_t *)&rxData;
    esp_err_t ret = httpd_ws_recv_frame(req, &pkt, sizeof(rxData));
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      return ret;
    }

    // rx_data[0]: name [rx_data[1...], ...]
    // 0: off
    // 1: on
    // 2: drive [bool override, i16 left, i16 right]
    // 3: arm angles [u16 x, u16 j1, u16 j2, u16 j3]
    // 4: arm IK [u16 x, u16 y, u16 z]
    ESP_LOGI(TAG, "Command id: %d", rxData.id);
    switch (rxData.id) {
    case 0:
      ESP_LOGI(TAG, "Command OFF");
      break;
    case 1:
      ESP_LOGI(TAG, "Command ON");
      break;
    case 2:
      ESP_LOGI(TAG, "Command DRIVE override: %s, left: %d, right: %d",
               rxData.drive.override ? "true" : "false", rxData.drive.left,
               rxData.drive.right);
      break;
    case 3:
      ESP_LOGI(TAG,
               "Command ANGLES override: %s, x: %d, j1: %d, j2: %d, j3: %d",
               rxData.armAngles.override ? "true" : "false", rxData.armAngles.x,
               rxData.armAngles.j1, rxData.armAngles.j2, rxData.armAngles.j3);
      break;
    case 4:
      ESP_LOGI(TAG, "Command IK override: %s, x: %d, y: %d, z: %d",
               rxData.armIK.override ? "true" : "false", rxData.armIK.x,
               rxData.armIK.y, rxData.armIK.z);
      break;
    }

    txData.batteryVoltage = 12.3;
    txData.batteryCurrent = 10.1;
    txData.cell1 = 1.1;
    txData.cell2 = 1.2;
    txData.cell3 = 2.1;
    txData.cell4 = 2.2;
    txData.l = 1.0;
    txData.r = 0.53;
    txData.ax = 1.0;
    txData.j1 = 0.12;
    txData.j2 = 0.34;
    txData.j3 = 0.56;
    txData.x = 128;
    txData.y = 452;
    txData.z = 321;

    pkt.fragmented = false;
    pkt.type = HTTPD_WS_TYPE_BINARY;
    pkt.payload = (uint8_t *)&txData;
    pkt.len = sizeof(txData);

    ret = httpd_ws_send_frame(req, &pkt);
    pkt.payload = NULL;
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
      return ret;
    }
  }
}

static const httpd_uri_t websocket_config = {.uri = "/ws",
                                             .method = HTTP_GET,
                                             .handler = websocket_handler,
                                             .user_ctx = NULL,
                                             .is_websocket = true};

static httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Start the httpd server
  ESP_LOGI(TAG, "Starting webserver on port %d.", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Registering the ws handler
    ESP_LOGI(TAG, "Registering URI handlers.");
    httpd_register_uri_handler(server, &root_config);
    httpd_register_uri_handler(server, &websocket_config);
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server) {
  return httpd_stop(server);
}

void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
  wifi_init_softap();
  httpd_handle_t server = start_webserver();
}
