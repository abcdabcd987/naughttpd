#include <string>
#include <vector>
#include <iostream>

#include "parser.hpp"

bool fill_request_info(HTTPRequest *r) {
    // method
    if (r->parser_method == "GET") {
        r->method = HTTP_METHOD_GET;
    } else {
        r->method = HTTP_METHOD_UNKNOWN;
        return false;
    }

    // request_path, query_string, fragment
    std::string::size_type question_mark = r->uri.find('?');
    std::string::size_type sharp = r->uri.find('#', question_mark == std::string::npos ? 0 : question_mark);
    if (question_mark != std::string::npos)
        r->request_path = r->uri.substr(0, question_mark);
    else
        r->request_path = r->uri.substr(0, sharp);
    if (question_mark != std::string::npos)
        r->query_string = r->uri.substr(question_mark + 1, sharp - question_mark - 1);
    if (sharp != std::string::npos)
        r->fragment = r->uri.substr(sharp + 1);

    auto iter = r->headers.find("Connection");
    if (r->http_version_major == 1 && r->http_version_minor == 0) {
        r->keep_alive = iter != r->headers.end() && iter->second == "Keep-Alive";
    } else {
        r->keep_alive = iter == r->headers.end();
    }

    return true;
}

ParseResult parse(HTTPRequest *r) {
    enum ParseState {
        P_BEFORE_METHOD = 0,
        P_METHOD,
        P_BEFORE_URI,
        P_URI,
        P_BEFORE_HTTP,
        P_HTTP_H,
        P_HTTP_HT,
        P_HTTP_HTT,
        P_HTTP_HTTP,
        P_HTTP_SLASH,
        P_HTTP_VERSION_MAJOR,
        P_HTTP_VERSION_DOT,
        P_HTTP_VERSION_MINOR,
        P_BEFORE_HEADER,
        P_HEADER_KEY,
        P_HEADER_COLON,
        P_HEADER_VALUE,
        P_HEADER_FINISHED
    };

#define EXPECT_CHAR(EXPECTED, SUCCESS_STATE, FAILURE_RESULT) \
    if (ch == EXPECTED) { \
        state = SUCCESS_STATE; \
        break; \
    } \
    return FAILURE_RESULT;

    ParseState state = static_cast<ParseState>(r->parser_state);
    for (size_t i = r->buf_head; i < r->buf_tail; ++i) {
        char ch = r->buf[i % HTTPRequest::BUF_SIZE];
        switch (state) {
            case P_BEFORE_METHOD:
                switch (ch) {
                    case '\r':
                    case '\n':
                        break; // ignore leading CR and LF
                    default:
                        state = P_METHOD;
                        r->parser_method.clear();
                        r->parser_method += ch;
                        break;
                }
                break;

            case P_METHOD:
                if (isspace(ch)) {
                    state = P_BEFORE_URI;
                    break; // method ends
                }
                switch (ch) {
                    case '\r':
                    case '\n':
                        return PARSE_RESULT_INVALID; // shouldn't be newline
                    default:
                        r->parser_method += ch;
                        break; // concat method
                }
                break;
            
            case P_BEFORE_URI:
                if (isspace(ch))
                    break; // ignore additional spaces
                switch (ch) {
                    case '/':
                        r->uri = "/";
                        state = P_URI;
                        break; // start to match uri
                    default:
                        return PARSE_RESULT_INVALID;
                }
                break;

            case P_URI:
                if (isspace(ch)) {
                    state = P_BEFORE_HTTP;
                    break; // uri finished
                }
                r->uri += ch;
                break; // concat uri

            case P_BEFORE_HTTP:
                if (isspace(ch))
                    break; // ignore additional spaces
                switch (ch) {
                    case 'H':
                        state = P_HTTP_H;
                        break; // start to match http
                    default:
                        return PARSE_RESULT_INVALID;
                }
                break;

            case P_HTTP_H   : EXPECT_CHAR('T', P_HTTP_HT  , PARSE_RESULT_INVALID)
            case P_HTTP_HT  : EXPECT_CHAR('T', P_HTTP_HTT , PARSE_RESULT_INVALID)
            case P_HTTP_HTT : EXPECT_CHAR('P', P_HTTP_HTTP, PARSE_RESULT_INVALID)
            case P_HTTP_HTTP: EXPECT_CHAR('/', P_HTTP_SLASH, PARSE_RESULT_INVALID)
            case P_HTTP_SLASH:
                if ('0' <= ch && ch <= '9') {
                    state = P_HTTP_VERSION_MAJOR;
                    r->http_version_major = ch - '0';
                    break;
                }
                return PARSE_RESULT_INVALID;
            case P_HTTP_VERSION_MAJOR:
                if ('0' <= ch && ch <= '9') {
                    state = P_HTTP_VERSION_MAJOR;
                    r->http_version_major = r->http_version_major * 10 + ch - '0';
                    break;
                }
                if (ch == '.') {
                    state = P_HTTP_VERSION_DOT;
                    break;
                }
                return PARSE_RESULT_INVALID;
            case P_HTTP_VERSION_DOT:
                if ('0' <= ch && ch <= '9') {
                    state = P_HTTP_VERSION_MINOR;
                    r->http_version_minor = ch - '0';
                    break;
                }
                return PARSE_RESULT_INVALID;
            case P_HTTP_VERSION_MINOR:
                if ('0' <= ch && ch <= '9') {
                    state = P_HTTP_VERSION_MAJOR;
                    r->http_version_minor = r->http_version_minor * 10 + ch - '0';
                    break;
                }
                if (ch == '\r')
                    break; // tolerate both LF and CRLF, thus ignore CR
                if (ch == '\n') {
                    state = P_BEFORE_HEADER;
                    break; // start to parse header line
                }
                return PARSE_RESULT_INVALID;

            case P_BEFORE_HEADER:
                switch (ch) {
                    case '\r':
                        break; // tolerate both LF and CRLF, thus ignore CR
                    case '\n':
                        state = P_HEADER_FINISHED;
                        break; // an empty line, finished!
                    default:
                        state = P_HEADER_KEY;
                        r->parser_hdr_key.clear();
                        r->parser_hdr_key += ch;
                        break; // start to read header key
                }
                break;
            case P_HEADER_KEY:
                switch (ch) {
                    case ':':
                        state = P_HEADER_COLON;
                        break; // header key ends
                    case '\r':
                    case '\n':
                        return PARSE_RESULT_INVALID; // shouldn't be newline
                    default:
                        r->parser_hdr_key += ch;
                        break; // concat header key
                }
                break;
            case P_HEADER_COLON:
                if (isspace(ch))
                    break; // ignore spaces
                switch (ch) {
                    case '\r':
                    case '\n':
                        return PARSE_RESULT_INVALID; // shouldn't be newline
                    default:
                        r->parser_hdr_value.clear();
                        r->parser_hdr_value += ch;
                        state = P_HEADER_VALUE;
                        break; // start to read header value
                }
                break;
            case P_HEADER_VALUE:
                switch (ch) {
                    case '\r':
                        break; // tolerate both LF and CRLF, thus ignore CR
                    case '\n':
                        state = P_BEFORE_HEADER;
                        if (r->headers.find(r->parser_hdr_key) != r->headers.end())
                            return PARSE_RESULT_INVALID; // duplicated headers
                        r->headers[r->parser_hdr_key] = r->parser_hdr_value;
                        break; // a header ends. look for more.
                    default:
                        r->parser_hdr_value += ch;
                        break; // concat header value
                }
                break;

            case P_HEADER_FINISHED:
                // for now, there shouldn't be any char after header ends
                return PARSE_RESULT_INVALID;

            default:
                return PARSE_RESULT_INVALID;
        }
    }

    r->parser_state = state;
    r->buf_head = r->buf_tail;

    if (state == P_HEADER_FINISHED) {
        if (!fill_request_info(r))
            return PARSE_RESULT_INVALID;
        return PARSE_RESULT_OK;
    }
    return PARSE_RESULT_AGAIN;
}
