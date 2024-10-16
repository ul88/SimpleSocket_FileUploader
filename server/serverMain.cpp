#include<filesystem>
#include<fstream>
#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<string>
#include<sstream>
#include<vector>
#include<thread>
#include<io.h>
#pragma comment(lib,"ws2_32.lib")

#define BUFSIZE 1024
#define SERVERPORT 4444

using namespace std;

const char* directoryPath = "download_file\\";
vector<string> paths;

void init_paths() {

	const char* direct = "download_file";

	for (auto iter : filesystem::directory_iterator(direct)) {
		string p = iter.path().generic_string();
		p.erase(0, p.find("/")+1);
		paths.push_back(p);
	}
}

void send_file(SOCKET client_sock, int idx, sockaddr_in clientaddr) {
	char sign;
	ifstream is(directoryPath + paths[idx - 1], ios::in | ios::binary);
	if (!is) {
		cout << "파일 열기 오류";
		return;
	}

	send(client_sock, paths[idx - 1].c_str(), paths[idx - 1].length() + 1, 0);
	recv(client_sock, &sign, 1, 0);

	is.seekg(0, is.end);
	int file_size = is.tellg();
	is.seekg(0, is.beg);

	send(client_sock, to_string(file_size).c_str(), 
		to_string(file_size).length() + 1, 0);
	recv(client_sock, &sign, 1, 0);

	char* file_data = new char[file_size];
	is.read(file_data, file_size);
	send(client_sock, file_data, file_size, 0);
	recv(client_sock, &sign, 1, 0);
	
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	cout << "[TCP 서버] "<< addr << ":" <<
		ntohs(clientaddr.sin_port) << "로 " << 
		paths[idx - 1] << "를 전송했습니다." << endl;

	is.close();
	delete[] file_data;
}

void recv_file_clientToServer(SOCKET client_sock, sockaddr_in clientaddr) {
	char buf[BUFSIZE];
	int ret;
	ret = recv(client_sock, buf, BUFSIZE, 0);
	if (ret == SOCKET_ERROR || ret == 0) return;

	//디렉토리를 전부 자르고 파일명만 남김
	string temp = buf;
	istringstream ss(temp);
	string path;
	while (getline(ss, path, '\\'));
	paths.push_back(path);

	ofstream out(directoryPath + path, ios::out | ios::binary);
	if (!out) {
		cout << "파일 열기 실패" << endl;
		return;
	}
	
	//파일 사이즈 받아오기
	int file_size;
	ret = recv(client_sock, (char*)&file_size, sizeof(int), 0);

	char* file_data = new char[file_size];
	
	cout << "파일 사이즈 전달 완료 [" << file_size << "]" << endl;
	
	ret = recv(client_sock, file_data, file_size, MSG_WAITALL);

	out.write((char*)file_data, file_size);

	//send(client_sock, "1", 1, 0);
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	cout << "[TCP 서버] " << addr << ":" <<
		ntohs(clientaddr.sin_port) << "가 서버로 "<<path<<"를 전송했습니다."<<endl;
	
	out.close();
	delete[] file_data;
}

void recv_file(SOCKET client_sock, sockaddr_in clientaddr) {
	char addr[INET_ADDRSTRLEN];
	char sign;
	while (!WSAGetLastError()) {
		char buf[BUFSIZE] = {};
		string temp;
		int ret = recv(client_sock, buf, 4, 0);
		if (ret == 0 || ret == SOCKET_ERROR) return;

		int command = buf[ret - 1] - '0';
		int num = 0;

		switch (command) {
		case 1:
			recv_file_clientToServer(client_sock, clientaddr);
			break;
		case 2:
			temp = to_string(paths.size());
			send(client_sock, temp.c_str(), temp.length() + 1, 0);
			//recv(client_sock, &sign, 1, 0);

			for (int i = 0;i < paths.size();i++) {
				temp = to_string(i + 1) + " : " + paths[i];
				send(client_sock, temp.c_str(), temp.length() + 1, 0);
				//recv(client_sock, &sign, 1, 0);
			}
			ret = recv(client_sock, buf, 1024, 0);
			//send(client_sock, "1", 1, 0);
			num = atoi(buf);
			send_file(client_sock, num, clientaddr);
			break;
		case 3:
			temp = to_string(paths.size());
			send(client_sock, temp.c_str(), temp.length() + 1, 0);
			//recv(client_sock, &sign, 1, 0);
			for (int i = 0;i < paths.size();i++) {
				temp = to_string(i + 1) + " : " + paths[i];
				send(client_sock, temp.c_str(), temp.length() + 1, 0);
				//recv(client_sock, &sign, 1, 0);
			}
			break;
		case 4:
			temp = to_string(paths.size());
			send(client_sock, temp.c_str(), temp.length() + 1, 0);
			//recv(client_sock, &sign, 1, 0);

			for (int i = 0;i < paths.size();i++) {
				temp = to_string(i + 1) + " " + paths[i];
				send(client_sock, temp.c_str(), temp.length() + 1, 0);
				//recv(client_sock, &sign, 1, 0);
			}
			ret = recv(client_sock, buf, BUFSIZE, 0);
			//send(client_sock, "1", 1, 0);
			num = atoi(buf);
			if (::remove((directoryPath + paths[num - 1]).c_str()) == 0) {
				paths.erase(paths.begin() + (num - 1));
				cout << "파일 삭제에 성공했습니다." << endl;
			}
			else {
				cout << "파일 삭제에 실패했습니다." << endl;
			}
			break;
		case 7:
			inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
			closesocket(client_sock);
			cout<<"[TCP 서버] 클라이언트 종료: IP 주소="<<addr<<", 포트 번호="<<
				ntohs(clientaddr.sin_port)<<endl;
			return;
		default:
			break;
		}
	}
}
int main()
{	
	init_paths();

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	SOCKET server_sock{ socket(AF_INET, SOCK_STREAM, 0)};
	if (server_sock == INVALID_SOCKET) return 1;

	sockaddr_in sock_addr{ AF_INET, htons(SERVERPORT) };
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (::bind(server_sock, (sockaddr*)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR) return 1;
	if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR) return 1;

	SOCKET client_sock;
	sockaddr_in clientaddr;
	int addrlen;

	addrlen = sizeof(clientaddr);
	client_sock = accept(server_sock, (sockaddr*)&clientaddr, &addrlen);
	if (client_sock == INVALID_SOCKET) return 1;
	
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	cout << endl << "[TCP 서버] 클라이언트 접속 : IP 주소=" << addr
		<< ", 포트 번호=" << ntohs(clientaddr.sin_port) << endl;

	thread th(recv_file, client_sock, clientaddr);

	bool serverFlag = true;
	while (serverFlag) {
		int command;
		cout << "1: 파일 리스트 출력" << endl;
		cout << "2: 파일 삭제" << endl;
		cout << "3: 서버 종료" << endl;
		cin >> command;

		if (command == 1) {
			for (int i = 0;i < paths.size();i++) {
				cout << i + 1 << " : " << paths[i] << endl;
			}
		}
		else if (command == 2) {
			int num;
			if (paths.empty()) {
				cout << "삭제할 파일이 없습니다." << endl;
				continue;
			}

			for (int i = 0;i < paths.size();i++) {
				cout << i + 1 << " : " << paths[i] << endl;
			}
			
			cout << "몇 번 파일을 삭제할건가요? : ";
			cin >> num;
			if (num <= 0 && num > paths.size()) {
				cout << "없는 파일 번호를 선택했습니다." << endl;
				continue;
			}
			if (::remove((directoryPath + paths[num - 1]).c_str()) == 0) {
				paths.erase(paths.begin() + (num - 1));
				cout << "파일 삭제에 성공했습니다." << endl;
			}
			else {
				cout << "파일 삭제에 실패했습니다." << endl;
			}
		}
		else if (command == 3) {
			break;
		}
	}

	th.join();

	closesocket(server_sock);

	WSACleanup();
	return 0;
}