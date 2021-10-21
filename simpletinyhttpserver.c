#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void error_handler(char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(-1);
}
void send_error(FILE *client_write)
{
    char protocol[]="HTTP/1.0 400 Bad Request\r\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cnt_len[] = "Content-length:2048\r\n";
    char cnt_type[] = "Content-type:text/html\r\n\r\n";
    char content[] = "<html><head><title>NETWORK</title></head>"
                    "<body><font size=+5><br>发生错误! 查看请求文件名和请求方式！"
                    "</font></body></html>";
    fputs(protocol, client_write);
    fputs(server, client_write);
    fputs(cnt_len, client_write);
    fputs(cnt_type, client_write);
    fputs(content, client_write);
    fflush(client_write); 
}
void send_data(FILE *fp, char *ct, char *file_name)
{
    char protocol[]="HTTP/1.0 200 OK\r\n";
    char server[] = "Server:Linux Web Server\r\n";
    char cnt_len[] = "Content-length:2048\r\n";
    char cnt_type[SMALL_BUF];
    char buf[BUF_SIZE];
    FILE *send_file;
    sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
    send_file = fopen(file_name, "r");
    if(send_file == NULL)
    {
        send_error(fp);
        return;
    }
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);

    while(fgets(buf, BUF_SIZE, send_file) != NULL)
    {
        fputs(buf, fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
}
void *request_handler(void *arg)
{
    int client_sock = *(int *)arg;
    char req_line[SMALL_BUF];
    FILE *client_read, *client_write;
    
    char method[10];
    char ct[15];
    char file_name[30];
    client_read = fdopen(client_sock, "r");
    client_write = fdopen(dup(client_sock), "w");

    fgets(req_line, SMALL_BUF, client_read);
    if(strstr(req_line, "HTTP/") == NULL)
    {
        send_error(client_write);
        fclose(client_read);
        fclose(client_write);
        return NULL;
    }
    strcpy(method, strtok(req_line, " /"));
    strcpy(file_name, strtok(NULL, " /"));
    if(strcmp(method, "GET") != 0)
    {
        send_error(client_write);
        fclose(client_read);
        fclose(client_write);
        return NULL;
    }
    fclose(client_read);
    send_data(client_write, ct, file_name);
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        error_handler("Usage: simpletinyhttpserver <port>");
    }
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0)
    {
        error_handler("socket error");
    }
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if(bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        error_handler("bind error");
    }
    if(listen(server_sock, 20) < 0)
    {
        error_handler("listen error");
    }

    while(1)
    {
        addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if(client_sock < 0)
            continue;
        pthread_t t_id;
        pthread_create(&t_id, NULL, request_handler, (void *)&client_sock);
        pthread_detach(t_id);
        printf("new connection ip: %s port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    }
    close(server_sock);
    return 0;
}