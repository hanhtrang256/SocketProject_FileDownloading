// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
// xfgddgfddhdhfd
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include "stdafx.h"
#include "afxsock.h"
#include "Server.h"

#define asd dsdas
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

unsigned int PORT = 1234;
const int BUFLEN = 1024;

string toString(char* msg) {
	string s = "";
	for (int i = 0; i < strlen(msg); ++i) s += msg[i];
	return s;
}

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
		int name_len;
		conn.Receive(&name_len, sizeof(int), 0);
		char* nickname = new char[name_len + 1];
		nickname[name_len] = '\0';
		conn.Receive((char*)nickname, name_len, 0);
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

		/* *********************************************** */
		/* TODO: receive client's files and transfer files */
		/* *********************************************** */
		while (true) {
			conn.Receive((char*)msg, BUFLEN, 0);
			if (strcmp(msg, "QUIT") == 0) {
				printf("Quitting\n");
				break;
			}
			if (strcmp(msg, "INSTALL") == 0) {
				printf("Downloading file...\n");
				// Getting downloading files from client
				while (true) {
					conn.Receive((char*)msg, BUFLEN, 0); // msg is filename
					if (strcmp(msg, "END_LIST") == 0) break;
					
					// remove \n from filename
					for (int i = 0; i < strlen(msg); ++i) {
						if (msg[i] == '\n') {
							msg[i] = '\0';
							break;
						}
					}

					printf("Client %s wants to download file %s ", nickname, msg);

					string s = toString(msg);
					
					fptr = fopen(("avail/" + s).c_str(), "r");
					// check if file exists in server
					if (fptr == NULL) {
						printf("(non_exist)\n");
						conn.Send((char*)("NON_EXIST"), BUFLEN, 0);
						continue;
					}
					printf("(exist)\n");
					conn.Send((char*)("EXIST"), BUFLEN, 0);

					// downloading file for clients
					//while (fgets(msg, BUFLEN, fptr)) conn.Send((char*)msg, BUFLEN, 0);
					//conn.Send((char*)"END_FILE", BUFLEN, 0);
					
					fclose(fptr);
				}
			}
		}

		conn.Close();
		printf("Disconnect with client %s\n", nickname);
	}

	server.Close();
    return 0;
}
