#pragma once
#include <map>
#include <string>
#include <ostream>
#include "util.hpp"

enum HTTPMethod {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST
};

struct HTTPRequest {
    // constants
    static constexpr size_t BUF_SIZE = 1024;

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
    bool keep_alive;

    // engine data
    int fd_socket;
    union EngineData {
        int fd_epoll;
    };
    EngineData ngdata;

    // funcs
    HTTPRequest();
    ~HTTPRequest();
    void clear();
};

std::ostream &operator<<(std::ostream &out, const HTTPRequest &r);
