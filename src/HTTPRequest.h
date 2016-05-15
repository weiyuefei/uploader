#ifndef __HTTPREQUEST_H__
#define __HTTPREQUEST_H__

#include "curl/curl.h"
#include <string>
using std::string;

typedef enum {
    httpGetMethod           = 0,
    httpHeadMethod          = 1,
    httpPostMethod          = 2,
    httpOptionsMethod       = 3,
    httpPutMethod           = 4,
    httpDeleteMethod        = 5,
    httpTraceMethod         = 6,
    httpConnectMethod       = 7,

    httpNumsMethod          = 8,
    httpIllegalMethod       = 8
} eHttpMethod;

enum eErrCode {
    ERR_NoErr                   = 0,
    ERR_UnsupportedProtocol     = 1,
    ERR_UnsupportedMethod       = 2,
    ERR_ConnectFail             = 3,
    ERR_RemoteAccessDenied      = 4,
    ERR_HttpReturnedErr         = 5,
    ERR_ReadErr                 = 6,
    ERR_CurlFail                = 7
};

typedef eErrCode ERR_CODE;

#define MAX_BUFFER_SIZE 4094

class HTTPRequest {

public:
    HTTPRequest();
    ERR_CODE sendRequest(eHttpMethod method, string url, string data = "");

    bool parse(string strUrl);
    int connectTo();
    string constructRequest();
    off_t getContentLength();

    // return the body of the get request
    string body;

private:

    string m_strIP;
    int    m_nPort;
    string m_strUri;
    string m_strQuery;
    string m_strScheme;

    char m_szRecvBuffer[MAX_BUFFER_SIZE];

    ERR_CODE getRequest(string url);
    ERR_CODE postRequest(string url, string data);

    static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
};

#endif
