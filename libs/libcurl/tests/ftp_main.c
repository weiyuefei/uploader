//test.c
 
#include <stdio.h>
 
#include "ftp_upload.h"
 
int main(int argc, char **argv)
{
    FTP_OPT ftp_opt;
    ftp_opt.url = "ftp://10.10.121.90/home/weiyf/upload.txt";
    ftp_opt.user_key = "root:123456";
    ftp_opt.file = "/home/weiyf/movies/BrocadCity.mp4";
 
    if(FTP_UPLOAD_SUCCESS == ftp_upload(ftp_opt))
        printf("Upload success.\n");
    else
        printf("Upload failed.\n");
 
#if 0
    ftp_opt.url = "ftp://127.0.0.1//var/ftp/download.txt";
    ftp_opt.file = "/home/xxx/project/ftpManager/download.txt";
 
    if(FTP_DOWNLOAD_SUCCESS == ftp_download(ftp_opt))
        printf("Download success.\n");
    else
        printf("Download failed.\n");
#endif

    return 0;
}
