#include "http.hpp"

typedef struct rest_server_context {
    int a = 0;
    // char base_path[ESP_VFS_PATH_MAX + 1];
    // char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

rest_server_context_t rest_context;

/* Simple handler for getting temperature data */
static esp_err_t test_get_handler(httpd_req_t *req) {
    // int total_len = req->content_len;
    // ((rest_server_context_t *)(req->user_ctx))->a;

    httpd_resp_set_type(req, "text/plain");

    char line[32] = {'\0'};
    sprintf(line, "id=%d", flash.getID());
    httpd_resp_sendstr_chunk(req, line);
    httpd_resp_send_chunk(req, NULL, 0);
    // free((void *)sys_info);
    // cJSON_Delete(root);
    return ESP_OK;
}

static const char html_body_header[] = "<head><meta http-equiv='content-type' content='text/html; charset=gbk'><meta name='viewport' content='width=device-width,initial-scale=1'><style>\nbody,pre{font-family:\"Courier New\",\"Helvetica Neue\",Helvetica,monospace;color:#ffffff;background-color:#000000;font-size: 12px;}</style></head>";

static esp_err_t up_handler(httpd_req_t *req) {
    // req->method;

    uint32_t id=0;
    int len = 1000;

    int ret = sscanf(req->uri, "/u/%d/%d", &id, &len);
    // Serial.printf("%d %d %d\n", ret, id, len);

    if (ret < 2) len = 1000;
    if ((ret < 1) || (id == 0)) id = flash.getID() - 1;

    httpd_resp_set_type(req, "text/html; charset=utf-8");

    httpd_resp_sendstr_chunk(req, html_body_header);

    httpd_resp_sendstr_chunk(req, "<body><pre>");

    char buffer[512] = {'\0'};

    PagerClient::poc32 poc32;
    lbj_data lbj;
    rx_info rx;
    // uint32_t id = flash.getID() - 1;
    for (int i=0; i<len; i++) {
        if (id <= 0) break;
        bool flag = flash.retrieve2(&poc32, &lbj, &rx, &id);
        if (flag && 
            '0' <= lbj.train[4] && lbj.train[4] <= '9' && 
            '0' <= lbj.speed[2] && lbj.speed[2] <= '9' && 
            '0' <= lbj.position[5] && lbj.position[5] <= '9') {

            LBJ_html(buffer, poc32, lbj, rx);
            httpd_resp_sendstr_chunk(req, buffer);
        }
        id--;

        if (id % 10) {
            vTaskDelay(1); // ms/portTICK_PERIOD_MS // avoid INT_WDT timeout
        }
    }
    httpd_resp_sendstr_chunk(req, "</pre>");
    sprintf(buffer, "<a href='/u/%d'>end</a>", id);
    httpd_resp_sendstr_chunk(req, buffer);
    httpd_resp_sendstr_chunk(req, "</body>");
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

//Function for starting the webserver
esp_err_t start_webserver(void) {
    // rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = tskIDLE_PRIORITY+1;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) != ESP_OK) {
        return ESP_FAIL;
    }

    httpd_uri_t test_get_uri = {
        .uri = "/t",
        .method = HTTP_GET,
        .handler = test_get_handler,
        .user_ctx = &rest_context
    };
    httpd_register_uri_handler(server, &test_get_uri);

    httpd_uri_t up_uri = {
        .uri = "/u",
        .method = HTTP_GET,
        .handler = up_handler,
        .user_ctx = &rest_context
    };
    httpd_register_uri_handler(server, &up_uri);

    up_uri = {
        .uri = "/u/*",
        .method = HTTP_GET,
        .handler = up_handler,
        .user_ctx = &rest_context
    };
    httpd_register_uri_handler(server, &up_uri);
    // httpd_register_uri_handler(server, &uri_post);

    return ESP_OK;
}
