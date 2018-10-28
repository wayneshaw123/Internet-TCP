
/*  EEEN20060 Communication Systems
    Simple TCP client program, to demonstrate the basic concepts,
    using IP version 4.

    It asks for details of the server, and tries to connect.
    Then it gets a request string from the user, and sends this
    to the server.  Then it waits for a response from the server,
    possibly quite long.  This continues until the string ###
    is found in the server response, or the server closes the
    connection.  Then the client program tidies up and exits.

    This program is not robust - if a problem occurs, it just
    tidies up and exits, with no attempt to fix the problem.  */

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "CS_TCP.h"

#define MAXREQUEST 52    // maximum size of request, in bytes
#define MAXRESPONSE 70   // size of response array, in bytes
#define BLOCKSIZE 100

//******************************************global variable***************************************
int size_of_file;
char file_name[200];
int received_file_size;
char *addFilePath(char *fullFileName, char *fileName);
int findFileSize(char *fName);
   int bytesRead = 0;
//function declaration
//The function to separate header and file
char *stringsplit(char response[]){
   const char s[2] = "\n";
   char *token;
   /* get the first token */
   token = strtok(response, s); // split the header and the file by "\n"
   if(token == NULL){
        printf("Header cannot be found! \n");
        return 0;
   }
   return token;                //extract to get header
}

int status_information(char header[]) {
   const char s[2] = "?";
   char *token;
   int count;
   count = 0;
    int index;
   /* get the first token */
   token = strtok(header, s);
   count++;
   printf("Is the file there(Y/N)? |%s|\n", token);
   if(strcmp(token,"n") == 0 )
    printf("no file exists");
   else{
   /* walk through other tokens */
   while( count<3 ) {
      token = strtok(NULL, s);
      count++;
      if(count == 2){
         for(index=0;index<200;index++){
            file_name[index] = token[index];
         }
         printf("file_name is : |%s|\n", token);
         }
      else if(count == 3){
         size_of_file = atoi(token);  //get the file size
         printf("The size of file is : %d\n", size_of_file);
         return size_of_file;
         }
      }
   }
   return -1;
}


char *addFilePath(char *fullFileName, char *fileName){

    //Adding folder name in front
    char *filePath = "./TCPstorage/";
    int fullFileSize = strlen(filePath) + strlen(fileName) + 1;

    if((fullFileName = (char *)calloc(fullFileSize, sizeof(char))) == 0){
        perror("sendfile:");
        return NULL;
    }

    strcat(fullFileName, filePath);
    strcat(fullFileName, fileName);

    return fullFileName;
}

int recieveFile(char *fullFileName, int downloadSize, SOCKET clientSocket){
    int numRx = 0;

    char fileData[BLOCKSIZE+1];
    FILE *fp = fopen(fullFileName, "w+b");

    while(bytesRead < downloadSize){
        numRx = recv(clientSocket, fileData, BLOCKSIZE, 0);
        // numRx will be number of bytes received, or an error indicator (negative value)

        if( numRx < 0)  // check for error
        {
            printf("Problem receiving, maybe connection closed by client?\n");
            printError();   // give details of the error
            fclose(fp);
            return 1;   // set the flag to end the loop
        }
        else if (numRx == 0)  // indicates connection closing (but not an error)
        {
            printf("Connection closed by server\n");
            fclose(fp);
            return 1;   // set the flag to end the loop
        }
        else// numRx > 0 => we got some data from the client
        {
            bytesRead += numRx;
            fwrite(fileData, numRx, 1, fp);

        }
    }
    fclose(fp);
    return 0;
}


//  Upload file functions
int sendFile(char *fullFileName, SOCKET clientSocket){
    int nByte=0, retVal=0;
    char dataSend[BLOCKSIZE+2];
    //Open file and check for errors
    FILE *fp = fopen(fullFileName, "rb");
    if (fp == NULL){
        perror("sendFile: File does not exist");
        return -1;
    }

    //write loop
    while(!feof(fp)){
        nByte = (int) fread(dataSend, 1, BLOCKSIZE, fp);
        retVal = send(clientSocket, dataSend, nByte, 0);
        if( retVal == SOCKET_ERROR)  // check for error
        {
            printf("*** Error sending response\n");
            return -1;
        }
    }

    printf("File sent successfully.\n");
    return 0;
}

int findFileSize(char *fName){
    FILE *fp;
    long size;    //file size
    int ret;      //return code from functions

    fp = fopen(fName, "rb"); //open for binary read
    ret = fseek(fp, 0, SEEK_END); // seek to end of file
    if (fp == NULL){
                      //say NO to client
       return -1;
    }
    if ( ret !=0){
      perror("Error in fseek"); // can't seek for the end, error
      fclose(fp);
      return -1;
    }
    return size = ftell(fp); // get current file position from the start
}


///////////////////////////
//void printError(void);  // function to display error messages

int main()
{
    // Create variables needed by this function
    SOCKET clientSocket = INVALID_SOCKET;  // identifier for the client socket
    int retVal;                 // return value from various functions
    int retVal1;
    int numRx;                  // number of bytes received
    int req_Len;              // request string length
    int stop = 0;               // flag to control the loop
    int fileSize = 0;
    char serverIPstr[20];       // IP address of server as a string
    int serverPort;             // server port number
    char request[MAXREQUEST];   // array to hold request from user
    char response[MAXRESPONSE+1]; // array to hold response from server
    char *header;            //define header as a new array
    char mode;
    char name[200];
    char close ='y';
    int ideal_file_size;
    // Print starting message
    printf("\nCommunication Systems client program\n\n");


// ============== CONNECT TO SERVER =============================================

    clientSocket = TCPcreateSocket();  // initialise and create a socket
    if (clientSocket == INVALID_SOCKET)  // check for error
        return 1;       // no point in continuing

    // Get the details of the server from the user
    printf("Enter the IP address of the server: ");
    scanf("%20s", serverIPstr);  // get IP address as string

    printf("Enter the port number on which the server is listening: ");
    scanf("%d", &serverPort);     // get port number as integer
    fgets(request, MAXREQUEST, stdin);  // clear the input buffer

    // Now connect to the server
    retVal = TCPclientConnect(clientSocket, serverIPstr, serverPort);
    if (retVal < 0)  // failed to connect
        stop = 1;    // set the flag so skip to tidy up section


// ============== SEND REQUEST ======================================137.43.169.193
while (close=='y')
{
    if(stop == 0)      // if we are connected
    {
        // Get user request and send it to the server
        //printf("\nEnter request in the form U/D?filename.txt(max %d bytes):  ", MAXREQUEST-2);
        //get the U/D,file name
        printf("Please enter request mode(U/D):");
        scanf(" %c", &mode);
        printf("Please enter file name:");
        scanf("%s", name);
        //check if the first character sent is either U or D
        char *fullFileName = NULL;
        fullFileName = addFilePath(fullFileName, name);
        if(fullFileName == NULL){
            printf("*** Error adding path to folder\n");
        }

        if( mode != 'U' && mode!= 'D'){
            printf("*** Error: It has to be either U(uploading) or D(downloading)\n");
            stop = 1 ;
        }
        else if(mode == 'D'){
            // send() arguments: socket identifier, array of bytes to send,
            // number of bytes to send, and last argument of 0.
            req_Len = sprintf(request, "%c?%s\n", mode, name);
            retVal1 = send(clientSocket, request, req_Len, 0);  // send bytes of
            // retVal will be number of bytes sent, or an error indicator

            if( retVal1 == SOCKET_ERROR){ // check for error
                printf("*** Error sending\n");
                printError();   // print the details
                stop = 1;       // set the flag to skip to tidy up
            }
            else printf("Sent %d bytes, waiting for reply...\n", retVal1);



            // ============== RECEIVE RESPONSE ======================================
            if (req_Len > 0){
                // Wait to receive bytes from the server, using the recv function
                // recv() arguments: socket identifier, array to hold received bytes,
                // maximum number of bytes to receive, last argument 0.
                // Normally, recv() will not return until it receives at least one byte...
                numRx = recv(clientSocket, response, MAXRESPONSE, 0);
                // numRx will be number of bytes received, or an error indicator

                response[numRx] = 0;  // make response array into a string
                header = stringsplit(response);
                printf("This is the whole header received: |%s|\n", header);
                ideal_file_size = status_information(header); // get the name-file_size
                printf("The ideal file size = %d\n", ideal_file_size);
                recieveFile(fullFileName, ideal_file_size, clientSocket);
                received_file_size = bytesRead; // get the real size of file that client received
                if(received_file_size == ideal_file_size)
                    printf("The downloading is successful\n");
            }
        }
        //Upload request
        else if(mode == 'U'){
            ////////////send request/////////////
            //find file size
            fileSize = findFileSize(fullFileName);
            req_Len = sprintf(request, "%c?%s?%d\n", mode, name, fileSize);
            retVal1 = send(clientSocket, request, req_Len, 0);  // send bytes of
            // retVal will be number of bytes sent, or an error indicator
            if( retVal1 == SOCKET_ERROR){ // check for error
                printf("*** Error sending\n");
                printError();   // print the details
                stop = 1;       // set the flag to skip to tidy up
            }
            else printf("Sent %d bytes, waiting for reply...\n", retVal1);
            sendFile(fullFileName,clientSocket);// send the file
            free(fullFileName);
        }
    }
    printf("Would you like to continue the process ?\n");
    scanf(" %c", &close);
    fflush(stdin);
}


    // ============== TIDY UP AND END ======================================

        /* A better client might loop to allow another request from the client.
           This simple client just stops after receiving the full response
           has been received from the server. */

        printf("\nClient is closing the connection...\n");

        // Close the socket and shut down the WSA system
        TCPcloseSocket(clientSocket);

        // Prompt for user input, so window stays open when run outside CodeBlocks
        printf("\nPress enter key to end:");
        getchar();  // wait for character input

        return 1;
}









