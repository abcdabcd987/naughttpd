#include "http_request.hpp"

HTTPRequest::HTTPRequest() :
        // buffer
        buf_head(0),
        buf_tail(0),

        // parser temporary variables
        parser_state(0)
{

}


HTTPRequest::~HTTPRequest() {
    
}
