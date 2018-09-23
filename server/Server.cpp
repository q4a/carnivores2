
#include <iostream>
#include <vector>
#include <string>

#if defined( AF_PLATFORM_WINDOWS )

#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#	include <WinSock2.h>
#	include <Ws2tcpip.h>
#	pragma comment( lib, "Ws2_32.lib" )
#	define THREAD_RET DWORD WINAPI
#	define thread_t HANDLE

	WSADATA wsaData;

#elif defined( AF_PLATFORM_LINUX )

#	include <pthread.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <memory.h>
#	include <stdio.h>
#	include <unistd.h>
#	include <errno.h>

#	define WSAGetLastError() errno
#	define ZeroMemory( a, b ) memset( a, 0, b )
#	define closesocket( a ) close( a )
#	define SOCKET			int
#	define INVALID_SOCKET	~0
#	define SOCKET_ERROR		-1
#	define THREAD_RET		void*
#	define thread_t			pthread_t

#endif

using namespace std;

thread_t	g_hHttpThread;
bool		g_Stopping = false;


THREAD_RET HttpThread( void* p_param )
{
	struct addrinfo* result, hints;
	SOCKET listen_socket = INVALID_SOCKET;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// -- Init
	if ( getaddrinfo( 0, "80", &hints, &result ) )
	{
		printf( "Failed to get the address information for \"80\"\n" );
		return 0;
	}

	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
	{
		printf(" Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		return 0;
	}

	// Setup the TCP listening socket
    if ( bind( listen_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR )
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listen_socket);
        return 0;
    }
	freeaddrinfo( result );

	while ( !g_Stopping )
	{
		// -- Process
	}

	// -- Close
	closesocket( listen_socket );

	return 0;
}


int main( int argc, char *argv[] )
{
#ifdef AF_PLATFORM_WINDOWS
	printf( "Starting WinSock 2... " );
	if ( WSAStartup( MAKEWORD( 2,2 ), &wsaData ) )
	{
		printf( "failed to start WinSock 2\n" );
		goto END;
	}
	printf( "[Ok]\n" );
#endif


	while ( true )
	{
		char buffer[128];
		printf( "> " );
		scanf( "%s", buffer );

		char* tok = strtok( buffer, " \n" );

		if ( !strcmp( tok, "stop" ) )
		{
			g_Stopping = true;
			break;
		}
	}


CLEANUP:
#ifdef AF_PLATFORM_WINDOWS
	WSACleanup();
#endif

END:

	pthread_join( g_hHttpThread, NULL );

	//system( "pause" );
	return 0;
}
