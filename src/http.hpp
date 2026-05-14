#ifndef PAGER_RECEIVE_HTTP_HPP
#define PAGER_RECEIVE_HTTP_HPP

#include "esp_http_server.h"

#include "LBJ.hpp"

#include "sdlog.hpp"
#include "aPreferences.h"

extern SD_LOG sd1;
extern class aPreferences flash;


esp_err_t start_webserver(void);

#endif //PAGER_RECEIVE_HTTP_HPP
