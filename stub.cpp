#include <iostream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")

#define AGENT_IP "127.0.0.1"
#define AGENT_PORT 9999
#define AGENT_PASSWORD "defaultpassword"

// Function to connect to server
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

// Function to encrypt message using RSA public key
std::string encryptMessage(const std::string& message, RSA* rsa) {
    std::string encryptedMessage;
    encryptedMessage.resize(RSA_size(rsa));
    int result = RSA_public_encrypt(message.size(), (const unsigned char*)message.c_str(), (unsigned char*)&encryptedMessage[0], rsa, RSA_PKCS1_PADDING);
    if (result == -1) {
        ERR_print_errors_fp(stderr);
        return "";
    }
    return encryptedMessage;
}

// Function to load public key from file
RSA* loadPublicKey(const std::string& pubKeyFile) {
    FILE* pubKeyFP = fopen(pubKeyFile.c_str(), "r");
    if (pubKeyFP == nullptr) {
        return nullptr;
    }
    RSA* rsa = PEM_read_RSA_PUBKEY(pubKeyFP, nullptr, nullptr, nullptr);
    fclose(pubKeyFP);
    return rsa;
}

int main() {
    SOCKET serverSocket = connectToServer(AGENT_IP, AGENT_PORT);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::string pubKeyFile = "public.pem"; // Path to the server's public key file
    RSA* rsa = loadPublicKey(pubKeyFile);
    if (!rsa) {
        std::cerr << "Failed to load public key" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::string password = AGENT_PASSWORD;
    std::string encryptedPassword = encryptMessage(password, rsa);
    if (encryptedPassword.empty()) {
        std::cerr << "Failed to encrypt password" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    send(serverSocket, encryptedPassword.c_str(), encryptedPassword.length(), 0);

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

    RSA_free(rsa);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
