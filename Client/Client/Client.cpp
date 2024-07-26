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

void f(int signum) {
	exit(EXIT_SUCCESS);
}

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
	AfxSocketInit(NULL);
	CSocket client;
	client.Create();

	char* msg = new char[BUFLEN];

	if (client.Connect(_T("10.124.7.189"), PORT) != 0) {
		printf("Successfully connect to the server\n");
		/* *************************** */
		/*     TODO: Input nickname    */
		/* *************************** */
		signal(SIGINT, f);
		printf("Enter your nickname length: ");
		int name_len;
		scanf("%d", &name_len);
		client.Send(&name_len, sizeof(int), 0);
		
		char* nickname = new char[name_len + 1];
		nickname[name_len] = '\0';
		printf("Enter nickname: ");
		scanf("%s", nickname);
		client.Send((char*)nickname, name_len, 0);

		/* ******************************* */
		/* TODO: Receive available files   */
		/* Protocol: message "END_LIST" to */
		/* stop the while loop.            */
		/* ******************************* */
		while (true) {
			int sz;
			client.Receive((char*)&sz, sizeof(int), 0);
			client.Receive((char*)msg, sz + 1, 0);
			if (strcmp(msg, "END_LIST\0") == 0) break;
			printf("%s\n", msg);
		}
		/* **************************************************************  */
		/* TODO: Download files or CTRL C to break connection              */
		/* Protocol: when client type install, client_socket will send     */
		/* message "INSTALL" to server to download files. When users press */
		/* CTRL+C, client_socket will send to server the message "QUIT" to */
		/* break the connection.                                           */
		/* *************************************************************** */
		printf("\n");
		int ch = getchar();
		bool running = true;
		FILE* fptr = fopen("input.txt", "r");
		while (running) {
			printf("Type 'install' to download file or press CTRL + C to stop the program : ");
			int index = 0;
			int ch;
			while (true) {
				ch = getchar();
				if (ch == '\n') {
					msg[index] = '\0';
					if (compareStr(msg, "install")) {
						// Send "INSTALL" protocol and filenames to be downloaded
						// "END_LIST" msg is sent when there are no filenames left
						int sz = strlen("INSTALL");
						client.Send((char*)&sz, sizeof(int), 0);
						client.Send((char*)("INSTALL"), sz + 1, 0);
						bool flag = false;
						while (fgets(msg, BUFLEN, fptr)) {
							flag = true;
							msg[strlen(msg)] = '\0';
							// removing \n from filename
							for (int i = 0; i < strlen(msg); ++i) {
								if (msg[i] == '\n') {
									msg[i] = '\0';
									break;
								}
							}
							if (strlen(msg) == 0) continue;
							int sz_filename = strlen(msg);
							client.Send((char*)&sz_filename, sizeof(int), 0);
							client.Send((char*)msg, sz_filename + 1, 0);

							char* tmp = new char[strlen(msg) + 1];
							for (int i = 0; i < strlen(msg); ++i) tmp[i] = msg[i];
							tmp[strlen(msg)] = '\0';

							// get message protocol EXIST to know if file exists in the server
							int sz_exist;
							client.Receive((char*)&sz_exist, sizeof(int), 0);
							client.Receive((char*)msg, sz_exist + 1, 0);
							if (strcmp(msg, "NON_EXIST") == 0) {
								printf("File %s does not exist in the server\n", tmp);
								continue;
							}
							printf("File %s exists in the server\n", tmp);

							string s = toString(tmp);
							FILE* fout = fopen(("output/" + s).c_str(), "wb");
							if (fout == NULL) {
								printf("Cannot create file %s\n", tmp);
							}
							
							// download
							char* buffer;
							size_t size = 0;
							int file_size = 0;
							client.Receive((char*)&file_size, sizeof(int), 0);

							printf("File size is %d\n", file_size);

							buffer = new char[file_size + 1];

							while (client.Receive(buffer, file_size, size)) {
								if (strcmp(buffer, "END_FILE") == 0) {
									printf("Finish downloading\n");
									break;
								}
								buffer[file_size] = '\0';
								//printf("%s ", buffer);
								int len = strlen(buffer);
								//printf("%d ", len);
								fwrite(buffer, 1, file_size, fout);
							}
							fclose(fout);
							printf("Download succeed\n");

							delete[] buffer;
						}
						int sz_endlist = strlen("END_LIST");
						client.Send((char*)&sz_endlist, sizeof(int), 0);
						client.Send((char*)("END_LIST"), sz_endlist + 1, 0);	
						if (!flag) {
							printf("All files are already downloaded\n");
							break;
						}
					}
					else {
						printf("Invalid command. Please type again!\n");
					}
					break;
				}
				if (ch == -1) { // ctrl + c
					int sz_quit = strlen("QUIT");
					client.Send((char*)&sz_quit, sizeof(int), 0);
					client.Send((char*)("QUIT"), sz_quit + 1, 0);
					running = false;
					fclose(fptr);
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
