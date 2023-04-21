#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <wait.h>


#define BUFSIZE 2048

int communication(int cfd);

char *INBOUND(int cfd);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IP address of server> <port number>.\n",
                argv[0]);
        exit(-1);
    }

    struct sockaddr_in serverAddress;

    memset(&serverAddress, 0, sizeof(struct sockaddr_in));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_port = htons(atoi(argv[2]));

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        fprintf(stderr, "[!]socket() error.\n");
        exit(-1);
    }

    int rc;

    while ((rc = connect(cfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr))) != -1)
    {
        printf("[+]Connected to server.\n");
        sleep(1);
        return communication(cfd);
    }

    close(cfd);
    return 0;
}
char *INBOUND(int cfd)
{
    static char buff[BUFSIZE];
    int nBYTES = recv(cfd, buff, BUFSIZE, 0); // receiving the prompt
    buff[nBYTES] = '\0';
    if (nBYTES == -1)
    {
        fprintf(stderr, "[!]recv() error.\n");
        exit(-1);
    }
    else if (nBYTES == 0)
    { // the other connection has closed the socket (ctrl-C)
        return NULL;
    }
    //fprintf(stdout, "[+]Data Recieved\n");
    return buff;
}
int communication(int cfd)
{
    char *returned;
    char *endcheck;
    char end1[] = {'y','e','.','\0'};
    char end2[] = {'E','N','D','\0'};
    char buffer[BUFSIZE];
    while (1)
    {
        returned = INBOUND(cfd);
        endcheck = (char *) malloc(4 * sizeof(char));
        endcheck = &returned[strlen(returned)-4];
        endcheck[3] = '\0';
        if (strcmp(endcheck, end1) == 0 || strcmp(endcheck, end2) == 0)
        {
            printf("[-]Server has closed the connection.\n");
            return 0;
        }
        printf("%s\n", returned);
        fgets(buffer, BUFSIZE, stdin);
        int i = 0;
        while (buffer[i] != '\n')
        {
            i++;
        }
        buffer[i] = '\0';
        send(cfd, buffer, i + 1, 0);
    }
    return 0;
}