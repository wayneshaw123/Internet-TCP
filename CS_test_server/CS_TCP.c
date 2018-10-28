/* Functions for use in TCP communications.  */

#define WINVER 0x0501   // specify Windows XP as minimum version
#include <stdio.h>      // needed for printf
#include <winsock2.h>   // windows sockets library
#include <ws2tcpip.h>   // windows TCP and IP functions
#include <time.h>       // for timeval
#include "CS_TCP.h"

// Shared variables for all functions in this file
static int socketCount = 0;  // number of sockets created
static WSADATA wsaData;  // structure to hold winsock data

// ======================================================================

/*  Function to create a socket for use with TCP and IPv4.
    It first starts the WSA system if this has not been done already.  */
SOCKET TCPcreateSocket (void)
{
    SOCKET mySocket = INVALID_SOCKET;  // identifier for socket
    int retVal;     // return value from function

    if (socketCount == 0)  // this is the first socket being created
    {
        // So need to initialise the winsock system
        // Arguments are: version (2.2), pointer to data structure
        retVal = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (retVal != 0)  // check for error
        {
            printf("*** WSAStartup failed: %d\n", retVal);
            printError();  // print the details
            return INVALID_SOCKET;  // socket is invalid if error
        }
        else
        {
            printf("WSAStartup succeeded\n" );
        }
    }

    // Now create the socket as requested.
    // AF_INET means IP version 4,
    // SOCK_STREAM means the socket works with streams of bytes,
    // IPPROTO_TCP means TCP transport protocol.
    mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (mySocket == INVALID_SOCKET)  // check for error
    {
        printf("*** Failed to create socket\n");
        printError();  // print the details
        return INVALID_SOCKET;  // socket is invalid if error
    }
    else
    {
        printf("Socket created\n" );
        socketCount++;    // increment the socket counter
        return mySocket;  // return the socket identifier
    }

}  // end of TCPcreateSocket


// ======================================================================

/*  Function to set up a server for use with TCP and IPv4.
    Arguments are an existing socket to use, and a port number.
    It first defines a service, in a struct of type sockaddr_in.
    Then it binds the socket to this service.   */
int TCPserverSetup (SOCKET lSocket, int port)
{
    int retVal;     // for return values from functions;
    struct sockaddr_in service;  // IP address and port structure

    // Configure the service structure with the service to be offered
    service.sin_family = AF_INET;  // specify IP version 4 family
    service.sin_addr.s_addr = htonl(INADDR_ANY);  // set IP address
    // function htonl() converts 32-bit integer to network format
    // INADDR_ANY means accept a connection on any IP address
    service.sin_port = htons(port);  // set port number on which to listen
    // function htons() converts a 16-bit integer to network format

    // Bind the socket to this service (IP address and port)
    retVal = bind(lSocket, (SOCKADDR *) &service, sizeof(service));
    if( retVal == SOCKET_ERROR)  // check for error
    {
        printf("*** Error binding to socket\n");
        printError();       // print the details
        return -1;          // indicate failure
    }
    else printf("Socket bound to port %d, server is ready\n", port);

    return 0;

}  // end of TCPserverSetup


// ======================================================================

/*  Function to use in a server, to wait for a connection request.
    It will then accept the request, making a connection with the client.
    It reports the IP address and port number used by the client.
    Argument is an existing socket, bound to the required port.
    Return value is the socket used for the connection.  */
SOCKET TCPserverConnect (SOCKET lSocket)
{
    SOCKET cSocket = INVALID_SOCKET;  // socket to use for connection
    struct sockaddr_in client;  // structure to identify the client
    int len = sizeof(client);   // initial length of structure
    int clientPort;             // port number being used by client
    struct in_addr clientIP;    // structure to hold client IP address
    int retVal;     // for return values from functions;

    // Set this socket to listen for connection requests.
    // Second argument is maximum number of requests to allow in queue
    retVal = listen(lSocket, 2);
    if( retVal == SOCKET_ERROR)  // check for error
    {
        printf("*** Error trying to listen\n");
        printError();       // print the details
        return INVALID_SOCKET;  // indicate failure
    }
    else printf("\nListening for connection requests...\n");

    // Wait until a connection is requested, then accept the connection.
    cSocket = accept(lSocket, (SOCKADDR *) &client, &len );
    if( cSocket == INVALID_SOCKET)  // check for error
    {
        printf("*** Failed to accept connection\n");
        printError();       // print the details
        return INVALID_SOCKET;    // indicate failure
    }
    else  // we have a connection, print details of the client
    {
        clientPort = client.sin_port;  // get client port number
        clientIP = client.sin_addr;    // get client IP address
        printf("\nAccepted connection from port %d on %s\n",
               ntohs(clientPort), inet_ntoa(clientIP));
        // function ntohs() converts 16-bit integer from network form to normal
        // function inet_ntoa() converts IP address structure to a string
        socketCount++;    // increment the active socket counter
        return cSocket;   // return the socket identifier
    }

}  // end of TCPserverConnect


// ======================================================================

/*  Function to use in a client, to request a connection to a server.
    The IP address is given as a string, in dotted decimal form.
    The port number is given as an integer, and converted to
    network form in this function. */
int TCPclientConnect (
            SOCKET cSocket,     // socket to use to make the connection
            char * IPaddrStr,   // IP address of the server as a string
            int port)           // port number for connection
{
    int retVal;     // for return value of function
    unsigned long IPaddr;   // IP address as a 32-bit integer

    // Get the IP address in integer form
    IPaddr = inet_addr(IPaddrStr); // convert string to integer
    /* inet_addr() converts an IP address string (dotted decimal)
    to a 32-bit integer address in the required form.  */

    // Build a structure to identify the service required
    // This has to contain the IP address and port of the server
    struct sockaddr_in service;  // IP address and port structure
    service.sin_family = AF_INET;       // specify IP version 4 family
    service.sin_addr.s_addr = IPaddr;   // set IP address
    service.sin_port = htons(port);     // set port number
    // function htons() converts 16-bit integer to network format

    // Try to connect to the service required
    printf("Trying to connect to %s on port %d\n", IPaddrStr, port);
    retVal = connect(cSocket, (SOCKADDR *) &service, sizeof(service));
    if( retVal != 0)  // check for error
    {
        printf("*** Error connecting\n");
        printError();   // print the details
        return -1;  // indicate failure
    }
    else
    {
        printf("Connected!\n");  // now connected to server
        return 0;   // success
    }

}  // end of TCPclientConnect


// ======================================================================

/*  Function to close a socket.
    After closing the last socket, it tidies up the WSA system. */
void TCPcloseSocket (SOCKET socket2close)
{
    int retVal;     // for return values of functions

    // First block sending, also signals disconnection to the other end
    retVal = shutdown(socket2close, SD_SEND);  // try to shut down sending
    if (retVal == 0) printf("Socket shutting down...\n");  // success
    else  // some problem occurred
    {
        retVal = WSAGetLastError();  // get the error code
        if (retVal == WSAENOTCONN)  // socket was not connected
            printf("Closing an idle socket\n");
        else if (retVal == WSAECONNRESET)  // connection has been lost
            printf("Connection already closed by the other side\n");
        else
        {
            printf("*** Error shutting down sending\n");
            printError();  // print the details
        }
    }

    // It is now safe to close the socket
    retVal = closesocket(socket2close);  // close the socket
    if( retVal != 0)  // check for error
    {
        printf("*** Error closing socket\n");
        printError();  // print the details
    }
    else
    {
        socketCount--;  // decrement the socket counter
        printf("Socket closed, %d remaining\n", socketCount);
    }

    // If there are no sockets left, clean up the winsock system
    if (socketCount == 0)
    {
        retVal = WSACleanup();
        printf("WSACleanup returned %d\n",retVal);
    }

}  // end of TCPcloseSocket


// ======================================================================

/*  Function to find an IP address from the name of a server.  If the
    server has more than one address, only the first is returned.
    The WSA system must be initialised already, so call TCPcreateSocket()
    before calling this function.
    Arguments:  pointer to a string containing the name of the server (host);
    pointer to an in_addr struct to hold the IP address (or NULL if not needed);
    pointer to a string to hold the IP address in dotted decimal form.
    Return value is 0 for success, negative for error.  */
int getIPaddress(char * hostName, struct in_addr * IPaddr, char * IPaddrStr)
{
    int retVal;     // for return value of function
    struct addrinfo hints;  // struct of type addrinfo
    struct addrinfo * result = NULL;  // pointer to struct of type addrinfo
    struct sockaddr_in * hostAddrPtr; // pointer to address structure
    struct in_addr hostAddr;          // struct to hold an IP address

    // Initialise the hints structure - all zero
    ZeroMemory( &hints, sizeof(hints) );

    // Fill in some parts, to specify what type of address we want
    hints.ai_family = AF_INET;        // IPv4 address
    hints.ai_socktype = SOCK_STREAM;  // stream type socket
    hints.ai_protocol = IPPROTO_TCP;  // using TCP protocol

    // Try to find the address of the host computer
    retVal = getaddrinfo(hostName, NULL, &hints, &result);
    if (retVal != 0)  // attempt has failed
    {
        printf("Failed to get the IP address, error code %d\n", retVal);
        if (retVal == WSAHOST_NOT_FOUND)
            printf("Name %s was not found\n", hostName); // explain why
        else printError();  // or print the details
        return -1;  // report failure
    }
    else  // success

    // One or more result structures have been created by getaddrinfo(),
    // forming a linked list - a computer could have multiple addesses.
    // The result pointer points to the first structure, and this
    // example only uses the first address found.

    // The result structure has many elements - we only want the address
    hostAddrPtr = (struct sockaddr_in*) result->ai_addr; // get pointer to address

    // The address structure has many element - we only want the IP address
    hostAddr = hostAddrPtr->sin_addr;  // get IP address in network form

    // Provide this value if the user wants it
    if (IPaddr != NULL) *IPaddr = hostAddr;  // IP address as struct

    // Convert the IP address to a string (dotted decimal)
    IPaddrStr = inet_ntoa(hostAddr);

    // Free the memory that getaddrinfo allocated for result structure(s)
    freeaddrinfo(result);

    printf("Host %s has IP address %s\n", hostName, IPaddrStr);
    return 0;   // return success

}  // end of getIPaddress


// ======================================================================

/* Function to print error messages when something goes wrong.
   No arguments needed - it finds the problem in the WSA structure.*/
void printError(void)
{
	char lastError[1024];
	int errCode;

	errCode = WSAGetLastError();  // get the error code for the last error
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		lastError,
		1024,
		NULL);  // convert error code to error message
	printf("WSA Error Code %d = %s\n", errCode, lastError);

}  // end of printError
