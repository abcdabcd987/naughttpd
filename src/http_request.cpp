#include "http_request.hpp"

HTTPRequest::HTTPRequest(int sfd, int efd) :
        fd_socket(sfd),
        fd_epoll(efd)
{
    clear();
}


HTTPRequest::~HTTPRequest() {
    
}

void HTTPRequest::clear() {
    buf_head = 0;
    buf_tail = 0;
    parser_state = 0;
    headers.clear();
}

std::ostream &operator<<(std::ostream &out, const HTTPRequest &r) {
    out << "  method = " << r.method << std::endl
        << "  uri = " << r.uri << std::endl
        << "  request_path = " << r.request_path << std::endl
        << "  query_string = " << r.query_string << std::endl
        << "  fragment = " << r.fragment << std::endl
        << "  http_version = " << r.http_version_major << "." << r.http_version_minor << std::endl
        << "  headers = " << std::endl;
    for (const auto &pair : r.headers)
        out << "    " << pair.first << " => " << pair.second << std::endl;
    out << "  body = " << r.body << std::endl
        << std::endl;
}
