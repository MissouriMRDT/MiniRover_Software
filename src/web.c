#include "web.h"
#include "tft.h"
#include <esp_check.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <string.h>

DMA_ATTR uint16_t pixels[PIXELS_LENGTH];
static esp_err_t display_handler(httpd_req_t *req) {
  ESP_LOGI(TAG_WEB, "/upload %i", req->content_len);
  if (req->content_len > LCD_WIDTH * LCD_HEIGHT * 2)
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File too large.");

  size_t remaining_bytes = req->content_len;
  int received_bytes;
  uint8_t part = 0;
  while (remaining_bytes > 0) {
    size_t bytes_in_part = PIXELS_BYTES - (remaining_bytes % PIXELS_BYTES);
    size_t pixel_offset = 0;
    ESP_LOGI(TAG_WEB, "Remaining bytes: %zu, bytes in part: %zu",
             remaining_bytes, bytes_in_part);
    while (pixel_offset < bytes_in_part) {
      ESP_LOGI(TAG_WEB,
               "Remaining bytes: %zu, reading %zu bytes into pixels + %zu",
               remaining_bytes, bytes_in_part - pixel_offset, pixel_offset);
      received_bytes = httpd_req_recv(req, (char *)&pixels + pixel_offset,
                                      bytes_in_part - pixel_offset);
      if (received_bytes <= 0) {
        if (received_bytes == HTTPD_SOCK_ERR_TIMEOUT)
          continue; // Retry read
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to receive file.");
        return ESP_FAIL;
      }
      pixel_offset += received_bytes;
      remaining_bytes -= received_bytes;
    }
    ESP_LOGI(TAG_WEB, "Sending part %i", part);
    tft_send_image_part(part, pixels);
    part++;
  }

  return ESP_OK;
}

httpd_uri_t display_config = {.uri = "/display",
                              .method = HTTP_POST,
                              .handler = display_handler,
                              .user_ctx = NULL};

static esp_err_t root_handler(httpd_req_t *req) {
  ESP_LOGI(TAG_WEB, "Request %s", req->uri);

  const char *fileStart = NULL;
  const char *fileEnd = NULL;
  if (strcmp(req->uri, "/") == 0 || strcmp(req->uri, "/index.html") == 0) {
    fileStart = FILE_HTML_START;
    fileEnd = FILE_HTML_END;
    httpd_resp_set_type(req, "text/html");
  } else if (strcmp(req->uri, "/B612Mono.woff2") == 0) {
    fileStart = FILE_FONT_START;
    fileEnd = FILE_FONT_END;
    httpd_resp_set_type(req, "font/woff2");
  } else if (strcmp(req->uri, "/main.js") == 0) {
    fileStart = FILE_JS_START;
    fileEnd = FILE_JS_END;
    httpd_resp_set_type(req, "text/javascript");
  } else if (strcmp(req->uri, "/style.css") == 0) {
    fileStart = FILE_CSS_START;
    fileEnd = FILE_CSS_END;
    httpd_resp_set_type(req, "text/css");
  } else {
    ESP_LOGE(TAG_WEB, "Failed to find %s", req->uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
    return ESP_FAIL;
  }

  if (httpd_resp_send(req, fileStart, fileEnd - fileStart) != ESP_OK) {
    ESP_LOGE(TAG_WEB, "File sending failed!");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to send file");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG_WEB, "File sending complete");
  return ESP_OK;
}

static const httpd_uri_t root_config = {.uri = "/?*",
                                        .method = HTTP_GET,
                                        .handler = root_handler,
                                        .user_ctx = NULL};

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
  httpd_ws_frame_t pkt = {0};
  command rxData = {0};
  telemetry txData;
  esp_err_t ret;

  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG_WEB, "Handshake done, the new connection was opened");
    return ESP_OK;
  }

  pkt.payload = (uint8_t *)&rxData;
  ret = httpd_ws_recv_frame(req, &pkt, 0);
  ESP_LOGI(TAG_WEB, "rx length %d", pkt.len);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_WEB, "httpd_ws_recv_frame failed with %d", ret);
    return ret;
  }
  ret = httpd_ws_recv_frame(req, &pkt, sizeof(command));
  for (size_t i = 0; i < sizeof(command); i++) {
    printf("%02x ", pkt.payload[i]);
  }
  printf("\n");
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_WEB, "httpd_ws_recv_frame failed with %d", ret);
    return ret;
  }

  // rx_data[0]: name [rx_data[1...], ...]
  // 0: off
  // 1: on
  // 2: drive [bool override, i16 left, i16 right]
  // 3: arm angles [u16 x, u16 j1, u16 j2, u16 j3]
  // 4: arm IK [u16 x, u16 y, u16 z]
  ESP_LOGI(TAG_WEB, "Command id: %d", rxData.id);
  switch (rxData.id) {
  case 0:
    ESP_LOGI(TAG_WEB, "Command OFF");
    break;
  case 1:
    ESP_LOGI(TAG_WEB, "Command ON");
    break;
  case 2:
    ESP_LOGI(TAG_WEB, "Command DRIVE override: %s, left: %d, right: %d",
             rxData.drive.override ? "true" : "false", rxData.drive.left,
             rxData.drive.right);
    break;
  case 3:
    ESP_LOGI(TAG_WEB,
             "Command ANGLES override: %s, x: %d, j1: %d, j2: %d, j3: %d",
             rxData.armAngles.override ? "true" : "false", rxData.armAngles.x,
             rxData.armAngles.j1, rxData.armAngles.j2, rxData.armAngles.j3);
    break;
  case 4:
    ESP_LOGI(TAG_WEB, "Command IK override: %s, x: %d, y: %d, z: %d",
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
  txData.l = -0.5 * 0x7000;
  txData.r = 0.63 * 0x7000;
  txData.ax = 0.12 * 0x10000;
  txData.j1 = 0.34 * 0x10000;
  txData.j2 = 0.56 * 0x10000;
  txData.j3 = 0.78 * 0x10000;
  txData.x = 128;
  txData.y = 452;
  txData.z = 321;

  pkt.fragmented = false;
  pkt.type = HTTPD_WS_TYPE_BINARY;
  pkt.payload = (uint8_t *)&txData;
  pkt.len = sizeof(telemetry);
  ESP_LOGI(TAG_WEB, "tx length %d", pkt.len);
  for (size_t i = 0; i < pkt.len; i++) {
    printf("%02x ", pkt.payload[i]);
  }
  printf("\n");

  ret = httpd_ws_send_frame(req, &pkt);
  pkt.payload = NULL;
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_WEB, "httpd_ws_send_frame failed with %d", ret);
    return ret;
  }

  return ESP_OK;
}

static const httpd_uri_t websocket_config = {.uri = "/ws",
                                             .method = HTTP_GET,
                                             .handler = websocket_handler,
                                             .user_ctx = NULL,
                                             .is_websocket = true};

httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(TAG_WEB, "Starting webserver on port %d.", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Registering the ws handler
    ESP_LOGI(TAG_WEB, "Registering URI handlers.");
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &websocket_config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &display_config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_config));
    return server;
  }

  ESP_LOGI(TAG_WEB, "Error starting server!");
  return NULL;
}