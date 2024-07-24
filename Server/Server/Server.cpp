// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include "stdafx.h"
#include "afxsock.h"
#include "Server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

unsigned int PORT = 1234;
const int BUFLEN = 1024;

int main()
{
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		printf("Fatal Error: MFC initialization failed\n");
		return 1;
	}

	// TODO: code application's behaivour here
	CSocket server;
	AfxSocketInit(NULL); // initialize windows socket to use CSocket functions

	server.Create(PORT);
	printf("[LISTENING] Server is listening on port %d...\n", PORT);

	if (server.Listen() == FALSE) {
		printf("Cannot listen on port %d\n", PORT);
		server.Close();
		return 1;
	}

	char* msg = new char[BUFLEN];

	while (true) {
		CSocket conn;
		server.Accept(conn);

		/* *************************** */
		/* TODO: get client's nickname */
		/* *************************** */
		char* nickname = new char[BUFLEN];
		int name_len;
		conn.Receive(&name_len, sizeof(int), 0);
		nickname[name_len] = '\0';
		conn.Receive((char*)nickname, BUFLEN, 0);
		printf("[NEW CONNECTION] Connects with %s\n", nickname);
		

		/* ************************************************************************* */
		/*               TODO: give client's list of available files                 */
		/* Protocol used here is string "END_LIST" to stop when file is fully readed */
		/* Client will stop receiving list files after seeing the message "END_LIST" */
		/* ************************************************************************* */
		conn.Send((char*)("Here are the available files you can download\n\0"), BUFLEN, 0);
		
		// read file avail.txt
		FILE* fptr;
		fptr = fopen("avail/avail.txt", "r");
		while (fgets(msg, BUFLEN, fptr)) {
			conn.Send((char*)msg, BUFLEN, 0);
		}
		conn.Send((char*)("END_LIST\0"), BUFLEN, 0); // protocol
		fclose(fptr);

		/* ********************************** */
		/* TODO: keep track of file input.txt */
		/* ********************************** */
		while (true) {
			conn.Receive((char*)msg, BUFLEN, 0);
			if (strcmp(msg, "QUIT") == 0) {
				printf("Quitting\n");
				break;
			}
			if (strcmp(msg, "INSTALL") == 0) {
				printf("Downloading file...\n");
			}
		}

		conn.Close();
		printf("Disconnect with client %s\n", nickname);
	}

	server.Close();
    return 0;
}
