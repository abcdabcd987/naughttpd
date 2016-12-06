// adapted from https://github.com/nodejs/http-parser/blob/master/test.c

#include <random>
#include <algorithm>
#include "catch.hpp"
#include "http.hpp"
#include "http_request.hpp"
#include "parser.hpp"
#include "util.hpp"

struct Message {
    const bool is_valid;
    const std::string raw;
    const enum HTTPMethod method;
    const std::string uri;
    const std::string request_path;
    const std::string query_string;
    const std::string fragment;
    const unsigned short http_major;
    const unsigned short http_minor;
    const std::string body;
    const std::map<ci_string, std::string> headers;
};

void test_parser(const Message &m) {
    std::random_device rd;
    HTTPRequest r;
    for (std::string::size_type i = 0; i < m.raw.size(); ) {
        size_t len = rd() % std::min(30UL, m.raw.size() - i) + 1;
        for (size_t j = 0; j < len; ++j)
            r.buf[r.buf_tail++ % HTTPRequest::BUF_SIZE] = m.raw[i + j];
        i += len;

        ParseResult res = parse(&r);
        if (res == PARSE_RESULT_INVALID) {
            REQUIRE(!m.is_valid);
            return;
        }
        REQUIRE(r.buf_head == r.buf_tail);
        if (i == m.raw.size()) {
            REQUIRE(res == PARSE_RESULT_OK);
        } else {
            REQUIRE(res == PARSE_RESULT_AGAIN);
        }
    }
    REQUIRE(m.is_valid);
    REQUIRE(r.method == m.method);
    REQUIRE(r.uri == m.uri);
    REQUIRE(r.request_path == m.request_path);
    REQUIRE(r.query_string == m.query_string);
    REQUIRE(r.fragment == m.fragment);
    REQUIRE(r.http_version_major == m.http_major);
    REQUIRE(r.http_version_minor == m.http_minor);
    REQUIRE(r.headers == m.headers);
    REQUIRE(r.body == m.body);
}

#define T(name, message) \
    TEST_CASE(name, "[parser]") { \
        test_parser(message); \
    }

T("simple single line get", (Message{
    .is_valid = true,
    .raw = "GET /hello.html HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("invalid http method", (Message{
    .is_valid = false,
    .raw = "GETT /hello.html HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_UNKNOWN,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("no space between method and uri", (Message{
    .is_valid = false,
    .raw = "GET/hello.html HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("uri not starts with slash", (Message{
    .is_valid = false,
    .raw = "GET hello.html HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("multiple space between method and uri", (Message{
    .is_valid = true,
    .raw = "GET    /hello.html HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("multiple space between uri and http", (Message{
    .is_valid = true,
    .raw = "GET /hello.html     HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("treat \\t as space", (Message{
    .is_valid = true,
    .raw = "GET \t /hello.html  \t  HTTP/1.0\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/hello.html",
    .request_path = "/hello.html",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 0,
    .body = "",
    .headers = {}
}))

T("curl get", (Message{
    .is_valid = true,
    .raw = "GET /test HTTP/1.1\r\n"
           "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
           "Host: 0.0.0.0=5000\r\n"
           "Accept: */*\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/test",
    .request_path = "/test",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
        { "User-Agent", "curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1" },
        { "Host", "0.0.0.0=5000" },
        { "Accept", "*/*" }
    }
}))

T("header key is case-insensitive", (Message{
    .is_valid = true,
    .raw = "GET /test HTTP/1.1\r\n"
           "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
           "Host: 0.0.0.0=5000\r\n"
           "Accept: */*\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/test",
    .request_path = "/test",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
        { "user-agent", "curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1" },
        { "hoSt", "0.0.0.0=5000" },
        { "acCePt", "*/*" }
    }
}))

T("firefox get", (Message{
    .is_valid = true,
    .raw = "GET /favicon.ico HTTP/1.1\r\n"
           "Host: 0.0.0.0=5000\r\n"
           "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
           "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
           "Accept-Language: en-us,en;q=0.5\r\n"
           "Accept-Encoding: gzip,deflate\r\n"
           "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
           "Keep-Alive: 300\r\n"
           "Connection: keep-alive\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/favicon.ico",
    .request_path = "/favicon.ico",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
        { "Host", "0.0.0.0=5000" },
        { "User-Agent", "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0" },
        { "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8" },
        { "Accept-Language", "en-us,en;q=0.5" },
        { "Accept-Encoding", "gzip,deflate" },
        { "Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7" },
        { "Keep-Alive", "300" },
        { "Connection", "keep-alive" }
    }
}))

T("dumbfuck", (Message{
    .is_valid = true,
    .raw = "GET /dumbfuck HTTP/1.1\r\n"
           "aaaaaaaaaaaaa:++++++++++\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/dumbfuck",
    .request_path = "/dumbfuck",
    .query_string = "",
    .fragment = "",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
        { "aaaaaaaaaaaaa",  "++++++++++" }
    }
}))

T("query_string", (Message{
    .is_valid = true,
    .raw = "GET /forums/1/topics/2375?page=1 HTTP/1.1\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/forums/1/topics/2375?page=1",
    .request_path = "/forums/1/topics/2375",
    .query_string = "page=1",
    .fragment = "",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
    }
}))

T("query_string with &", (Message{
    .is_valid = true,
    .raw = "GET /forums/1/topics/2375?page=1&sid=abcdefghijk HTTP/1.1\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/forums/1/topics/2375?page=1&sid=abcdefghijk",
    .request_path = "/forums/1/topics/2375",
    .query_string = "page=1&sid=abcdefghijk",
    .fragment = "",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
    }
}))

T("fragment", (Message{
    .is_valid = true,
    .raw = "GET /forums/1/topics/2375#posts-17408 HTTP/1.1\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/forums/1/topics/2375#posts-17408",
    .request_path = "/forums/1/topics/2375",
    .query_string = "",
    .fragment = "posts-17408",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
    }
}))

T("query_string and fragment", (Message{
    .is_valid = true,
    .raw = "GET /forums/1/topics/2375?page=1&sid=abcdefghijk#posts-17408 HTTP/1.1\r\n"
           "\r\n",
    .method = HTTP_METHOD_GET,
    .uri = "/forums/1/topics/2375?page=1&sid=abcdefghijk#posts-17408",
    .request_path = "/forums/1/topics/2375",
    .query_string = "page=1&sid=abcdefghijk",
    .fragment = "posts-17408",
    .http_major = 1,
    .http_minor = 1,
    .body = "",
    .headers = {
    }
}))

