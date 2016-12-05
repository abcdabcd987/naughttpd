#pragma once

#include "http_request.hpp"

void close_request(HTTPRequest *r);
void do_request(HTTPRequest *r);
