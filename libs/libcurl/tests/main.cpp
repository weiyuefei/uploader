
#include <iostream>
#include <string>
using namespace std;

#include "HTTPRequest.h"

int main(int argc, char **argv)
{
    HTTPRequest hr;
    //string url = "http://10.10.121.90:6060/home/weiyf/test.c";
    
    cout << "argv[2]=" << argv[2] << endl;
 
    ERR_CODE ecode = hr.sendRequest((argv[2] == NULL) ? httpGetMethod : httpPostMethod, argv[1], argv[2]);
    if (ecode != ERR_NoErr) {
	cerr << "something wrong  occur" << endl;
    }

    cout << "Request body: \n" << hr.body << endl;

    return 0;
}
