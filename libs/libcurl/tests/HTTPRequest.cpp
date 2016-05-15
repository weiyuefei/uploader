
#include <iostream>
using namespace std;

#include "HTTPRequest.h"

HTTPRequest::HTTPRequest()
: body("")
{
}

ERR_CODE HTTPRequest::sendRequest(eHttpMethod method, string url, string data)
{
    ERR_CODE ecode;
    switch (method)
    {
        case httpGetMethod:
            ecode = getRequest(url);
            break;
        case httpPostMethod:
            if (data.length() == 0) {
                cout << "Warn: You have not provided the post data" << endl;
            }
            ecode = postRequest(url, data);
            break;
        default:
            cerr << "###### Method not supported." << endl;
            ecode = ERR_UnsupportedMethod;
            break;
    }

    return ecode;
}


ERR_CODE HTTPRequest::getRequest(string url)
{
    CURL *curlhandle = NULL;
    CURLcode success = CURLE_OK;
    ERR_CODE retcode = ERR_NoErr;

    curl_global_init(CURL_GLOBAL_ALL);

    // get the easy curl handle
    curlhandle = curl_easy_init();
    if (NULL == curlhandle)
    {
        retcode = ERR_CurlFail;
        curl_global_cleanup();
        cerr << "curl init fail" << endl;

        return retcode;
    }

    // set curl options
    curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, this);

    curl_easy_setopt(curlhandle, CURLOPT_MAXREDIRS, 1);
    curl_easy_setopt(curlhandle, CURLOPT_FOLLOWLOCATION, 1);

    success = curl_easy_perform(curlhandle);

    if (success != CURLE_OK)
    {
        const char *strErr = curl_easy_strerror(success);
        cerr << "libcurl perform error: " << strErr << endl;

        retcode = ERR_CurlFail;
    }

    long httpResponseCode = 0;
    success = curl_easy_getinfo(curlhandle, CURLINFO_RESPONSE_CODE, &httpResponseCode);
    if (success == CURLE_OK) {
        if (httpResponseCode != 200 && httpResponseCode != 206) {
	    cerr << "libcurl perform error, status code: " <<  httpResponseCode << endl;
            body.erase();
	    body = "Content IS Deleted!";
	    retcode = ERR_CurlFail;
	}
    }

    curl_easy_cleanup(curlhandle);
    curl_global_cleanup();

    return retcode;
}

ERR_CODE HTTPRequest::postRequest(string url, string data)
{
    CURL *curlhandle;
    CURLcode res;
    ERR_CODE retcode = ERR_NoErr;

    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curlhandle = curl_easy_init();
    if(curlhandle)
    {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());
        /* Now specify the POST data */
        curl_easy_setopt(curlhandle, CURLOPT_POSTFIELDS, data.c_str());

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curlhandle);
        /* Check for errors */
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

            retcode = ERR_CurlFail;
        }

        long httpResponseCode = 0;
        res = curl_easy_getinfo(curlhandle, CURLINFO_RESPONSE_CODE, &httpResponseCode);
        if (res == CURLE_OK) {
            if (httpResponseCode != 200 && httpResponseCode != 206) {
                cerr << "Http Post maybe failed, as status code is " <<  httpResponseCode << endl;
                retcode = ERR_CurlFail;
            } else {
		body = "Http Post success";
	    }
        }

	
        /* always cleanup */
        curl_easy_cleanup(curlhandle);
    } else {
        retcode = ERR_CurlFail;
    }

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
