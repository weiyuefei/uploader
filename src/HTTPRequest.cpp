
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <iostream>
#include <sstream>
using namespace std;

#include "HTTPRequest.h"
#include "Logger.h"

HTTPRequest::HTTPRequest()
: body("")
{
}

ERR_CODE HTTPRequest::sendRequest(eHttpMethod method, string url, string data)
{
    ERR_CODE ecode = ERR_NoErr;
    switch (method)
    {
        case httpGetMethod:
            ecode = getRequest(url);
            break;
        case httpPostMethod:
            if (data.length() == 0)
            {
                LOG_WARN("****** No post data provided the request will be sent without post data");
            }
            ecode = postRequest(url, data);
            break;
        default:
            LOG_ERROR("###### Method not supported.");
            ecode = ERR_UnsupportedMethod;
            break;
    }

    return ecode;
}


ERR_CODE HTTPRequest::getRequest(string url)
{
    CURL *curlhandle = NULL;
    CURLcode curlcode = CURLE_OK;
    ERR_CODE retcode = ERR_NoErr;
    long httpResponseCode = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    // get the easy curl handle
    curlhandle = curl_easy_init();
    if (NULL == curlhandle)
    {
        retcode = ERR_CurlFail;
        curl_global_cleanup();
        LOG_ERROR("###### curl init fail");

        return retcode;
    }

    // set curl options
    curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, this);

    curl_easy_setopt(curlhandle, CURLOPT_MAXREDIRS, 1);
    curl_easy_setopt(curlhandle, CURLOPT_FOLLOWLOCATION, 1);

    curlcode = curl_easy_perform(curlhandle);

    if (curlcode != CURLE_OK)
    {
        const char *strErr = curl_easy_strerror(curlcode);
        cerr << "libcurl perform error: " << strErr << endl;

        retcode = ERR_CurlFail;

        goto next;
    }

    // Even when curlcode is CURLE_OK, the returned content
    // maybe the 404 web content instead of the real what we
    // want. Thus we still need to judge http response code
    // further.

    //long httpResponseCode = 0;
    curlcode = curl_easy_getinfo(curlhandle, CURLINFO_RESPONSE_CODE, &httpResponseCode);
    if (curlcode == CURLE_OK)
    {
        if (httpResponseCode != 200 && httpResponseCode != 206)
        {
            LOG_ERROR("Http status code: %d",  httpResponseCode);

            // the content is not what we want, thus clear it.
            body.erase();

            retcode = ERR_CurlFail;
        }
    }
    else
    {
        LOG_ERROR("###### curl_easy_getinfo failed, failed to know whether the content is ok or not");
    }

next:

    curl_easy_cleanup(curlhandle);
    curl_global_cleanup();

    return retcode;
}

ERR_CODE HTTPRequest::postRequest(string url, string data)
{
    CURL *curlhandle;
    CURLcode res = CURLE_OK;;
    ERR_CODE retcode = ERR_NoErr;
    long httpResponseCode = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curlhandle = curl_easy_init();
    if (curlhandle == NULL)
    {
        curl_global_cleanup();
        LOG_ERROR("###### postRequest::curl init failed");
        return ERR_CurlFail;
    }

    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. 
    */
    curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());

    /* Custom HTTP headers */
    struct curl_slist *list = NULL;
    list = curl_slist_append(list, "Content-Type: application/json;charset=UTF-8");
    list = curl_slist_append(list, "User-Agent: uploader");
    curl_easy_setopt(curlhandle, CURLOPT_HTTPHEADER, list);

    /* Set the callback for receiving the data */
    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, this);

    /* Now specify the POST data */
    curl_easy_setopt(curlhandle, CURLOPT_POSTFIELDS, data.c_str());

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curlhandle);
    /* Check for errors */
    if(res != CURLE_OK)
    {
        LOG_ERROR("###### curl_easy_perform() failed: %s",
                curl_easy_strerror(res));
        retcode = ERR_CurlFail;
        goto next;
    }

    //long httpResponseCode = 0;
    res = curl_easy_getinfo(curlhandle, CURLINFO_RESPONSE_CODE, &httpResponseCode);
    if (res == CURLE_OK)
    {
        if (httpResponseCode != 200 && httpResponseCode != 206)
        {
            LOG_ERROR("###### Http status code: %d", httpResponseCode);

            // the content is not what we want, clear it.
            body.erase();

            retcode = ERR_CurlFail;
        }
    }
    else
    {
        LOG_ERROR("###### curl_easy_getinfo failed, failed to know whether the content is ok or not");
    }

next:

    curl_slist_free_all(list);
    curl_easy_cleanup(curlhandle);
    curl_global_cleanup();

    return retcode;
}

size_t HTTPRequest::write_data(void * buffer, size_t size, size_t nmemb, void * userp)
{
    // recv data and process it
    HTTPRequest *theRequest = (HTTPRequest *)userp;

    size_t recvSize = size * nmemb;

    theRequest->body.append((const char *)buffer, recvSize);

    return recvSize;
}

//////////// add for ftp upload ///////////////

bool HTTPRequest::parse(string strUrl)
{
    string::size_type posColon, posSlash, posQuestion;
    if (strUrl.find("http://") != string::npos)
    {
        m_strScheme = "http://";
    }
    else if (strUrl.find("https://") != string::npos)
    {
        m_strScheme = "https://";
    }
    else
    {
        m_strScheme = "";
    }

    int schemeLen = m_strScheme.length();
    posColon    = strUrl.find(":", schemeLen);
    posSlash    = strUrl.find("/", schemeLen);
    posQuestion = strUrl.find("?", schemeLen);

    if (posColon != string::npos)
    {
        m_strIP = strUrl.substr(m_strScheme.length(), posColon - m_strScheme.length());
        string strPort = strUrl.substr(posColon+1, posSlash-posColon-1);
        stringstream ss;
        ss.clear();
        ss.str(strPort);
        ss >> m_nPort;
    }
    else
    {
        m_strIP = strUrl.substr(m_strScheme.length(), posSlash - m_strScheme.length());
        m_nPort = 80;
    }

    if (posQuestion == string::npos)
    {
        m_strUri = strUrl.substr(posSlash);
        m_strQuery = "";
    }
    else
    {
        m_strUri = strUrl.substr(posSlash, posQuestion-posSlash);
        m_strQuery = strUrl.substr(posQuestion + 1);
    }

    LOG_INFO("URL parsed results:[\nScheme=\"%s\"\nIP addr=\"%s\"\nport=%d\nURI=\"%s\"\nQuery string=\"%s\"\n]",
                m_strScheme.c_str(),
                m_strIP.c_str(),
                m_nPort,
                m_strUri.c_str(),
                m_strQuery.c_str());

    return true;
}

int HTTPRequest::connectTo()
{
    int sockfd;
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ((he = gethostbyname(m_strIP.c_str())) == NULL)
    {

        LOG_ERROR("###### gethostbyname error, host: %s", m_strIP.c_str());
        return -1;
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        LOG_ERROR("###### socket error");
        return -1;
    }

    bzero(&their_addr, sizeof(their_addr));

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(m_nPort);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(their_addr)) == -1)
    {
        LOG_ERROR("###### connect error");
        return -1;
    }

    LOG_INFO("Connect remote server success, begin to send the request to remote");

    string strReqHdr = constructRequest();

    if (send(sockfd, strReqHdr.c_str(), strReqHdr.length(), 0 ) == -1 )
    {
        LOG_ERROR("###### Send request header failed");
        close(sockfd);
        return -1;
    }

	char *p = m_szRecvBuffer;
	bool bStart = false;
    size_t numbytes;

    memset(m_szRecvBuffer, 0, MAX_BUFFER_SIZE);

	while (1)
	{
		char* pEnd = strstr(m_szRecvBuffer, "\r\n\r\n");
		if (pEnd != NULL)
        {
			// means read out the headers
			bStart = true;
			LOG_INFO("Read the response headers: [%s]", m_szRecvBuffer);
			break;
		}

        // Everytime read out one byte from socket buffer
        if (( numbytes = recv(sockfd, p, 1, 0)) == -1)
        {
        	LOG_ERROR("###### recv response header failed");
            close(sockfd);
            return -1;
        }
        else if( numbytes == 0 )
        {
        	LOG_INFO("Remote server has shutdown!");
            close(sockfd);
            return -1;
        }

 		p++;
	}

    return sockfd;
}

string HTTPRequest::constructRequest()
{
    string strReq = "GET ";
    strReq += m_strUri;
    if (0 != m_strQuery.length())
    {
        strReq += "?";
        strReq += m_strQuery;
    }

    strReq += " HTTP/1.1\r\n";
    strReq += "User-Agent: uploader\r\n";
    strReq += "Host: ";
    strReq += m_strIP;
    if (m_nPort != 80)
    {
        strReq += ":";
        stringstream ss;
        ss.clear();
        ss << m_nPort;
        strReq += ss.str();
    }

    strReq += "\r\n";

    strReq += "Accept: */*\r\n";
    strReq += "\r\n";

    LOG_INFO("Construct request: %s", strReq.c_str());

    return strReq;
}

off_t HTTPRequest::getContentLength()
{
    string strContentLength;
    string strRespHeader;
    strRespHeader.assign(m_szRecvBuffer, strlen(m_szRecvBuffer));

    LOG_INFO("Response header: \n[%s\n]", strRespHeader.c_str());

    string::size_type pos;
    pos = strRespHeader.find("Content-Length");
    if (pos == string::npos)
    {
        LOG_WARN("Failed to find Content-Length");
        return -1;
    }

    string::size_type p;
    pos += sizeof("Content-Length") - 1;
    while (strRespHeader[pos] > '9' || strRespHeader[pos] < '0')
    {
        pos++;
    }

    p = pos;
    while (strRespHeader[p] >= '0' && strRespHeader[p] <= '9')
    {
        p++;
    }

    strContentLength = strRespHeader.substr(pos, p - pos);

    LOG_INFO("Parsed \"Content-Length\"=[%s]", strContentLength.c_str());

    stringstream ss;
    off_t content_length;
    ss.clear();
    ss << strContentLength;
    ss >> content_length;

    LOG_INFO("****** content_length: %lu", content_length);

    return content_length;
}
