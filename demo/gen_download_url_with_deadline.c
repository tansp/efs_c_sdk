#include "../efs/cdn.h"
#include "../efs/tm.h"

int main(int argc, char * argv[])
{
    char * key = argv[1];
    char * path = argv[2];
    char * deadlineStr = argv[3];
    char * encodedPath;
    Efs_Uint64 deadline;

    if (argc < 3) {
        printf("Usage: gen_download_url_with_deadline <KEY> <URL> [DEADLINE]\n");
        return 1;
    } // if

    if (deadlineStr) {
        deadline = atol(deadlineStr);
    } else {
        deadline = Efs_Tm_LocalTime() + 3600;
    } // if

    encodedPath = Efs_Cdn_MakeDownloadUrlWithDeadline(key, path, deadline);
    printf("%s\n", encodedPath);
    Efs_Free(encodedPath);
    return 0;
}
