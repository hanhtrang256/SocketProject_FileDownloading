// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "conio.h"
#include <cstring>
#include "stdafx.h"
#include "afxsock.h"
#include "Client.h"
#include "signal.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

unsigned int PORT = 1234;
const int BUFLEN = 1024;

bool compareStr(char* msg, string s) {
	for (int i = 0; i < (int)s.size(); ++i) if (msg[i] != s[i]) return false;
	return true;
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
	AfxSocketInit(NULL);
	CSocket client;
	client.Create();

	char* msg = new char[BUFLEN];

	if (client.Connect(_T("127.0.0.1"), PORT) != 0) {
		printf("Successfully connect to the server\n");
		/* *************************** */
		/*     TODO: Input nickname    */
		/* *************************** */
		printf("Enter your nickname length: ");
		int name_len;
		scanf("%d", &name_len);
		client.Send(&name_len, sizeof(int), 0);
		
		char* nickname = new char[BUFLEN];
		nickname[name_len] = '\0';
		printf("Enter nickname: ");
		scanf("%s", nickname);
		client.Send((char*)nickname, BUFLEN, 0);

		/* ******************************* */
		/* TODO: Receive available files   */
		/* Protocol: message "END_LIST" to */
		/* stop the while loop.            */
		/* ******************************* */
		while (true) {
			client.Receive((char*)msg, BUFLEN, 0);
			if (strcmp(msg, "END_LIST\0") == 0) break;
			printf("%s", msg);
		}
		/* **************************************************************  */
		/* TODO: Download files or CTRL C to break connection              */
		/* Protocol: when client type install, client_socket will send     */
		/* message "INSTALL" to server to download files. When users press */
		/* CTRL+C, client_socket will send to server the message "QUIT" to */
		/* break the connection.                                           */
		/* *************************************************************** */
		printf("\n");
		bool flag = true;
		bool running = true;
		while (running) {
			printf("Type 'install' to download file or press CTRL + C to stop the program : ");
			int index = 0;
			int ch;
			while (true) {
				ch = getchar();
				if (flag) {
					ch = getchar();
					flag = false;
				}
				if (ch == '\n') {
					msg[index] = '\0';
					if (compareStr(msg, "install")) client.Send((char*)("INSTALL"), BUFLEN, 0);
					else {
						printf("Invalid command. Please type again!\n");
					}
					break;
				}
				if (ch == -1) { // ctrl + c
					client.Send((char*)("QUIT"), BUFLEN, 0);
					running = false;
					break;
				}
				if (ch != EOF) msg[index++] = ch;
			}
		}
	}
	else {
		printf("Cannot connect to server");
	}
	client.Close();
	printf("Disconnect from server");
	return 0;
}
