/*  EEEN20060 Communication Systems
    Simple TCP server program, to demonstrate the basic concepts,
    using IP version 4.

    It listens on a specified port until a client connects.
    Then it waits for a request from the client, and keeps trying
    to receive bytes until it finds the end marker.
    Then it sends a long response to the client, which includes
    the last part of the request received.
    Then it tidies up and stops.

    This program is not robust - if a problem occurs, it just
    tidies up and exits, with no attempt to fix the problem.  */


#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "CS_TCP.h"
#include <time.h>

#define SERVER_PORT 32980  // port to be used by the server
#define MAXREQUEST 52      // size of request array, in bytes
#define MAXRESPONSE 90     // size of response array (at least 35 bytes more)
#define ENDMARK 10         // the newline character
#define BLOCKSIZE 100      // number of bytes in each block sent
#define DOWNLOAD 1
#define UPLOAD 2

int MODE=0;

//Function Declarations
char *addFilePath(char *fullFileName, char *fileName);
int sendFile(char *filename, SOCKET connectSocket);
int findFileSize(char *fName);
char *findHeader(char request[]);
struct requestHeader status_information(char header[]);
int fileExists(char *fullFileName);
int recieveFile(char *fullFileName, int fileSize, SOCKET connectSocket);
int checkRequestType(char requestType);

//Stores request information
struct requestHeader{
    char type;
    char fName[300];
    int size;
};

int main()
{
    // Create variables needed by this function
    // The server uses 2 sockets - one to listen for connection requests,
    // the other to make a connection with the client.
    SOCKET listenSocket = INVALID_SOCKET;  // identifier for listening socket
    SOCKET connectSocket = INVALID_SOCKET; // identifier for connection socket
    int retVal;         // return value from various functions
    int numRx;      // number of bytes received
    int numResp;        // number of bytes in response string
    int stop;       // flag to control the loop
    char *loc = NULL;          // location of character
    char request[MAXREQUEST+1];   // array to hold request from client
    char response[MAXRESPONSE+1]; // array to hold our response
    char success = 'y';    //response to user request

    // Print starting message
    printf("\nCommunication Systems server program\n\n");


// ============== SERVER SETUP ===========================================

    listenSocket = TCPcreateSocket();  // initialise and create a socket
    if (listenSocket == INVALID_SOCKET)  // check for error
        return 1;       // no point in continuing

    // Set up the server to use this socket and the specified port
    retVal = TCPserverSetup(listenSocket, SERVER_PORT);
    if (retVal < 0) // check for error
        stop = 1;   // set the flag to prevent other things happening

// ============== WAIT FOR CONNECTION ====================================

    // Listen for a client to try to connect, and accept the connection
    connectSocket = TCPserverConnect(listenSocket);
    if (connectSocket == INVALID_SOCKET)  // check for error
        stop = 1;   // set the flag to prevent other things happening


// ============== RECEIVE REQUEST ======================================

   do {
        stop = 0;
        numRx = 0;

        // Loop to receive data from the client, until the end marker is found
        while (stop == 0)   // loop is controlled by the stop flag
        {
            // Wait to receive bytes from the client, using the recv function
            // recv() arguments: socket identifier, array to hold received bytes,
            // maximum number of bytes to receive, last argument 0.
            // Normally, this function will not return until it receives at least one byte...
            numRx = recv(connectSocket, request, MAXREQUEST, 0);
            // numRx will be number of bytes received, or an error indicator (negative value)

            if( numRx < 0)  // check for error
            {
                printf("Problem receiving, maybe connection closed by client?\n");
                printError();   // give details of the error
                stop = 2;   // set the flag to end the loop
            }
            else if (numRx == 0)  // indicates connection closing (but not an error)
            {
                printf("Connection closed by client\n");
                stop = 2;   // set the flag to end the loop
            }
            else // numRx > 0 => we got some data from the client
            {
                request[numRx] = 0;  // add 0 byte to make request into a string

                // Check to see if the request contains the end marker
                loc = memchr(request, ENDMARK, numRx);  // search the array
                if (loc != NULL)  // end marker was found
                {
                    printf("Request contains end marker\n");
                    stop = 1;   // set the flag to end the loop
                }

            } // end of if data received

        } // end of while loop


    // ============== SEND RESPONSE ======================================

        // If we received a request, then we send a response
        if (numRx > 0 && stop == 1)
        {
            char *header;
            struct requestHeader reqHeader;

            header = findHeader(request);
            reqHeader = status_information(header);

             // Print details of the request
            printf("\nRequest received, %d bytes: '%c' '%s' '%d'\n",
                   numRx, reqHeader.type, reqHeader.fName, reqHeader.size);

            // Check request type
            if(checkRequestType(reqHeader.type) == -1){
                printf("*** Error finding request type\n");
                success = 'n';
            }

            //function for adding a specified folder
            char *fullFileName = NULL;
            fullFileName = addFilePath(fullFileName, reqHeader.fName);
            if(fullFileName == NULL){
                printf("*** Error adding path to folder\n");
                success = 'n';
            }

            if(MODE == UPLOAD){

                //check if there are no files with similar names
                if(fileExists(fullFileName)){
                    printf("***Error: File already exits\n");
                    success = 'n';
                }

                else if(reqHeader.size == -1){
                    printf("***Error: Invalid file size received\n");
                    success = 'n';
                }

                //if not - create a file with that file name
                //print the incoming data to the file
                else if(recieveFile(fullFileName, reqHeader.size, connectSocket)){
                    printf("***Error: Full file was not received\n");
                    success = 'n';
                }

                //send response
                numResp = sprintf(response, "%c?%s?%d\n", success, reqHeader.fName, reqHeader.size);

                // Send it to the client.
                retVal = send(connectSocket, response, numResp, 0);  // send bytes
                if( retVal == SOCKET_ERROR)  // check for error
                {
                    printf("*** Error sending response\n");
                    printError();
                }
                else printf("Sent response of %d bytes: %s\n", retVal, response);

            }

            if(MODE == DOWNLOAD){
                //Find size of file
                int fileSize = findFileSize(fullFileName);
                if(fileSize == -1){
                    printf("*** Error finding file size\n");
                    success = 'n';
                }

                //send size as part of header (size will need to be converted into a string
                numResp = sprintf(response, "%c?%s?%d\n", success, reqHeader.fName, fileSize);
                // Send it to the client.
                retVal = send(connectSocket, response, numResp, 0);  // send bytes
                if(retVal == SOCKET_ERROR)  // check for error
                {
                    printf("*** Error sending response\n");
                    printError();
                }
                else printf("Sent response of %d bytes: %s\n", retVal, response);

                //send file if no error has occurred
                if(success == 'y')
                    if(sendFile(fullFileName, connectSocket) == -1)
                        printf("*** Error sending file\n");
            }
            free(fullFileName);


        }  // end of if we received a request

   } while(stop != 2);// end if connection closed by client


// ============== TIDY UP AND END ======================================

    /* A better server might loop to deal with another request from the
       client or to wait for another client to connect.  This one stops
       after dealing with one request from one client. */

    printf("\nServer is closing the connection...\n");

    // Close the connection socket first
    TCPcloseSocket(connectSocket);

    // Then close the listening socket
    TCPcloseSocket(listenSocket);

    // Prompt for user input, so window stays open when run outside CodeBlocks
    printf("\nPress enter key to end:");
    gets(response);

    return 0;
}

/*  Searches for the first newline character in a request in order
    to find the header information. */
char* findHeader(char request[]){
    const char s[2] = "\n";
    char *token;

    /* get the first token */
    token = strtok(request, s);
    return token;
}

/*  Splits the header string into its 3 components: request type,
    file name and size. This information is separated by ? in the
    header string. The split header information is then stored in
    a structure. */
struct requestHeader status_information(char header[]) {
    const char s[2] = "?";
    char *token;
    int count=0;
    struct requestHeader reqHeader;

    // gets the first token
    token = strtok(header, s);
    count++;
    reqHeader.type = header[0];

    while( token != NULL && (count<3) ) {
        token = strtok(NULL, s);
        count++;
        if(count == 2)
            strcpy(reqHeader.fName, token); //gets file name
        else if(count == 3)
            reqHeader.size = atoi(token);  //gets the file size
    }
    return reqHeader;
}

/*  Adds the folder path in front of the received file name. The
    full file name is dynamically allocated.
    Note: There must be a folder named TCPstorage where the program
    is being run from in order for the server to locate the files.
    It may be easier to replace "./TCPstorage/" with a full address
    to a folder.
    */
char *addFilePath(char *fullFileName, char *fileName){

    //Adding folder name in front
    char *filePath = "./TCPstorage/";
    int fullFileSize = strlen(filePath) + strlen(fileName) + 1;

    if((fullFileName = (char *)calloc(fullFileSize, sizeof(char))) == 0){
        perror("addFilePath:");
        return NULL;
    }
    strcat(fullFileName, filePath);
    strcat(fullFileName, fileName);

    return fullFileName;
}

/*  Opens a file and then loops through the file contents sending a
    BLOCKSIZE of data each time. If there are any errors opening or
    sending the file, it returns -1. */
int sendFile(char *fullFileName, SOCKET connectSocket){
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
        retVal = send(connectSocket, dataSend, nByte, 0);
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

/*  Checks to see if the file already exists in the TCPstorage folder so
    no file is overwritten. */
int fileExists(char *fullFileName){
    FILE *fp = fopen(fullFileName, "r");
    if (fp){
        fclose(fp);
        return 1;
    }
    return 0;
}

int recieveFile(char *fullFileName, int uploadSize, SOCKET connectSocket){
    int numRx = 0;
    int bytesRead = 0;
    char fileData[BLOCKSIZE];
    FILE *fp = fopen(fullFileName, "w+b");

    while(bytesRead < uploadSize){
        numRx = recv(connectSocket, fileData, BLOCKSIZE, 0);
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
            printf("Connection closed by client\n");
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

int checkRequestType(char requestType){
    if(requestType == 'U') MODE = UPLOAD;
    else if(requestType == 'D') MODE = DOWNLOAD;
    else return -1;
    return 0;
}



