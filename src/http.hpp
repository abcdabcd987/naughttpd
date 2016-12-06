#pragma once

#include "http_request.hpp"

enum DoRequestResult {
    DO_REQUEST_AGAIN,
    DO_REQUEST_CLOSE
};

void close_request(HTTPRequest *r);
DoRequestResult do_request(HTTPRequest *r);
