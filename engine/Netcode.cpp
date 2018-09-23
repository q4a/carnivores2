/*
	Rexhunter99~
	This is a mess, feel free to try your hand at it since I lost the code that was nice and neat.
	It's a simple setup, basically it's a threaded state machine.
	The idea will be to give the functions appropriate names like:
	NET_GameConnect()
	or
	Network::GameConnect()
	Where Network is a namespace, this will help keep the standard game code clean while encapsulating the important network code in a container.
*/

#include "Hunt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace std;

#if defined( AF_PLATFORM_LINUX )
#include <sys/socket.h>
#include <netinet/in.h>

#define SOCKET int
#elif defined( AF_PLATFORM_WINDOWS )
#include <WinSock2.h>

WSADATA wsaData;
#endif

struct sockaddr_in	server_addr;
struct sockaddr_in	client_addr;

SOCKET				ClientSocket;


void InitializeNetwork()
{
	PrintLog( "void InitializeNetwork()\n{\n" );

	/*if( WSAStartup( WINSOCK_VERSION,&wsaData ) )
	{
		PrintLog( "WinSock initialization failed!\n" );
		return;
	}*/

	PrintLog( "}\n" );
}

void ShutdownNetwork()
{
	PrintLog( "void ShutdownNetwork()\n{\n" );
    //WSACleanup();
    PrintLog( "}\n" );
}

void ServerConnect()
{
	// Connect to server.
}
