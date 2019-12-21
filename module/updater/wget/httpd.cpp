#include "httpd.h"

void Httpd::parse_url(const char *url, char *host, int *port, char *file_name)
{
    int j = 0;
    int start = 0;
    *port = 80;
    char *patterns[] = {"http://", "https://", NULL};

    for (int i = 0; patterns[i]; i++)
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    for (int i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

    char *pos = strstr(host, ":");
    if (pos)
        sscanf(pos, ":%d", port);

    for (int i = 0; i < (int)strlen(host); i++) {
        if (host[i] == ':') {
            host[i] = '\0';
            break;
        }
    }

    j = 0;
    for (int i = start; url[i] != '\0'; i++) {
        if (url[i] == '/') {
            if (i !=  strlen(url) - 1)
                j = 0;
            continue;
        } else
            file_name[j++] = url[i];
    }
    file_name[j] = '\0';
}

struct HTTP_RES_HEADER Httpd::parse_header(char *response)
{
    struct HTTP_RES_HEADER resp;

    char *pos = strstr(response, "HTTP/");
    if (pos)
        sscanf(pos, "%*s %d", &resp.status_code);

    pos = strstr(response, "Content-Type:");
    if (pos)
        sscanf(pos, "%*s %s", resp.content_type);

    pos = strstr(response, "Content-Length:");
    if (pos)
        sscanf(pos, "%*s %ld", &resp.content_length);

    return resp;
}

void Httpd::get_ip_addr(char *host_name, char *ip_addr)
{
    struct hostent *host = gethostbyname(host_name);
    if (!host) {
        ip_addr = NULL;
        return;
    }

    for (int i = 0; host->h_addr_list[i]; i++) {
        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}

void Httpd::progress_bar(long cur_size, long total_size, double speed)
{
    float percent = (float) cur_size / total_size;
    const int numTotal = 50;
    int numShow = (int)(numTotal * percent);
    int fixstrsize = (strlen("download : ") + strlen(filepath)) << 3;

    if (numShow == 0)
        numShow = 1;

    if (numShow > numTotal)
        numShow = numTotal;

    char sign[51] = {0};
    memset(sign, '=', numTotal);

    if (RKdisp != NULL) {
        if (cap.str_len != 0) {
            cap_rect.w = cap.str_len - fixstrsize;
            cap_rect.x = cap.x + fixstrsize;
            RKdisp->DrawRect(cap_rect, 1, 0);
        }

        sprintf(cap.str, "download %s: %.2f%%", filepath, percent * 100);
        cap.str_len = strlen(cap.str) * 8;
        cap.y = 100;
        cap.x = 80;
        cap.color = 0xFFFFFFFF;

        RKdisp->DrawString(cap);

        cap_rect.x = cap.x;
        cap_rect.y = cap.y;
        cap_rect.w = cap.str_len;
        cap_rect.h = 20;
    }

    printf("\r%.2f%%[%-*.*s] %.2f/%.2fMB %4.0fkb/s", percent * 100, numTotal, numShow, sign, cur_size / 1024.0 / 1024.0, total_size / 1024.0 / 1024.0, speed);
    fflush(stdout);

    if (numShow == numTotal)
        printf("\n");
}

unsigned long Httpd::get_file_size(const char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) < 0)
        return 0;
    return (unsigned long) buf.st_size;
}

void Httpd::download(int client_socket, char *file_name, long content_length)
{
    long hasrecieve = 0;
    struct timeval t_start, t_end;
    int mem_size = 8192;
    int buf_len = mem_size;
    int len;

    snprintf(filepath, sizeof(filepath), "%s%s", downpath, file_name);

    int fd = open(filepath, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);
    if (fd < 0) {
        printf("File creation failed!\n");
        exit(0);
    }

    char *buf = (char *) malloc(mem_size * sizeof(char));

    long diff = 0;
    int prelen = 0;
    double speed;

    while (hasrecieve < content_length) {
        gettimeofday(&t_start, NULL );
        len = read(client_socket, buf, buf_len);
        write(fd, buf, len);
        gettimeofday(&t_end, NULL );

        hasrecieve += len;


        if (t_end.tv_usec - t_start.tv_usec >= 0 &&  t_end.tv_sec - t_start.tv_sec >= 0)
            diff += 1000000 * ( t_end.tv_sec - t_start.tv_sec ) + (t_end.tv_usec - t_start.tv_usec);//us

        if (diff >= 1000000) {
            speed = (double)(hasrecieve - prelen) / (double)diff * (1000000.0 / 1024.0);
            prelen = hasrecieve;
            diff = 0;
        }

        progress_bar(hasrecieve, content_length, speed);

        if (hasrecieve == content_length)
            break;
    }

    close(fd);
}

Httpd::Httpd(RKDisplay *disp)
    : RKdisp(disp)
{
    strcpy(downpath, "/tmp/");

    if (RKdisp != NULL) {
        cap_rect.x = 0;
        cap_rect.y = 300;
        cap_rect.w = 480;
        cap_rect.h = 20;
        cap_rect.color = 0x0;

        cap.str_len = 0;
        RKdisp->RKDispClean();
    }
}

Httpd::~Httpd()
{

}

int Httpd::get(char *url)
{
    char host[64] = {0};
    char ip_addr[16] = {0};
    int port = 80;
    char file_name[256] = {0};

    puts("1: Parsing the download address...");
    parse_url(url, host, &port, file_name);

    puts("2: Getting the remote server IP address...");
    get_ip_addr(host, ip_addr);
    if (strlen(ip_addr) == 0) {
        printf("Error: the remote server's IP address could not be obtained\n");
        return -1;
    }

    puts("\n>>>>Download address resolved successfully<<<<");
    printf("\tDownload address: %s\n", url);
    printf("\tThe remote host : %s\n", host);
    printf("\tIP address      : %s\n", ip_addr);
    printf("\tPORT            : %d\n", port);
    printf("\tDownpath        : %s\n", downpath);
    printf("\tFilename        : %s\n\n", file_name);

    char header[2048] = {0};
    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
            , url, host);

    puts("3: Create a network socket...");
    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0) {
        printf("Create a network socket failed: %d\n", client_socket);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    puts("4: Connecting to remote host...");
    int res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1) {
        printf("Connecting to remote host, error: %d\n", res);
        return -1;
    }

    puts("5: Sending an http download request...");
    write(client_socket, header, strlen(header));


    int mem_size = 4096;
    int length = 0;
    int len;
    char *buf = (char *) malloc(mem_size * sizeof(char));
    char *response = (char *) malloc(mem_size * sizeof(char));

    puts("6: Parsing the http response header...");
    while ((len = read(client_socket, buf, 1)) != 0) {
        if (length + len > mem_size) {
            mem_size *= 2;
            char * temp = (char *) realloc(response, sizeof(char) * mem_size);
            if (temp == NULL) {
                printf("Dynamic memory request failed\n");
                exit(-1);
            }
            response = temp;
        }

        buf[len] = '\0';
        strcat(response, buf);

        int flag = 0;
        for (int i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
        if (flag == 4)
            break;

        length += len;
    }

    struct HTTP_RES_HEADER resp = parse_header(response);

    printf("\n>>>>http response header resolved successfully:<<<<\n");

    printf("\thttp response code: %d\n", resp.status_code);
    if (resp.status_code != 200) {
        printf("The file could not be downloaded, status_code: %d\n", resp.status_code);
        return -1;
    }
    printf("\tHTTP content_type  : %s\n", resp.content_type);
    printf("\tHTTP content_length: %ldByte\n\n", resp.content_length);


    printf("7: Start download...\n");
    download(client_socket, file_name, resp.content_length);
    printf("8: Close socket\n");

    if (resp.content_length == get_file_size(filepath))
        printf("\nfile %s download successful! ^_^\n\n", filepath);
    else {
        remove(filepath);
        printf("\nThere is a byte missing in the file download, please try again!\n\n");
    }
    shutdown(client_socket, 2);
    return 0;
}

int Httpd::setDownPath(char* path)
{
    if (path != NULL)
        strcpy(downpath, path);
    return 0;
}

#if 0
int main(int argc, char const *argv[])
{
    char url[2048];

    if (argc == 1) {
        printf("must be given an HTTP address to start working\n");
        exit(-1);
    } else {
        strcpy(url, argv[1]);
    }

    Httpd* httpd = new Httpd();
    httpd->get(url);
    delete httpd;
    httpd = NULL;
    return 0;
}
#endif

