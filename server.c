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
/*include standard libs*/
#include "QuizDB.h"
#include "Prompt.h"
/*include the quiz questions and the user prompts*/

#define BUFSIZE 2048 // Define the maximum buffer size

int communication(int cfd);
void OUTBOUND(int cfd, char **data, int call, int end);
char *INBOUND(int cfd);
void MAKERANDOMS(int *listPTR);
/*Function Prototypes*/

int main(int argc, char const *argv[])
{
   int cfd;       // Will hold socket ID
   if (argc != 3) // If we have too few arguments
   {
      fprintf(stderr, "[!]Usage: %s <Server IP ADDR> <port>\n", argv[0]);
      exit(-1);
   }

   struct sockaddr_in serverAddress; // Define server adress structure

   memset(&serverAddress, 0, sizeof(struct sockaddr_in));

   serverAddress.sin_family = AF_INET;
   serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
   serverAddress.sin_port = htons(atoi(argv[2]));
   int addressLEN = sizeof(serverAddress);
   // Update the structure with the users specified values

   int lfd = socket(AF_INET, SOCK_STREAM, 0);
   if (lfd == -1)
   {
      fprintf(stderr, "[!]socket() error.\n");
      exit(-1);
   }
   // Create a socket and error check
   int optval = 1;
   if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
   {
      fprintf(stderr, "[!]setsockopt() error.\n");
      exit(-1);
   }
   // Set socket options and error check
   int rc = bind(lfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr));
   if (rc == -1)
   {
      fprintf(stderr, "[!]bind() error.\n");
      exit(-1);
   }
   // Bind the socket to the specified port and error check
   if (listen(lfd, BUFSIZE) == -1)
   {
      fprintf(stderr, "[!]listen() error.\n");
      exit(-1);
   }
   // Listen for connections and error check
   fprintf(stdout, "[+]Listening on %s:%d \n[=]<Press ctrl-C to terminate>\n", argv[1], atoi(argv[2]));
   // Announce that we are now listening
   while ((cfd = accept(lfd, (struct sockaddr *)&serverAddress, (socklen_t *)&addressLEN)) != -1)
   {
      // While we dont have an error accepting, wait to accept new connections
      if (fork() == 0)
      {
         // Once we have an accepted connection
         fprintf(stdout, "[+]Connection accepted with ID:%d\n", cfd);
         // Print that we have accepted the connection
         return communication(cfd); // Run the communication function in the child, kill child after execution
      }
      else
      {
         wait(NULL);                                                // Wait for the child process to end
         fprintf(stdout, "[-]Connection closed with ID:%d\n", cfd); // Print that we have ended the connection
         if (close(cfd) == -1)                                      /* Close connection */
         {
            fprintf(stderr, "[!]close error.\n");
            exit(EXIT_FAILURE);
         }
      }
   }
   if (cfd == -1)
   {
      // if we had an accept error, print the error and exit
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

int communication(int cfd) // Communication function that runs the quiz communication for a given client
{
   int correct = 0;
   char *returned;
   int *questions;
   OUTBOUND(cfd, prompt, 0, 1);                                  // Send the prompt, marked with call = 0 to send all lines, end = 1 to get the user input
   returned = INBOUND(cfd);                                      // Get the user input
   if (strcmp(returned, "Y") == 0 || strcmp(returned, "y") == 0) // If the user replied with a perm of yes
   {
      questions = (int *)malloc(5 * sizeof(int)); // Then malloc the questions array
      MAKERANDOMS(questions);                     // Call makerandoms to make the radoms for the questions
      for (int i = 0; i < 5; i++)                 // For each of the 5 questions
      {
         OUTBOUND(cfd, QuizQ, questions[i] + 1, 1);                   // Send the question, terminated with the EOF character
         returned = INBOUND(cfd);                                     // Then capture the users response
         if (strcmp(returned, "") == 0 || strcmp(returned, " ") == 0) // We want to ignore accidental empty inputs
         {
            i--;      // Decrement i
            continue; // Continue
         }
         else if (strcmp(returned, QuizA[questions[i]]) == 0) // If the respone is the same as the answer (case and formatting sensitive)
         {
            OUTBOUND(cfd, response, 1, 0); // Send the correct answer response
            correct++;                     // Count the correct answer
         }
         else // Otherwise the answer is wrong
         {
            OUTBOUND(cfd, response, 2, 0);             // Send the incorrect answer string
            OUTBOUND(cfd, QuizA, questions[i] + 1, 0); // Followed by the correct answer
            OUTBOUND(cfd, response, 4, 0);             // Then a newline character
         }
      }
      OUTBOUND(cfd, response, 5 + correct, 1); // When the quiz has ended, print the number of correct answers the user got
      correct = 0;                             // Reset the correct value, this is just to ensure that no error occurs if the client disconnects before the end of the quiz
      free(questions);                         // Free the questions array
   }
   else
   {
      OUTBOUND(cfd, response, 3, 1); // Otherwise, if the user does not want to do the quiz, (q or otherwise), send the goodbye message
   }
   return 0;
}
void OUTBOUND(int cfd, char **data, int call, int end) // OUTBOUND function is used to send data
{
   char endmarker[] = {EOF, '\0'}; // The endmarker that will send to mark the end of transmission
   if (call == 0)                  // If the call is zero, we want to send the inital  prompt
      for (int i = 0; i < 6; i++)
      {
         send(cfd, data[i], strlen(data[i]), 0); // Send each line in the prompt
      }
   else
   {
      send(cfd, data[call - 1], strlen(data[call - 1]), 0); // Send the specific line
   }
   if (end == 1)
      send(cfd, endmarker, 2, 0); // Send EOF to mark end of transmission
   return;
}

char *INBOUND(int cfd) // Recieve the response from the client
{
   static char buff[BUFSIZE];
   int nBYTES = recv(cfd, buff, BUFSIZE, 0); // receiving the prompt
   buff[nBYTES] = '\0';                      // Terminate recieved data with the null char
   if (nBYTES == -1)                         // If there is an error, print the error
   {
      fprintf(stderr, "[!]recv() error.\n");
      exit(-1);
   }
   else if (nBYTES == 0)
   { // the other connection has closed the socket (ctrl-C)
      return NULL;
   }
   fprintf(stdout, "[+]Response Recieved:%s\n", buff); // Print the recieved response on the server side
   return buff;                                        // return a pointer to the recieved data
}

void MAKERANDOMS(int *listPTR) // write a series of 5 unique random numbers to the supplied array arguent
{
   srand(time(NULL));          // Use the time as a seed to generate random numbers
   for (int i = 0; i < 5; i++) // For each of the 5 numbers to be generated
   {
      listPTR[i] = rand() % 43 + 1; // generate a random number in the i position
      for (int j = 0; j < i; j++)   // For each number we have written before
      {
         if (listPTR[i] == listPTR[j]) // if this value has appeared before
         {
            i--; // regenerate that value so no questions repeat
            break;
         }
      }
   }
   fprintf(stdout, "[=]Questions to be given [%d] [%d] [%d] [%d] [%d]\n", listPTR[0], listPTR[1], listPTR[2], listPTR[3], listPTR[4]);
   // Print the questions we are going to ask
   return;
}
