/*client*/
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
/*include standard libs*/

#define BUFSIZE 2048 // Define the maxiumum buffer size

int communication(int cfd);
char *INBOUND(int cfd);
/*function prototypes*/

int main(int argc, char *argv[])
{
    if (argc != 3) // If too few arguemnts supplied, print an error and exit(-1)
    {
        fprintf(stderr, "Usage: %s <IP address of server> <port number>.\n",
                argv[0]);
        exit(-1);
    }

    struct sockaddr_in serverAddress; // Structure to hold the server and connection info

    memset(&serverAddress, 0, sizeof(struct sockaddr_in));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_port = htons(atoi(argv[2]));
    // Update the structure with the user supplied values

    int cfd = socket(AF_INET, SOCK_STREAM, 0); // Create a new socket and error check
    if (cfd == -1)
    {
        fprintf(stderr, "[!]socket() error.\n");
        exit(-1);
    }

    int rc;
    while ((rc = connect(cfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr))) != -1) // wait to connect to the server
    {
        // Once we have connected to a server
        printf("[+]Connected to server.\n"); // Print that we have connected to a server
        sleep(1);                            // Sleep for 1 second allow time for the connection to be estab. server side
        return communication(cfd);           // call the communication function and return
    }

    close(cfd); // Close the socket
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
    // fprintf(stdout, "[+]Data Recieved\n");
    return buff;
}
int communication(int cfd) // The communication function that handles the communication with the server during the test itself
{
    char *returned;
    char buffer[BUFSIZE];
    while (1) // Loop here
    {
        returned = INBOUND(cfd); // Get data from the server with the INBOUND function
        if (returned == NULL)    // If we havent returned anyting, the server has closed the connection
        {
            printf("[!]Connection closed by server.\n"); // Print that the connection has been closed
            return 0;                                    // Return to end the program
        }
        for (int i = 0; returned[i] != EOF && returned[i] != '\0'; i++) // For every character in the retuned data that is not the null char or EOF
        {
            printf("%c", returned[i]); // Print that character
        }
        printf("\n");   // Then print a new line character
        if (returned[strlen(returned) - 1] == EOF) // If the last character is an EOF marker, we have recieved the whole transmission
        {
            /*It is now our turn to reply*/
            fgets(buffer, BUFSIZE, stdin);  //Get the user input and write it to the buffer
            int i = 0;  //Initalise i at 0
            while (buffer[i] != '\n')   //While we dont have a newline, (fgets will be newline terminated)
            {
                i++;    // Increment i
            }
            buffer[i] = '\0'; // Once we have found the newline, replace it with a null character
            send(cfd, buffer, i + 1, 0);    //Send the whole buffer once completed
        }
    }
    return 0;
}