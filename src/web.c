#include "web.h"

#include "hardware.h"
#include "kinematics.h"
#include "tft.h"
#include <esp_check.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <socket.h>
#include <string.h>

typedef struct web_state
{
  bool off;
  int16_t left;
  int16_t right;
  uint16_t x;
  uint16_t j2;
  uint16_t j3;
  int32_t drive_priority_fd;
  int64_t drive_priority_until;
  int32_t arm_priority_fd;
  int64_t arm_priority_until;
  int32_t override_fd;
  int64_t overriden_until;
  uint16_t drive_speed;
} web_state;

static web_state webState = {
    .off = false,
    .left = 0,
    .right = 0,
    .x = 0,
    .j2 = 0,
    .j3 = 0,
    .drive_priority_fd = 0,
    .drive_priority_until = 0,
    .arm_priority_fd = 0,
    .arm_priority_until = 0,
    .override_fd = 0,
    .overriden_until = 0,
    .drive_speed = UINT16_MAX,
};

DMA_ATTR uint16_t pixels[PIXELS_LENGTH];
static esp_err_t display_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG_WEB, "/upload %i", req->content_len);
  if (req->content_len > LCD_WIDTH * LCD_HEIGHT * 2)
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File too large.");

  size_t remainingBytes = req->content_len;
  int receivedBytes;
  uint8_t part = 0;
  while (remainingBytes > 0)
  {
    size_t bytesInPart = PIXELS_BYTES - (remainingBytes % PIXELS_BYTES);
    size_t pixelOffset = 0;
    ESP_LOGI(TAG_WEB, "Remaining bytes: %zu, bytes in part: %zu",
             remainingBytes, bytesInPart);
    while (pixelOffset < bytesInPart)
    {
      ESP_LOGI(TAG_WEB,
               "Remaining bytes: %zu, reading %zu bytes into pixels + %zu",
               remainingBytes, bytesInPart - pixelOffset, pixelOffset);
      receivedBytes = httpd_req_recv(req, (char *)&pixels + pixelOffset,
                                     bytesInPart - pixelOffset);
      if (receivedBytes <= 0)
      {
        if (receivedBytes == HTTPD_SOCK_ERR_TIMEOUT)
          continue; // Retry read
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to receive file.");
        return ESP_FAIL;
      }
      pixelOffset += receivedBytes;
      remainingBytes -= receivedBytes;
    }
    ESP_LOGI(TAG_WEB, "Sending part %i", part);
    tft_send_image_part(part, pixels);
    part++;
  }

  return ESP_OK;
}

httpd_uri_t displayConfig = {
    .uri = "/display",
    .method = HTTP_POST,
    .handler = display_handler,
    .user_ctx = NULL,
};

static esp_err_t root_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG_WEB, "Request %s", req->uri);

  const char *fileStart = NULL;
  const char *fileEnd = NULL;
  if (strcmp(req->uri, "/") == 0 || strcmp(req->uri, "/index.html") == 0)
  {
    fileStart = FILE_HTML_START;
    fileEnd = FILE_HTML_END;
    httpd_resp_set_type(req, "text/html");
  }
  else if (strcmp(req->uri, "/B612Mono.woff2") == 0)
  {
    fileStart = FILE_FONT_START;
    fileEnd = FILE_FONT_END;
    httpd_resp_set_type(req, "font/woff2");
  }
  else if (strcmp(req->uri, "/images.js") == 0)
  {
    fileStart = FILE_IMAGE_JS_START;
    fileEnd = FILE_IMAGE_JS_END;
    httpd_resp_set_type(req, "text/javascript");
  }
  else if (strcmp(req->uri, "/main.js") == 0)
  {
    fileStart = FILE_JS_START;
    fileEnd = FILE_JS_END;
    httpd_resp_set_type(req, "text/javascript");
  }
  else if (strcmp(req->uri, "/style.css") == 0)
  {
    fileStart = FILE_CSS_START;
    fileEnd = FILE_CSS_END;
    httpd_resp_set_type(req, "text/css");
  }
  else
  {
    ESP_LOGE(TAG_WEB, "Failed to find %s", req->uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
    return ESP_FAIL;
  }

  if (httpd_resp_send(req, fileStart, fileEnd - fileStart) != ESP_OK)
  {
    ESP_LOGE(TAG_WEB, "File sending failed!");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to send file");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG_WEB, "File sending complete");
  return ESP_OK;
}

static const httpd_uri_t rootConfig = {
    .uri = "/?*",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL,
};

// Update override state and return true if command accepted.
static bool handle_override(int32_t fd, bool override, int32_t *overrideFD,
                            int64_t *overriddenUntil, int64_t now)
{
  if (override)
  {
    // Received override command, renew override and accept command.
    *overrideFD = fd;
    *overriddenUntil = now + OVERRIDE_TIMEOUT;
    return true;
  }
  if (fd == *overrideFD)
  {
    // overrideFD is no longer overriding.
    *overriddenUntil = 0;
  }
  if (now < *overriddenUntil)
  {
    // Override active, ignore non-override command.
    return false;
  }
  // Override inactive, accept command.
  return true;
}

// Update priority state and return true if command accepted.
static bool handle_priority(int32_t fd, int32_t *priorityFD,
                            int64_t *priorityUntil, int64_t now)
{
  if (now < *priorityUntil)
  {
    // Priority active.
    if (fd == *priorityFD)
    {
      // Recieved command from proprityFD, renew priority and accept command.
      *priorityUntil = now + PRIORITY_TIMEOUT;
      return true;
    }
    // fd does not have priority while another does, ignore command.
    return false;
  }
  // Priority inactive, assign priority to fd and accept command.
  *priorityFD = fd;
  *priorityUntil = now + PRIORITY_TIMEOUT;
  return true;
}

// id: union
// 0: off
// 1: on
// 2: drive [i16 left, i16 right]
// 3: arm target angles [u16 x, u16 j1, u16 j2, u16 j3]
// 4: arm IK [u16 x, u16 y, u16 z]
// 5: display image [u8 idx]
// 6: drive speed [u16 speed]
typedef struct __attribute__((__packed__)) command
{
  uint8_t id;    // 0
  bool override; // 1
  union
  {
    struct __attribute__((__packed__))
    {
      int16_t left;  // 2
      int16_t right; // 4
    } drive;
    struct __attribute__((__packed__))
    {
      uint16_t x;  // 2
      uint16_t j2; // 4
      uint16_t j3; // 6
    } arm_angles;
    struct __attribute__((__packed__))
    {
      uint16_t x; // 2
      uint16_t y; // 4
      uint16_t z; // 6
    } arm_ik;
    struct __attribute__((__packed__))
    {
      uint8_t idx; // 2
    } display;
    struct __attribute__((__packed__))
    {
      uint16_t speed; // 2
    } drive_speed;
  };
} command; // 8 bytes

typedef struct __attribute__((__packed__)) telemetry
{
  int32_t fd;                // 0
  int32_t drive_priority_fd; // 4
  int32_t arm_priority_fd;   // 8
  int32_t override_fd;       // 12
  float esc_current;         // 16
  float cell1;               // 20
  float cell2;               // 24
  float cell3;               // 28
  int16_t l;                 // 32
  int16_t r;                 // 34
  uint16_t ax;               // 36
  uint16_t j2;               // 38
  uint16_t j3;               // 40
  int16_t x;                 // 42
  int16_t y;                 // 44
  int16_t z;                 // 46
  uint16_t drive_speed;      // 48
} telemetry;                 // 50 bytes

static esp_err_t websocket_handler(httpd_req_t *req)
{
  httpd_ws_frame_t pkt = {0};
  command rxData = {0};
  esp_err_t ret;

  int32_t fd = httpd_req_to_sockfd(req);
  char tag[20];
  snprintf(tag, sizeof(tag), "%s fd%ld", TAG_WEB, fd);

  if (req->method == HTTP_GET)
  {
    ESP_LOGI(tag, "Handshake done, the new connection was opened");
    return ESP_OK;
  }

  pkt.payload = (uint8_t *)&rxData;
  ret = httpd_ws_recv_frame(req, &pkt, 0);
  if (ret != ESP_OK)
  {
    ESP_LOGE(tag, "httpd_ws_recv_frame failed with %d", ret);
    return ret;
  }
  ret = httpd_ws_recv_frame(req, &pkt, sizeof(command));
  if (ret != ESP_OK)
  {
    ESP_LOGE(tag, "httpd_ws_recv_frame failed with %d", ret);
    return ret;
  }

  // rx_data[0]: name [rx_data[1...], ...]
  // 0: off
  // 1: on
  // 2: drive [bool override, i16 left, i16 right]
  // 3: arm angles [u16 x, u16 j1, u16 j2, u16 j3]
  // 4: arm IK [u16 x, u16 y, u16 z]
  int64_t now = esp_timer_get_time();
  bool accept_based_on_override =
      handle_override(fd, rxData.override, &webState.override_fd,
                      &webState.overriden_until, now);

  switch (rxData.id)
  {
  case 0:
    if (accept_based_on_override)
      webState.off = true;
    break;
  case 1:
    if (accept_based_on_override)
      webState.off = false;
    break;
  case 2:
    // Update priority only if override inactive.
    if (accept_based_on_override &&
        (rxData.override ||
         handle_priority(fd, &webState.drive_priority_fd,
                         &webState.drive_priority_until, now)))
    {
      webState.left = rxData.drive.left;
      webState.right = rxData.drive.right;
    }
    break;
  case 3:
    // Update priority only if override inactive.
    if (accept_based_on_override &&
        (rxData.override ||
         handle_priority(fd, &webState.arm_priority_fd,
                         &webState.arm_priority_until, now)))
    {
      webState.x = rxData.arm_angles.x;
      webState.j2 = rxData.arm_angles.j3;
      webState.j3 = rxData.arm_angles.j3;
    }
    break;
  case 4:
    // Update priority only if override inactive.
    if (accept_based_on_override &&
        (rxData.override ||
         handle_priority(fd, &webState.arm_priority_fd,
                         &webState.arm_priority_until, now)))
    {
      ik_calculate_angles(rxData.arm_ik.x, rxData.arm_ik.y, rxData.arm_ik.z,
                          &webState.x, &webState.j2, &webState.j3);
    }
    break;
  case 5:
    if (accept_based_on_override)
      tft_draw_image(rxData.display.idx, pixels);
    break;
  case 6:
    webState.drive_speed = rxData.drive_speed.speed;
  }

  return ESP_OK;
}

static const httpd_uri_t websocketConfig = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true,
};

static telemetry txData = {0};
void send_telemetry(httpd_handle_t server)
{
  httpd_ws_frame_t pkt = {
      .final = false,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_BINARY,
      .payload = (uint8_t *)&txData,
      .len = sizeof(telemetry),
  };

  int fds[CONFIG_LWIP_MAX_LISTENING_TCP] = {0};
  size_t fdCount;
  char tag[20];
  if (httpd_get_client_list(server, &fdCount, fds) == ESP_OK)
  {
    for (size_t i = 0; i < fdCount; i++)
    {
      if (httpd_ws_get_fd_info(server, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
      {
        snprintf(tag, sizeof(tag), "%s fd%d", TAG_WEB, fds[i]);
        txData.fd = fds[i];
        httpd_ws_send_frame_async(server, fds[i], &pkt);
      }
    }
  }
}

void webserver()
{
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.backlog_conn = 10;
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG_WEB, "Starting webserver on port %d.", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK)
  {
    ESP_LOGI(TAG_WEB, "Registering URI handlers.");
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &websocketConfig));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &displayConfig));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &rootConfig));

    // Send telemetry every 100ms.
    while (server != NULL)
    {
      int64_t now = esp_timer_get_time();
      // Send file descriptor(s) with priority and override unless they are
      // expired.
      txData.drive_priority_fd = now <= webState.drive_priority_until
                                     ? webState.drive_priority_fd
                                     : -1;
      txData.arm_priority_fd =
          now <= webState.arm_priority_until ? webState.arm_priority_fd : -1;
      txData.override_fd =
          now <= webState.overriden_until ? webState.override_fd : -1;

      // Needed because txData is packed and pointers may be unaligned.
      float escCurrent, cell1, cell2, cell3;
      current_sense_get(&escCurrent, &cell1, &cell2, &cell3);
      txData.esc_current = escCurrent;
      txData.cell1 = cell1;
      txData.cell2 = cell2;
      txData.cell3 = cell3;

      txData.l = webState.left;
      txData.r = webState.right;
      txData.ax = webState.x;
      txData.j2 = webState.j2;
      txData.j3 = webState.j3;

      // Needed because txData is packed and pointers may be unaligned.
      int16_t x, y, z;
      fk_calculate_position(webState.x, webState.j2, webState.j3, &x, &y, &z);
      txData.x = x;
      txData.y = y;
      txData.z = z;

      txData.drive_speed = webState.drive_speed;

      bool estop = estop_get();
      bool pms_stop = true; // TODO: set based on cell_sense_get
      buzzer_set(pms_stop);
      if (estop || pms_stop)
      {
        esc_enabled_set(false);
        motor_control_set(0, 0, 0, 0, 0);
      }
      else
      {
        esc_enabled_set(true);
        motor_control_set(webState.left, webState.right, webState.x,
                          webState.j2, webState.j3);
      }

      httpd_queue_work(server, send_telemetry, server);
      vTaskDelay(MAINLOOP_DELAY / portTICK_PERIOD_MS);
    }
    ESP_LOGE(TAG_WEB, "Main loop exited!");
  }
  else
  {
    ESP_LOGE(TAG_WEB, "Error starting server!");
  }

  while (1)
  {
  }
}
