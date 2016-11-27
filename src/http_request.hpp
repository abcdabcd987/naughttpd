#pragma once
#include <map>
#include <string>
#include "http.hpp"
#include "util.hpp"

struct HTTPRequest {
    // constants
    static constexpr size_t BUF_SIZE = 8192;

    // buffer
    char buf[BUF_SIZE];
    size_t buf_head;
    size_t buf_tail;

    // parser temporary variables
    int parser_state;
    std::string parser_method;
    ci_string parser_hdr_key;
    std::string parser_hdr_value;

    // request info
    HTTPMethod method;
    std::string uri; // uri = request_path + "?" + query_string + "#" + fragment
    std::string request_path;
    std::string query_string;
    std::string fragment;
    unsigned short http_version_major;
    unsigned short http_version_minor;
    std::map<ci_string, std::string> headers;
    std::string body;

    // constructor & destructor
    HTTPRequest();
    ~HTTPRequest();
};
