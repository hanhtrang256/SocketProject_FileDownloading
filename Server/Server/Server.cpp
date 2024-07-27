// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include "stdafx.h"
#include "afxsock.h"
#include "Server.h"


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

char* standard(char* msg, int len = -1) {
	int index = strlen(msg) - 1;
	if (len != -1) index = len - 1;
	while (msg[index] == '\n' || msg[index] == ' ') --index;
	msg[index + 1] = '\0';
	return msg;
}

void progress_bar(int bytes_sent, int file_size) {
	int len_bar = 50;
	float progress = (float)(bytes_sent) / (float)(file_size);
	int pos = progress * len_bar;

	printf("[");
	for (int i = 0; i < len_bar; ++i) {
		if (i < pos) printf("=");
		else if (i == pos) printf(">");
		else printf(" ");
	}
	printf("] %3.0f%%\r", progress * (float)100.0);
	printf("\n");
	fflush(stdout);
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
	char buffer[BUFLEN] = { 0 };

	while (true) {
		CSocket conn;
		server.Accept(conn);

		printf("---------------------------[NEW CONNECTION]------------------------------\n");
		printf("A client has connected to the server!!!\n");

		/* *************************** */
		/* TODO: Get client's nickname */
		/* *************************** */
		int len_nickname;
		char* nickname;
		conn.Receive((char*)&len_nickname, sizeof(int), 0);
		nickname = new char[len_nickname + 1];
		conn.Receive((char*)nickname, len_nickname, 0);
		nickname = standard(nickname, len_nickname);
		printf("Client's nickname is %s\n", nickname);

		/* ****************************************** */
		/* TODO: Give clients list of available files */
		/* ****************************************** */
		printf("Give clients list of available files\n");
		FILE* f_list = fopen("avail/avail.txt", "r");
		char* avail_file = new char[100];
		while (fgets(avail_file, 100, f_list)) {
			avail_file = standard(avail_file);
			int len_filename = strlen(avail_file);
			conn.Send((char*)&len_filename, sizeof(int), 0);
			conn.Send((char*)avail_file, len_filename, 0);
		}
		int len_endlist = strlen("END_LIST");
		conn.Send((char*)&len_endlist, sizeof(int), 0);
		conn.Send((char*)("END_LIST"), len_endlist, 0);
		fclose(f_list);

		/* *********************************************** */
		/* TODO: receive client's files and transfer files */
		/* *********************************************** */

		/*FILE* f_down = fopen("avail/trc.jpg", "rb");
		if (f_down == NULL) {
			printf("Cannot open file\n");
			fflush(stdout);
		}

		int file_size;
		fseek(f_down, 0L, SEEK_END);
		file_size = ftell(f_down);
		fseek(f_down, 0L, SEEK_SET);
		printf("File size is %d\n", file_size);

		conn.Send((char*)&file_size, sizeof(int), 0);

		int bytes_read;
		while ((bytes_read = fread(buffer, 1, BUFLEN, f_down)) > 0) {
			printf("read %d bytes\n", bytes_read);
			int bytes_sent = 0;
			while (bytes_sent < bytes_read) {
				int actual_sent = conn.Send((char*)(buffer + bytes_sent), bytes_read - bytes_sent, 0);
				if (actual_sent < 0) {
					printf("Failed\n");
					exit(EXIT_FAILURE);
				}
				bytes_sent += actual_sent;
			}
		}

		printf("Finished\n");
		fflush(stdout);
		fclose(f_down);*/

		// main process -> communicate with user
		bool communicate = true;
		while (communicate) {
			// receive user's command
			int len_cmd;
			conn.Receive((char*)&len_cmd, sizeof(int), 0);
			char* usercmd = new char[len_cmd + 1];
			conn.Receive((char*)usercmd, len_cmd, 0);
			usercmd = standard(usercmd, len_cmd);

			printf("[COMMAND RECEIVED] Client command is %s\n", usercmd);
			fflush(stdout);

			// user wants to disconnect
			if (strcmp(usercmd, "QUIT") == 0) {
				printf("Quitting\n");
				communicate = false;
			}
			else if (strcmp(usercmd, "INVALID_CMD") == 0) {
				printf("Client types an invalid command\n");
			}
			else if (strcmp(usercmd, "INSTALL") == 0) {
				// receving user's new files or SEND_ALL protocol to stop
				bool sending = true;
				while (sending) {
					int len_filename;
					conn.Receive((char*)&len_filename, sizeof(int), 0);
					char* filename = new char[len_filename + 1];
					conn.Receive((char*)filename, len_filename, 0);
					filename = standard(filename, len_filename);

					if (strcmp(filename, "SEND_ALL") == 0) {
						printf("All new files are sent\n\n");
						fflush(stdout);
						sending = false;
					}
					else {
						printf("Client wants to install file %s\n", filename);
						fflush(stdout);

						// check if file exist in server
						string s = toString(filename);
						FILE* ftrans = fopen((("avail/") + s).c_str(), "rb");

						// file does not exist
						if (ftrans == NULL) {
							printf("File %s does not exist in server\n", filename);
							fflush(stdout);
							int len_nonexist = strlen("NON_EXIST");
							conn.Send((char*)&len_nonexist, sizeof(int), 0);
							conn.Send((char*)("NON_EXIST"), len_nonexist, 0);
							// then continue receiving other files from client
						}
						else {
							printf("File %s exists in server\n", filename);
							fflush(stdout);
							int len_exist = strlen("EXIST");
							conn.Send((char*)&len_exist, sizeof(int), 0);
							conn.Send((char*)("EXIST"), len_exist, 0);

							// now we transfer data in file to client

							long long file_size;
							fseek(ftrans, 0L, SEEK_END);
							file_size = ftell(ftrans);
							fseek(ftrans, 0L, SEEK_SET);

							printf("File size is %lld\n", file_size);
							fflush(stdout);

							// send file size to client to write file
							conn.Send((char*)&file_size, sizeof(long long), 0);

							int bytes_read;
							memset(buffer, 0, BUFLEN);

							long long bytes_toClient = 0; // total number of bytes have been sent to client
							
							while ((bytes_read = fread(buffer, 1, BUFLEN, ftrans)) > 0) {
								int bytes_sent = 0;
								while (bytes_sent < bytes_read) {
									int actual_sent = conn.Send((char*)(buffer + bytes_sent), bytes_read - bytes_sent, 0);
									bytes_sent += actual_sent;
									bytes_toClient += actual_sent;
								}
							}
							printf("\nFinish downloading!!!\n\n");
							fflush(stdout);
							fclose(ftrans);
						}
					}
					delete[] filename;
				}
			}

			delete[] usercmd;
		}

		conn.Close();
		printf("Disconnect with client %s!!!\n", nickname);
		delete[] nickname;
	}

	server.Close();
	return 0;
}
