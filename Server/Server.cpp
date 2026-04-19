#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>
#include <map>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define IP_ADDRESS "127.0.0.1"
#define TCP_PORT "9000"
#define HTTP_PORT 8080
#define MAX_CLIENTS 5
#define TIMEOUT_SECONDS 60

struct client_type {
    int id;
    SOCKET socket;
    time_t lastActivity;
    string ip;
};

vector<client_type> clients(MAX_CLIENTS);
mutex clientsMutex;

int messageCount = 0;
vector<string> messageLog;

// ---------------- LOGGING ----------------
void logMessage(const string& msg)
{
    ofstream file("server_log.txt", ios::app);
    file << msg << endl;
    file.close();

    messageLog.push_back(msg);
}

// ---------------- BROADCAST ----------------
void broadcast(const string& msg, int senderId)
{
    lock_guard<mutex> lock(clientsMutex);

    for (auto& c : clients)
    {
        if (c.socket != INVALID_SOCKET && c.id != senderId)
        {
            send(c.socket, msg.c_str(), msg.size(), 0);
        }
    }
}

// ---------------- CLIENT HANDLER ----------------
void handleClient(client_type client)
{
    char buffer[1024];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client.socket, buffer, sizeof(buffer), 0);

        if (bytes <= 0)
        {
            cout << "Client " << client.id << " disconnected.\n";
            closesocket(client.socket);
            clients[client.id].socket = INVALID_SOCKET;
            break;
        }

        clients[client.id].lastActivity = time(0);

        string msg = "Client #" + to_string(client.id) + ": " + buffer;

        cout << msg << endl;
        logMessage(msg);
        messageCount++;

        // COMMAND: file access
        if (string(buffer).find("dir") != string::npos)
        {
            system("dir");
        }

        broadcast(msg, client.id);
    }
}

// ---------------- TIMEOUT CHECK ----------------
void timeoutChecker()
{
    while (true)
    {
        this_thread::sleep_for(chrono::seconds(5));

        lock_guard<mutex> lock(clientsMutex);

        time_t now = time(0);

        for (auto& c : clients)
        {
            if (c.socket != INVALID_SOCKET)
            {
                if (difftime(now, c.lastActivity) > TIMEOUT_SECONDS)
                {
                    cout << "Client " << c.id << " timed out.\n";
                    closesocket(c.socket);
                    c.socket = INVALID_SOCKET;
                }
            }
        }
    }
}

// ---------------- HTTP SERVER ----------------
void httpServer()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HTTP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);

    cout << "HTTP Server running on port " << HTTP_PORT << endl;

    while (true)
    {
        SOCKET client = accept(sock, NULL, NULL);

        if (client == INVALID_SOCKET) continue;

        string json =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{"
            "\"active_clients\":" + to_string(MAX_CLIENTS) + ","
            "\"messages\":" + to_string(messageCount) +
            "}";

        send(client, json.c_str(), json.size(), 0);
        closesocket(client);
    }
}

// ---------------- MAIN SERVER ----------------
int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo hints{}, *server = NULL;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, TCP_PORT, &hints, &server);

    SOCKET serverSocket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    bind(serverSocket, server->ai_addr, (int)server->ai_addrlen);
    listen(serverSocket, SOMAXCONN);

    cout << "TCP Server running on port " << TCP_PORT << endl;

    // init clients
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = i;
        clients[i].socket = INVALID_SOCKET;
        clients[i].lastActivity = time(0);
    }

    // threads
    thread(timeoutChecker).detach();
    thread(httpServer).detach();

    while (true)
    {
        SOCKET incoming = accept(serverSocket, NULL, NULL);
        if (incoming == INVALID_SOCKET) continue;

        int id = -1;

        {
            lock_guard<mutex> lock(clientsMutex);

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].socket == INVALID_SOCKET)
                {
                    id = i;
                    break;
                }
            }
        }

        if (id == -1)
        {
            string msg = "Server full";
            send(incoming, msg.c_str(), msg.size(), 0);
            closesocket(incoming);
            continue;
        }

        clients[id].socket = incoming;
        clients[id].lastActivity = time(0);

        string welcome = to_string(id);
        send(incoming, welcome.c_str(), welcome.size(), 0);

        thread(handleClient, clients[id]).detach();
    }

    WSACleanup();
}