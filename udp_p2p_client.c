#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define SERVER_PORT 6666
#define BUFLEN 512
#define PEERS 10
#define BROADCAST_PORT 1153

int recvfromTimeOutUDP(SOCKET socket, long sec, long usec)

{

	// Setup timeval variable

	struct timeval timeout;

	timeout.tv_sec = sec;

	timeout.tv_usec = usec;

	// Setup fd_set structure

	fd_set fds;

	FD_ZERO(&fds);

	FD_SET(socket, &fds);

	// Return value:

	// -1: error occurred

	// 0: timed out

	// > 0: data ready to be read

	return select(0, &fds, 0, 0, &timeout);

}


int main() {

	SOCKET clientSocket,s;
	struct sockaddr_in server_addr,local_addr,peer_addr;
	struct sockaddr_in broadcast_addr;
	char recvbuf[BUFLEN];
	char messagePeer[50];
	char sendbuf[50];
	int iResult;
	WSADATA wsaData;
	int server_addr_len = sizeof(server_addr);
	int broadcast_addr_len = sizeof(broadcast_addr);
	int peer_addr_len = sizeof(peer_addr);
	int listenPort,flag = 0,i,peerListenPort;
	int peerInfo[PEERS];
	int str[INET_ADDRSTRLEN];
	fd_set read;
	int activity;
	int serverTiming,peerTiming;
	int option;
							

	for (i = 0; i < PEERS; i++) {

		peerInfo[i] = 0;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	printf("Winsock initialized\n");


	//create socket
	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		wprintf(L"connect socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	printf("socket created\n");

	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;//internet address family
	server_addr.sin_port = htons(SERVER_PORT);//server port
	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)//server IP address
	{
		printf("\n inet_pton error occured\n");
		return 1;
	}


	/*
	**set up the server properties of client
	*/

	printf("Enter listening port\n");
	scanf_s("%d", &listenPort);

	
	local_addr.sin_family = AF_INET;//internet address family
	local_addr.sin_port = htons(listenPort);//local port
	local_addr.sin_addr.s_addr = INADDR_ANY;//any incoming interface

	//Bind
	if (bind(clientSocket, (struct sockaddr *)&local_addr, sizeof(local_addr)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}
	printf("Binding done\n");

	
	snprintf(sendbuf, 50, "%d", listenPort);

	//send listening port to server
	iResult = sendto(clientSocket, sendbuf, (int)strlen(sendbuf), 0, (struct sockaddr *)
		&server_addr, server_addr_len);

	if (iResult == SOCKET_ERROR) {
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	printf("Port sent to server\n");
	printf("Waiting for other peers\n");

	broadcast_addr.sin_family = AF_INET;                 //Internet address family 
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);         // Any incoming interface 
	broadcast_addr.sin_port = htons(BROADCAST_PORT);     // Broadcast port

														  //create socket
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) {
		wprintf(L"broadcast failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	//allow multiple listeners on the broadcast address
	int options = 1;
	if ((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&options, sizeof(options))) < 0) {
		printf("%d", WSAGetLastError());
	}

   //bind to the broadcast port	
	if (bind(s, (struct sockaddr *) &broadcast_addr, sizeof(broadcast_addr)) == SOCKET_ERROR) {

		printf("bind() failed : %d\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("Binding broadcast done\n");
	
	while (TRUE) {

			
		serverTiming = recvfromTimeOutUDP(s, 10, 0);

		switch (serverTiming)

		{

		case 0:

			printf("Server: Timeout !...\n");

			break;

		case -1:

			//error
			printf("Server: Some error encountered with code number: %ld\n", WSAGetLastError());

			break;

		default:

				iResult = recvfrom(s, peerInfo, PEERS*sizeof(int), 0, (struct sockaddr*) &server_addr,
					&server_addr_len);

				if (iResult == SOCKET_ERROR) {

					printf("recvfrom() failed with error code : %d", WSAGetLastError());
					exit(EXIT_FAILURE);
				}

				for (i = 0; i < PEERS; i++) {

					if (peerInfo[i] != 0) {

						if (peerInfo[i] == listenPort) {

							printf("This is me at port %d\n", peerInfo[i]);
						}
						else {

							printf("Peer %d is listening at port %d\n", i + 1, peerInfo[i]);
						}

					}

				}//end for
		
		}///end switch
		
			printf("Send message to peer\n");
			printf("Enter port:\n");
			scanf_s("%d", &peerListenPort);
			printf("Enter message :");
			scanf_s("%s", messagePeer, 50); // not null terminated

			memset((char *)&peer_addr, 0, sizeof(peer_addr));
			peer_addr.sin_family = AF_INET;
			peer_addr.sin_port = htons(peerListenPort);
			if (inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr) <= 0)
			{
				printf("\n inet_pton error occured\n");
				return 1;
			}

			//send the message
			if (sendto(clientSocket, messagePeer, (int)strlen(messagePeer), 0, (struct sockaddr *) &peer_addr,
				peer_addr_len) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			//clear the buffer
			memset(recvbuf, '\0', BUFLEN);

			peerTiming = recvfromTimeOutUDP(clientSocket, 10, 0);


			switch (peerTiming)

			{

			case 0:

				printf("Peer: Timeout !...\n");

				break;

			case -1:

				//error
				printf("Peer: Some error encountered with code number: %ld\n", WSAGetLastError());

				break;

			default:

				if (recvfrom(clientSocket, recvbuf, BUFLEN, 0, (struct sockaddr *) &peer_addr, &peer_addr_len) == SOCKET_ERROR)
				{
					printf("recvfrom() failed with error code : %d", WSAGetLastError());
					exit(EXIT_FAILURE);
				}

				printf("\nReceived from peer , ip %s,  port %d \n",
					inet_ntop(AF_INET, &(peer_addr.sin_addr), str, INET_ADDRSTRLEN), ntohs(peer_addr.sin_port));

				printf("Data: %s\n", recvbuf);

			}///end switch
				

		
	}//end while

	 // close the socket
	iResult = closesocket(clientSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	WSACleanup();




	return 0;
}

