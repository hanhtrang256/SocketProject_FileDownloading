// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include "stdio.h"
#include "stdafx.h"
#include "afxsock.h"
#include "Client.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

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
	AfxSocketInit(NULL);
	CSocket client;
	client.Create();

	char* msg = new char[BUFLEN];

	if (client.Connect(_T("127.0.0.1"), PORT) != 0) {
		printf("Successfully connect to the server\n");
		// input nickname 
		printf("Enter your nickname length: ");
		int name_len;
		scanf("%d", &name_len);
		client.Send(&name_len, sizeof(int), 0);
		
		char* nickname = new char[BUFLEN];
		nickname[name_len] = '\0';
		printf("Enter nickname: ");
		scanf("%s", nickname);
		client.Send((char*)nickname, BUFLEN, 0);

		// receive available files
		while (true) {
			client.Receive((char*)msg, BUFLEN, 0);
			if (strcmp(msg, "END_LIST\0") == 0) break;
			printf("%s", msg);
		}
	}
	else {
		cout << "Ket noi khong thanh cong";
	}
	client.Close();
	return 0;
}
