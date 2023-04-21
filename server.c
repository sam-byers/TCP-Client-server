/*server*/
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
#include "QuizDB.h"
#include "Prompt.h"
/*Include paths*/

#define BUFSIZE 2048

int communication(int cfd);
void OUTBOUND(int cfd, char **data, int call);
char *INBOUND(int cfd);
void MAKERANDOMS(int *listPTR);
int main(int argc, char const *argv[])
{
   int cfd; // Will hold socket ID
   if (argc != 3)
   {
      fprintf(stderr, "[!]Usage: %s <Server IP ADDR> <port>\n", argv[0]);
      exit(-1);
   }
   struct sockaddr_in serverAddress;

   memset(&serverAddress, 0, sizeof(struct sockaddr_in));

   serverAddress.sin_family = AF_INET;
   serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
   serverAddress.sin_port = htons(atoi(argv[2]));
   int addressLEN = sizeof(serverAddress);

   int lfd = socket(AF_INET, SOCK_STREAM, 0);
   if (lfd == -1)
   {
      fprintf(stderr, "[!]socket() error.\n");
      exit(-1);
   }
   int optval = 1;
   if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
   {
      fprintf(stderr, "[!]setsockopt() error.\n");
      exit(-1);
   }
   int rc = bind(lfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr));
   if (rc == -1)
   {
      fprintf(stderr, "[!]bind() error.\n");
      exit(-1);
   }
   if (listen(lfd, BUFSIZE) == -1)
   {
      fprintf(stderr, "[!]listen() error.\n");
      exit(-1);
   }
   fprintf(stdout, "[+]Listening on %s:%d \n[=]<Press ctrl-C to terminate>\n", argv[1], atoi(argv[2]));

   while ((cfd = accept(lfd, (struct sockaddr *)&serverAddress, (socklen_t *)&addressLEN)) != -1)
   {
      if (fork() == 0)
      {
         fprintf(stdout, "[+]Connection accepted with ID:%d\n", cfd);
         return communication(cfd);
      }
      else
      {
         wait(NULL);
         fprintf(stdout, "[-]Connection closed with ID:%d\n", cfd);
         if (close(cfd) == -1) /* Close connection */
         {
            fprintf(stderr, "[!]close error.\n");
            exit(EXIT_FAILURE);
         }
      }
   }
   if (cfd == -1)
   {
      fprintf(stderr, "[!]accept() error.\n");
      exit(-1);
   }

   if (close(lfd) == -1) /* Close connection */
   {
      fprintf(stderr, "[!]close error.\n");
      exit(EXIT_FAILURE);
   }

   exit(EXIT_SUCCESS);

   return 0;
}

int communication(int cfd)
{
   int correct = 0;
   char *returned;
   int *questions;
   OUTBOUND(cfd, prompt, 0);
   returned = INBOUND(cfd);
   if (strcmp(returned, "Y") == 0 || strcmp(returned, "y") == 0)
   {
      questions = (int *)malloc(5 * sizeof(int));
      MAKERANDOMS(questions);
      for (int i = 0; i < 5; i++)
      {
         OUTBOUND(cfd, QuizQ, questions[i] + 1);
         returned = INBOUND(cfd);
         if (strcmp(returned, "") == 0 || strcmp(returned, " ") == 0)
         {
            i--;
            continue;
         }
         if (strcmp(returned, QuizA[questions[i]]) == 0)
         {
            OUTBOUND(cfd, response, 1);
            correct++;
         }
         else
         {
            OUTBOUND(cfd, response, 2);
            OUTBOUND(cfd, QuizA, questions[i] + 1);
            OUTBOUND(cfd, response, 4);
         }
      }
      OUTBOUND(cfd, response, 5 + correct);
      OUTBOUND(cfd, response, 11);
   }
   else
   {
      OUTBOUND(cfd, response, 3);
   }
   return 0;
}
void OUTBOUND(int cfd, char **data, int call) //OUTBOUND function is used to send data
{
   if (call == 0)  //If the call is zero, we want to send the inital  prompt
      for (int i = 0; i < 6; i++)
      {
         send(cfd, data[i], strlen(data[i]), 0); //Send each line in the prompt
      }
   else
   {
      send(cfd, data[call - 1], strlen(data[call - 1]), 0); //Send the specific line
   }
   return;
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
   fprintf(stdout, "[+]Response Recieved:%s\n", buff);
   return buff;
}

void MAKERANDOMS(int *listPTR)
{
   srand(time(NULL));
   for (int i = 0; i < 5; i++)
   {
      listPTR[i] = rand() % 43 + 1;
      for (int j = 0; j < i; j++)
      {
         if (listPTR[i] == listPTR[j]) // if this value has appeared before
         {
            i--; // regenerate that value so no questions repeat
            break;
         }
      }
   }
   fprintf(stdout, "[=]Questions to be given [%d] [%d] [%d] [%d] [%d]\n", listPTR[0], listPTR[1], listPTR[2], listPTR[3], listPTR[4]);
   return;
}
