#include<filesystem>
#include<fstream>
#include<iostream>
#include<string>
#include<thread>
#include<winsock2.h>
#include<ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#define SERVERIP "127.0.0.1"
#define SERVERPORT 4444
#define BUFSIZE 1024
using namespace std;

const char* directoryPath = "download_file\\";

void mkdir() {
	const char* direct = "download_file";
	if (!filesystem::exists(direct))
		filesystem::create_directory(direct);
}


void recv_file(SOCKET sock) {
	char buf[BUFSIZE];
	recv(sock, buf, BUFSIZE, 0);
	send(sock, "1", 1, 0);
	string file_name = buf;

	recv(sock, buf, BUFSIZE, 0);
	send(sock, "1", 1, 0);

	string temp = buf;
	int file_size = stoi(temp);

	char* file_data = new char[file_size];
	recv(sock, file_data, file_size, 0);
	send(sock, "1", 1, 0);

	ofstream out(directoryPath + file_name, ios::out | ios::binary);

	out.write(file_data, file_size);
	cout << file_name << " 다운로드 성공" << endl;

	out.close();
	delete[] file_data;
}

void send_file(SOCKET sock, string path) {
	char sign;
	ifstream is(path, ios::in | ios::binary);
	if (!is) {
		cout << "파일 열기 오류"<<endl;
		is.close();
		return;
	}

	//파일명을 서버로 전송
	send(sock, path.c_str(), strlen(path.c_str())+1, 0);

	is.seekg(0, is.end);
	int file_size = is.tellg();
	is.seekg(0, is.beg);

	send(sock, (char*)&file_size, sizeof(int), 0);
	
	//파일 크기 전송 완료 신호 보냄
	//recv(sock, &sign, 1, 0);
	cout << "파일 크기 전송 완료 [" << file_size <<"]" << endl;

	char* file_data = new char[file_size];
	is.read(file_data, file_size);

	send(sock, (const char*)file_data, file_size, 0);

	//recv(sock, &sign, 1, 0);
	cout << "서버로 파일 전송 성공"<<endl;

	is.close();
	delete[] file_data;
}

void recv_file_list(SOCKET sock) {
	char buf[1024];
	char sign;

	int len = 0;
	recv(sock, buf, 1024, 0);
	send(sock, "1", 1, 0);

	len = atoi(buf);
	for (int i = 0;i < len;i++) {
		memset(buf, 0, sizeof(buf));
		recv(sock, buf, 1024, 0);
		send(sock, "1", 1, 0);
		cout << buf << endl;
	}
}

void client_file_list() {
	const char* direct = "download_file";
	int idx = 0;
	for (auto iter : filesystem::directory_iterator(direct)) {
		string p = iter.path().generic_string();
		p.erase(0, p.find("/") + 1);
		cout << ++idx << " " << p << endl;
	}
}

void client_file_delete() {
	const char* direct = "download_file";
	
	client_file_list();
	
	int num;
	cout << "삭제하고 싶은 파일을 선택해주세요: ";
	cin >> num;

	int idx = 0;
	for (auto iter : filesystem::directory_iterator(direct)) {
		if (++idx == num) {
			string p = iter.path().generic_string();
			if (::remove(p.c_str()) == 0) {
				cout << "파일 삭제에 성공했습니다." << endl;
			}
			else {
				cout << "파일 삭제에 실패했습니다." << endl;
			}
			break;
		}
	}
}

int main()
{
	mkdir();

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	SOCKET sock{ socket(AF_INET, SOCK_STREAM, 0) };
	sockaddr_in addr{ AF_INET, htons(SERVERPORT) };
	inet_pton(AF_INET, SERVERIP, &addr.sin_addr);
	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return 1;

	bool flag = true;
	char sign;
	while (flag) {
		int command, num;
		string path,temp;
		cout << "1. 파일 업로드" << endl;
		cout << "2. 파일 다운로드" << endl;
		cout << "3. 서버에 존재하는 파일 확인" << endl;
		cout << "4. 서버에 파일 삭제" << endl;
		cout << "5. 클라이언에 존재하는 파일 확인" << endl;
		cout << "6. 클라이언트에 파일 삭제" << endl;
		cout << "7. 클라이언트 종료" << endl;
		cin >> command;
		cin.ignore();

		switch (command) {
		case 1:
			cout << "전송할 파일의 위치를 적어주세요." << endl;
			getline(cin, path);
			//경로에서 "" 삭제
			while (path.find("\"") != string::npos)
				path.erase(find(path.begin(), path.end(), '\"'));

			// 서버에 1을 전송
			send(sock, "1", 1, 0);
			
			//파일을 서버에 전송
			send_file(sock, path);
			break;
		case 2:
			send(sock, "2", 1, 0);
			recv_file_list(sock);
			cout << "다운로드하고 싶은 파일을 선택해주세요: ";
			cin >> num;
			send(sock, to_string(num).c_str(), to_string(num).length() + 1, 0);
			
			recv_file(sock);
			break;
		case 3:
			send(sock, "3", 1, 0);
			recv_file_list(sock);
			break;
		case 4:
			send(sock, "4", 1, 0);
			recv_file_list(sock);
			cout << "삭제하고 싶은 파일을 선택해주세요: ";
			cin >> num;
			temp = to_string(num);
			send(sock, temp.c_str(), temp.length() + 1, 0);
			
			break;
		case 5:
			client_file_list();
			break;
		case 6:
			client_file_delete();
			break;
		case 7:
			flag = false;
			send(sock, "7", 1, 0);
			break;
		}
	}
	
	closesocket(sock);
	WSACleanup();
	return 0;
}