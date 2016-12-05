#include "http.hpp"
#include "network.hpp"
#include "parser.hpp"
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

void close_request(HTTPRequest *r) {
    // closing a file descriptor will cause it to be removed from all epoll sets automatically
    // see: http://stackoverflow.com/questions/8707601/is-it-necessary-to-deregister-a-socket-from-epoll-before-closing-it
    fprintf(stderr, "close_request fd=%d\n", r->fd_socket);
    if (close(r->fd_socket) < 0) {
        perror("close");
    }
    delete r;
}

const char* get_http_status(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 301: return "Moved Permanently";
        case 302: return "Moved Temporarily";
        case 303: return "See Other";
        case 500: return "Server Error";
    }
    return NULL;
}

const std::unordered_map<std::string, std::string> HTTP_MIME{
    {"html", "text/html"},
    {"xml", "text/xml"},
    {"xhtml", "application/xhtml+xml"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"css", "text/css"},
};

std::string get_content_type(const std::string &ext) {
    auto iter = HTTP_MIME.find(ext);
    if (iter == HTTP_MIME.end()) return "text/plain";
    return iter->second;
}

void serve_error(HTTPRequest *r, int status_code) {
    std::ostringstream hdr, txt;
    const char *status_name = get_http_status(status_code);
    txt << "<html>\r\n"
        << "<head><title>" << status_code << ' ' << status_name << "</title></head>\r\n"
        << "<body bgcolor=\"white\">\r\n"
        << "<center><h1>" << status_code << ' ' << status_name << "</h1></center>\r\n"
        << "<hr><center>naughttpd</center>\r\n"
        << "</body>\r\n"
        << "</html>\r\n"
        << "<!-- a padding to disable MSIE and Chrome friendly error page -->\r\n"
        << "<!-- a padding to disable MSIE and Chrome friendly error page -->\r\n"
        << "<!-- a padding to disable MSIE and Chrome friendly error page -->\r\n"
        << "<!-- a padding to disable MSIE and Chrome friendly error page -->\r\n"
        << "<!-- a padding to disable MSIE and Chrome friendly error page -->\r\n"
        << "<!-- a padding to disable MSIE and Chrome friendly error page -->\r\n";
    std::string txt_str = txt.str();
    hdr << "HTTP/1.1 " << status_code << ' ' << status_name << "\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << txt_str.size() << "\r\n"
        << "\r\n";
    std::string hdr_str = hdr.str();
    rio_writen(r->fd_socket, hdr_str.c_str(), hdr_str.size());
    rio_writen(r->fd_socket, txt_str.c_str(), txt_str.size());
}

void serve_static(HTTPRequest *r) {
    std::cout << "==================serve_static==================" << std::endl << *r;

    std::string filename = "./" + r->request_path;
    size_t pos_rdot = filename.rfind('.');
    std::string ext = pos_rdot == std::string::npos ? "" : filename.substr(pos_rdot+1);
    struct stat sbuf;
    if (stat(filename.c_str(), &sbuf) < 0) {
        serve_error(r, 404);
        return;
    }

    std::ostringstream hdr;
    hdr << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << get_content_type(ext) << "\r\n"
        << "Content-Length: " << sbuf.st_size << "\r\n"
        << "\r\n";
    std::string hdr_str = hdr.str();
    ssize_t writen = rio_writen(r->fd_socket, hdr_str.c_str(), hdr_str.size());
    if (writen != hdr_str.size()) {
        fprintf(stderr, "writen != hdr_str.size()\n");
        return;
    }

    int srcfd = open(filename.c_str(), O_RDONLY, 0);
    if (srcfd < 0) {
        perror("open");
    }
    if (sendfile(r->fd_socket, srcfd, NULL, sbuf.st_size) != sbuf.st_size) {
        perror("sendfile");
    }
    if (close(srcfd) < 0) {
        perror("close");
    }
}

void do_request(HTTPRequest *r) {
    for (;;) {
        size_t buf_remain = HTTPRequest::BUF_SIZE - (r->buf_tail - r->buf_head) - 1;
        buf_remain = std::min(buf_remain, HTTPRequest::BUF_SIZE - r->buf_tail % HTTPRequest::BUF_SIZE);
        char *ptail = &r->buf[r->buf_tail % HTTPRequest::BUF_SIZE];
        int nread = read(r->fd_socket, ptail, buf_remain);
        fprintf(stderr, "nread=%d, fd=%d\n", nread, r->fd_socket);
        if (nread < 0) {
            // If errno == EAGAIN, that means we have read all
            // data. So go back to the main loop.
            if (errno != EAGAIN) {
                perror("read");
                abort();
            }
            break;
        } else if (nread == 0) {
            // End of file. The remote has closed the connection.
            close_request(r);
            return;
        }

        r->buf_tail += nread;
        ParseResult parse_result = parse(r);
        if (parse_result == PARSE_RESULT_AGAIN) {
            continue;
        } else if (parse_result == PARSE_RESULT_INVALID) {
            fprintf(stderr, "PARSE_RESULT_INVALID  fd=%d\n", r->fd_socket);
            close_request(r);
            return;
        }

        serve_static(r);
        r->clear();
        // close_request(r);
        // return;
    }

    struct epoll_event event;
    event.data.ptr = static_cast<void*>(r);
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    if (epoll_ctl(r->fd_epoll, EPOLL_CTL_MOD, r->fd_socket, &event) < 0) {
        perror("epoll_ctl");
        abort();
    }
}
