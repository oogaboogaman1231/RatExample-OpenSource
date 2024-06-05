#include <iostream>
#include <string>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define AGENT_IP "127.0.0.1"
#define AGENT_PORT 9999
#define AGENT_PASSWORD "defaultpassword"

SOCKET connectToServer(const std::string& ip, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return INVALID_SOCKET;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return serverSocket;
}

int main() {
    SOCKET serverSocket = connectToServer(AGENT_IP, AGENT_PORT);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::string password = AGENT_PASSWORD;
    send(serverSocket, password.c_str(), password.length(), 0);

    char buffer[512];
    int result;
    while ((result = recv(serverSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[result] = '\0';
        std::cout << "Received: " << buffer << std::endl;
    }

    if (result == 0) {
        std::cout << "Server disconnected" << std::endl;
    } else {
        int error = WSAGetLastError();
        std::cerr << "Error receiving from server: " << error << std::endl;
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
