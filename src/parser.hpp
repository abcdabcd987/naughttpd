#pragma once
#include "http_request.hpp"

enum ParseResult {
    PARSE_RESULT_INVALID,
    PARSE_RESULT_AGAIN,
    PARSE_RESULT_OK
};

ParseResult parse(HTTPRequest *r);
