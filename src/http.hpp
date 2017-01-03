#pragma once

#include "http_request.hpp"

enum DoRequestResult {
    DO_REQUEST_READ_AGAIN = 0,
    DO_REQUEST_WRITE_AGAIN,
    DO_REQUEST_CLOSE
};
enum SendfileMethod {
    SENDFILE_NAIVE,
    SENDFILE_MMAP,
    SENDFILE_SENDFILE
};

extern bool enable_tcp_cork;
extern bool enable_tcp_nodelay;
extern SendfileMethod sendfile_method;

void close_request(HTTPRequest *r);
DoRequestResult do_request(HTTPRequest *r);
