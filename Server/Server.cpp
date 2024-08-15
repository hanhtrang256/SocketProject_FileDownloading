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

string getCompressedFileSize(long long fileSize) {
	long long type[3] = { 1LL << 10, 1LL << 20, 1LL << 30 }; // KB, MB, GB
	if (fileSize <= type[0]) return "1KB";
	long long numCompress;
	char byteType;
	for (int i = 2; i >= 0; --i) {
		if (type[i] <= fileSize) {
			numCompress = fileSize / type[i];
			if (i == 2) byteType = 'G';
			else if (i == 1) byteType = 'M';
			else byteType = 'K';
			break;
		}
	}

	string ret = "";
	while (numCompress > 0) {
		ret += (char)(numCompress % 10 + '0');
		numCompress /= 10;
	}

	reverse(ret.begin(), ret.end());
	ret += byteType; ret += 'B';
	return ret;
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
	int clientID = 0;

	while (true) {
		CSocket conn;
		server.Accept(conn);

		printf("~---------------------------[NEW CONNECTION]------------------------------~\n");
		printf("|-------- A client has connected to the server with id %d!!! -------------|\n", clientID);

		/* *************************** */
		/* TODO: Get client's nickname */
		/* *************************** */
		int len_nickname;
		char* nickname;
		conn.Receive((char*)&len_nickname, sizeof(int), 0);
		nickname = new char[len_nickname + 1];
		conn.Receive((char*)nickname, len_nickname, 0);
		nickname = standard(nickname, len_nickname);
		printf("|-------- Client's nickname is %s --------|\n", nickname);
		printf("~-------------------------------------------------------------------------~\n");

		/* ****************************************** */
		/* TODO: Give clients list of available files */
		/* ****************************************** */
		printf("** Give clients list of available files\n");
		FILE* f_list = fopen("avail/avail.txt", "r");
		char* avail_file = new char[100];
		while (fgets(avail_file, 100, f_list)) {
			avail_file = standard(avail_file);
			int len_filename = strlen(avail_file);
			conn.Send((char*)&len_filename, sizeof(int), 0);
			conn.Send((char*)avail_file, len_filename, 0);

			string tmp = toString(avail_file);
			FILE* f_getSz = fopen(("avail/" + tmp).c_str(), "rb");
			long long fileSize;
			fseek(f_getSz, 0L, SEEK_END);
			fileSize = ftell(f_getSz);
			fseek(f_getSz, 0L, SEEK_SET);
			fclose(f_getSz);

			string s = getCompressedFileSize(fileSize);
			int lenCompress = (int)s.size();
			conn.Send((char*)&lenCompress, sizeof(int), 0);
			conn.Send((char*)s.c_str(), lenCompress, 0);
		}
		int len_endlist = strlen("END_LIST");
		conn.Send((char*)&len_endlist, sizeof(int), 0);
		conn.Send((char*)("END_LIST"), len_endlist, 0);
		fclose(f_list);
		delete[] avail_file;

		/* *********************************************** */
		/* TODO: receive client's files and transfer files */
		/* *********************************************** */

		printf("** Waiting for user command...\n");
		// main process -> communicate with user
		bool communicate = true;
		while (communicate) {
			// receive user's command
			int len_cmd;
			conn.Receive((char*)&len_cmd, sizeof(int), 0);
			char* usercmd = new char[len_cmd + 1];
			conn.Receive((char*)usercmd, len_cmd, 0);
			usercmd = standard(usercmd, len_cmd);

			// user wants to disconnect
			if (strcmp(usercmd, "QUIT") == 0) {
				printf("[COMMAND RECEIVED] Client command is %s\n", usercmd);
				fflush(stdout);
				printf("Client %s wants to close connection\n", nickname);
				communicate = false;
			}
			else if (strcmp(usercmd, "INVALID_CMD") == 0) {
				//printf("Client types an invalid command\n");
			}
			else if (strcmp(usercmd, "INSTALL") == 0) {
				printf("[COMMAND RECEIVED] Client command is %s\n", usercmd);
				fflush(stdout);
				// receving user's new files or SEND_ALL protocol to stop
				bool sending = true;
				while (sending) {
					int len_filename;
					conn.Receive((char*)&len_filename, sizeof(int), 0);
					char* filename = new char[len_filename + 1];
					conn.Receive((char*)filename, len_filename, 0);
					filename = standard(filename, len_filename);

					if (strcmp(filename, "SEND_ALL") == 0) {
						printf("\r");
						printf("");
						printf("All new files are sent");
						fflush(stdout);
						sending = false;
					}
					else {
						printf("\nClient %s wants to install file %s\n", nickname, filename);
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

							printf("Sending...\n");
							long long bytes_toClient = 0; // total number of bytes have been sent to client

							while ((bytes_read = fread(buffer, 1, BUFLEN, ftrans)) > 0) {
								int bytes_sent = 0;
								while (bytes_sent < bytes_read) {
									int actual_sent = conn.Send((char*)(buffer + bytes_sent), bytes_read - bytes_sent, 0);
									bytes_sent += actual_sent;
									bytes_toClient += actual_sent;
								}
							}
							printf("Finish transferring file %s!!!\n", filename);
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
		++clientID;
	}

	server.Close();
	return 0;
}
