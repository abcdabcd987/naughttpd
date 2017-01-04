#include "http.hpp"
#include "network.hpp"
#include "parser.hpp"
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

bool enable_tcp_cork;
bool enable_tcp_nodelay;
SendfileMethod sendfile_method;

void close_request(HTTPRequest *r) {
    if (close(r->fd_socket) < 0)
        perror("close");
}

const char* get_http_status(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 400: return "Bad Request";
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
        << "Content-Length: " << txt_str.size() << "\r\n";
    if (status_code != 400)
        hdr << "Connection: keep-alive\r\n";
    hdr << "\r\n";
    std::string hdr_str = hdr.str();
    rio_writen(r->fd_socket, hdr_str.c_str(), hdr_str.size());
    rio_writen(r->fd_socket, txt_str.c_str(), txt_str.size());
}

enum DoRequestState {
    DOREQUEST_READ = 0,
    DOREQUEST_SERVE_STATIC,
    DOREQUEST_SENDFILE,
    DOREQUEST_FINISHING,
    DOREQUEST_LOOP,
    DOREQUEST_CLOSE,
    DOREQUEST_READ_AGAIN,
};

void do_request_read(HTTPRequest *r) {
    // fprintf(stderr, "%s\n", r->uri.c_str());
    size_t buf_remain = HTTPRequest::BUF_SIZE - (r->buf_tail - r->buf_head) - 1;
    buf_remain = std::min(buf_remain, HTTPRequest::BUF_SIZE - r->buf_tail % HTTPRequest::BUF_SIZE);
    char *ptail = &r->buf[r->buf_tail % HTTPRequest::BUF_SIZE];
    int nread = read(r->fd_socket, ptail, buf_remain);
    if (nread < 0) {
        // If errno == EAGAIN, that means we have read all
        // data. So go back to the main loop.
        if (errno != EAGAIN) {
            r->do_request_state = DOREQUEST_CLOSE;
            return;
        }
        r->do_request_state = DOREQUEST_READ_AGAIN;
        return;
    } else if (nread == 0) {
        // End of file. The remote has closed the connection.
        r->do_request_state = DOREQUEST_CLOSE;
        return;
    }

    r->buf_tail += nread;
    ParseResult parse_result = parse(r);
    if (parse_result == PARSE_RESULT_AGAIN) {
        return;
    } else if (parse_result == PARSE_RESULT_INVALID) {
        serve_error(r, 400);
        r->do_request_state = DOREQUEST_CLOSE;
        return;
    }

    r->do_request_state = DOREQUEST_SERVE_STATIC;
}

void serve_static(HTTPRequest *r) {
    std::string filename = "./" + r->request_path;
    size_t pos_rdot = filename.rfind('.');
    std::string ext = pos_rdot == std::string::npos ? "" : filename.substr(pos_rdot+1);

    int srcfd = open(filename.c_str(), O_RDONLY, 0);
    if (srcfd < 0) {
        serve_error(r, 404);
        r->do_request_state = DOREQUEST_READ;
        return;
    }

    struct stat sbuf;
    if (fstat(srcfd, &sbuf) < 0) {
        perror("fstat");
        r->do_request_state = DOREQUEST_READ;
        return;
    }

    if (enable_tcp_nodelay)
        tcp_nodelay_on(r->fd_socket);
    if (enable_tcp_cork)
        tcp_cork_on(r->fd_socket);

    std::ostringstream hdr;
    hdr << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << get_content_type(ext) << "\r\n"
        << "Content-Length: " << sbuf.st_size << "\r\n"
        << "Connection: " << (r->keep_alive ? "keep-alive" : "close") << "\r\n"
        << "\r\n";
    std::string hdr_str = hdr.str();
    ssize_t writen = rio_writen(r->fd_socket, hdr_str.c_str(), hdr_str.size());
    if (writen != hdr_str.size()) {
        fprintf(stderr, "writen != hdr_str.size()\n");
        r->do_request_state = DOREQUEST_READ;
        return;
    }

    r->do_request_state = DOREQUEST_SENDFILE;
    r->srcfd = srcfd;
    r->file_size = sbuf.st_size;
    r->writen = 0;
    r->offset = 0;
    r->buf_head = 0;
    r->buf_tail = 0;
    if (sendfile_method == SENDFILE_MMAP) {
        void *srcaddr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
        if (srcaddr == (void *) -1) {
            perror("mmap");
            r->do_request_state = DOREQUEST_READ;
            return;
        }
        r->srcaddr = srcaddr;
    }
}

void serve_static_sendfile(HTTPRequest *r) {
    if (sendfile_method == SENDFILE_SENDFILE) {
        size_t &writen = r->writen;
        while (writen < r->file_size) {
            ssize_t n = sendfile(r->fd_socket, r->srcfd, &r->offset, r->file_size);
            if (n < 0) {
                if (errno == EAGAIN)
                    return;
                perror("sendfile");
                r->do_request_state = DOREQUEST_READ;
                return;
            }
            writen += n;
        }
    } else if (sendfile_method == SENDFILE_MMAP) {
        size_t &writen = r->writen;
        while (writen < r->file_size) {
            char *start =(char*)r->srcaddr + writen;
            ssize_t n = write(r->fd_socket, (void*)start, r->file_size - writen);
            if (n < 0) {
                if (errno == EAGAIN)
                    return;
                perror("write");
                r->do_request_state = DOREQUEST_READ;
                return;
            }
            writen += n;
        }
        if (munmap(r->srcaddr, r->file_size) < 0) {
            perror("munmap");
        }
    } else {
        if (r->buf_tail == r->buf_head) {
            // buffer is empty. read more.
            ssize_t readn = read(r->srcfd, r->buf, HTTPRequest::BUF_SIZE);
            if (readn < 0) {
                perror("readn");
                r->do_request_state = DOREQUEST_READ;
                return;
            }
            r->buf_head = 0;
            r->buf_tail = readn;
        }

        while (r->buf_head < r->buf_tail) {
            ssize_t n = write(r->fd_socket, r->buf + r->buf_head, r->buf_tail - r->buf_head);
            if (n < 0) {
                if (errno == EAGAIN)
                    return;
                perror("write");
                r->do_request_state = DOREQUEST_READ;
                return;
            }
            r->buf_head += n;
            r->writen += n;
        }

        if (r->writen != r->file_size)
            return;
    }

    if (close(r->srcfd) < 0) {
        perror("close");
    }
    if (enable_tcp_cork)
        tcp_cork_off(r->fd_socket); // send messages out
    r->do_request_state = DOREQUEST_FINISHING;
    return;
}

DoRequestResult do_request(HTTPRequest *r) {
    for (;;) {
        switch (r->do_request_state) {
            case DOREQUEST_READ:
                do_request_read(r);
                break;
            case DOREQUEST_SERVE_STATIC:
                serve_static(r);
                break;
            case DOREQUEST_SENDFILE:
                serve_static_sendfile(r);
                if (r->do_request_state == DOREQUEST_SENDFILE)
                    return DO_REQUEST_WRITE_AGAIN;
                break;
            case DOREQUEST_FINISHING:
                r->clear();
                r->do_request_state = r->keep_alive ? DOREQUEST_READ : DOREQUEST_CLOSE;
                break;
            case DOREQUEST_READ_AGAIN:
                r->do_request_state = DOREQUEST_READ;
                return DO_REQUEST_READ_AGAIN;
            case DOREQUEST_CLOSE:
                r->do_request_state = DOREQUEST_READ;
                return DO_REQUEST_CLOSE;
            case DOREQUEST_LOOP:
                break;
            default:
                break;
        }
    }
}
