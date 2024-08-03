// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
#define _CRT_SECURE_NO_WARNINGS

#include "stdio.h"
#include "conio.h"
#include <cstring>
#include "stdafx.h"
#include "afxsock.h"
#include "Client.h"
#include "signal.h"
#include "errno.h"
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

volatile sig_atomic_t running = 1;

void handle_sigint(int signum) {
	running = 0;
}

string toString(char* msg) {
	string s = "";
	for (int i = 0; i < strlen(msg); ++i) s += msg[i];
	return s;
}

char* standard(char* msg, int len = -1) {
	int index = strlen(msg) - 1;
	if (len != -1) index = len - 1;
	while (msg[index] == '\n' || msg[index] == ' ') --index;
	msg[index + 1] = '\0';
	return msg;
}

int main()
{
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		printf("Fatal Error: MFC initialization failed\n");
		fflush(stdout);
		return 1;
	}

	// TODO: code application's behaivour here
	AfxSocketInit(NULL);
	CSocket client;
	client.Create();

	char* msg = new char[BUFLEN];
	char buffer[BUFLEN] = { 0 };

	if (client.Connect(_T("127.0.0.1"), PORT) != 0) {
		printf("Successfully connect to the server\n");
		fflush(stdout);
		/* *************************** */
		/*     TODO: Input nickname    */
		/* *************************** */
		printf("Enter your nickname length: ");
		int name_len;
		scanf("%d", &name_len);
		client.Send(&name_len, sizeof(int), 0);

		char* nickname = new char[name_len + 1];
		printf("Enter nickname: ");
		fflush(stdout);
		scanf("%s", nickname);
		nickname = standard(nickname, name_len);
		client.Send((char*)nickname, name_len, 0);

		/* ************************************* */
		/* TODO: Receive list of available files */
		/* ************************************* */
		printf("Here are list of available files to download\n");
		int cnt = 0;
		while (true) {
			int len_filename;
			client.Receive((char*)&len_filename, sizeof(int), 0);
			//printf("file size is %d\n", len_filename);
			fflush(stdout);
			client.Receive((char*)msg, len_filename, 0);
			msg = standard(msg, len_filename);
			if (strcmp(msg, "END_LIST") == 0) break;
			printf("%s\n", msg);
			fflush(stdout);
		}

		/* **************************************************************  */
		/* TODO: Download files or CTRL C to break connection              */
		/* Protocol: when client type install, client_socket will send     */
		/* message "INSTALL" to server to download files. When users press */
		/* CTRL+C, client_socket will send the message "QUIT" to break the */
		/* connection.                                                     */
		/* *************************************************************** */

		/*FILE* f_out = fopen("output/trc.jpg", "wb");

		int file_size;
		client.Receive((char*)&file_size, sizeof(int), 0);

		char* buf = new char[file_size + 1];
		int bytes_written = 0;
		while (bytes_written < file_size) {
			int bytes_received = client.Receive((char*)buf, file_size, 0);
			printf("received %d bytes\n", bytes_received);
			fwrite(buf, 1, bytes_received, f_out);
			bytes_written += bytes_received;
		}
		delete[] buf;*/

		// Set up the SIGINT handler
		signal(SIGINT, handle_sigint);
		FILE* fin = fopen("input.txt", "r");

		while (running) {
			// reading command
			printf("Command: ");
			fflush(stdout);
			while (true) {
				if (fgets(msg, 100, stdin) == NULL) {
					if (!running) break;
					running = 0;
					break;
				}

				msg = standard(msg);
				if (strlen(msg) == 0) continue;
				break;
			}
			
			//printf("msg is %s\n", msg);

			// handle client's command
			// user presses ctrl + c
			if (!running) {
				printf("You choose to quit\n");
				fflush(stdout);
				int len_quit = strlen("QUIT");
				client.Send((char*)&len_quit, sizeof(int), 0);
				client.Send((char*)("QUIT"), len_quit, 0);
				continue;
			}

			// user types install
			if (strcmp(msg, "install") == 0) {
				printf("You choose to install\n");
				int len_install = strlen("INSTALL");
				client.Send((char*)&len_install, sizeof(int), 0);
				client.Send((char*)("INSTALL"), len_install, 0);

				// read input.txt to send new files to server
				char* filename = new char[100];
				bool have_new_files = false;
				while (fgets(filename, 100, fin)) {
					have_new_files = true;
					filename = standard(filename);
					int len_filename = strlen(filename);

					if (len_filename == 0) continue;

					// Send new files to server
					printf("New file to download is %s (", filename); 
					fflush(stdout);
					client.Send((char*)&len_filename, sizeof(int), 0);
					client.Send((char*)filename, len_filename, 0);

					// receive EXIST protocol from server to know if file exists
					int len_exist;
					client.Receive((char*)&len_exist, sizeof(int), 0);
					char* exist = new char[len_exist + 1];
					client.Receive((char*)exist, len_exist, 0);
					exist = standard(exist, len_exist);

					if (strcmp(exist, "NON_EXIST") == 0) {
						printf("does not exist in the server)\n");
					}
					else {
						printf("exists in the server)\n");

						// now we receive data for file from server
						string s = toString(filename);
						FILE* fout = fopen(("output/" + s).c_str(), "wb");
						// receive file size
						long long file_size;
						client.Receive((char*)&file_size, sizeof(long long), 0);
						printf("File size is %lld\n", file_size);

						// receiving data from server
						memset(buffer, 0, BUFLEN);

						int len_bar = 32;
						int index_bar = 0;
						
						// format of progress bar: Installing file: *****[20%]
						char progress_bar[123] = "Installing file: ";
						int index = 17;
						progress_bar[index] = '\0';
						
						long long bytes_written = 0;
						while (bytes_written < file_size) {
							int bytes_received = client.Receive((char*)buffer, BUFLEN, 0);
							//printf("Received %d bytes\n", bytes_received);
							fwrite(buffer, 1, bytes_received, fout);
							bytes_written += bytes_received;
							float progress = (float)(bytes_written) / (float)(file_size);
							int pos = len_bar * progress;
							while (index_bar < pos) {
								//printf("*");
								progress_bar[index] = '*';
								++index;
								progress_bar[index] = '\0';
								printf("\r"); fflush(stdout);
								printf("%s [%d%c]", progress_bar, (int)(progress*100), '%');
								fflush(stdout);
								++index_bar;
							}
						}
						printf("\nFinish downloading\n");
						fflush(stdout);
						fclose(fout);
					}

					delete[] exist;
				}
				int len_sendall = strlen("SEND_ALL");
				client.Send((char*)&len_sendall, sizeof(int), 0);
				client.Send((char*)("SEND_ALL"), len_sendall, 0);
				delete[] filename;

				if (!have_new_files) {
					printf("All files are already downloaded\n");
					fflush(stdout);
				}
			}
			// invalid command
			else {
				printf("Invalid command!\n");
				int len_invalid = strlen("INVALID_CMD");
				client.Send((char*)&len_invalid, sizeof(int), 0);
				client.Send((char*)("INVALID_CMD"), len_invalid, 0);
				// then go back and wait for user input again
			}
		}

		fclose(fin);
	}
	else {
		printf("Cannot connect to server");
	}
	client.Close();
	printf("Disconnect from server");
	return 0;
}
