#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>
#include <ctime>
#include <filesystem>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

#define IP_ADDRESS "127.0.0.1"
#define TCP_PORT "9000"
#define HTTP_PORT 8080
#define MAX_CLIENTS 5
#define TIMEOUT_SECONDS 60

const string SERVER_FILES_DIR = "server_files";

struct client_type {
    int id;
    SOCKET socket;
    time_t lastActivity;
    string ip;
    string role;
};

vector<client_type> clients(MAX_CLIENTS);
mutex clientsMutex;

int messageCount = 0;
vector<string> messageLog;

// ---------------- Helper functions ----------------
string escapeJson(const string& s)
{
    string out;
    for (char c : s)
    {
        switch (c)
        {
        case '\"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

void sendResponse(SOCKET sock, const string& msg)
{
    send(sock, msg.c_str(), (int)msg.size(), 0);
}

bool isAdminCommand(const string& input)
{
    return input == "/list" ||
        input.rfind("/read ", 0) == 0 ||
        input.rfind("/upload ", 0) == 0 ||
        input.rfind("/download ", 0) == 0 ||
        input.rfind("/delete ", 0) == 0 ||
        input.rfind("/search ", 0) == 0 ||
        input.rfind("/info ", 0) == 0;
}

bool isReadAllowedCommand(const string& input)
{
    return input == "/list" ||
        input.rfind("/read ", 0) == 0 ||
        input.rfind("/download ", 0) == 0 ||
        input.rfind("/search ", 0) == 0 ||
        input.rfind("/info ", 0) == 0;
}

// ---------------- LOGGING ----------------
void logMessage(const string& msg)
{
    ofstream file("server_log.txt", ios::app);
    file << msg << endl;
    file.close();

    lock_guard<mutex> lock(clientsMutex);
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
            send(c.socket, msg.c_str(), (int)msg.size(), 0);
        }
    }
}

// ---------------- File commands ----------------
string handleListCommand()
{
    if (!fs::exists(SERVER_FILES_DIR))
        fs::create_directory(SERVER_FILES_DIR);

    string result;
    for (const auto& entry : fs::directory_iterator(SERVER_FILES_DIR))
    {
        result += entry.path().filename().string() + "\n";
    }

    if (result.empty())
        return "No files found";

    return result;
}

string handleReadCommand(const string& filename)
{
    string path = SERVER_FILES_DIR + "/" + filename;
    ifstream file(path, ios::binary);

    if (!file.is_open())
        return "File not found";

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    if (content.empty())
        return "File is empty";

    return content;
}

string handleDownloadCommand(const string& filename)
{
    return handleReadCommand(filename);
}

string handleUploadCommand(const string& filename, const string& content)
{
    if (!fs::exists(SERVER_FILES_DIR))
        fs::create_directory(SERVER_FILES_DIR);

    string path = SERVER_FILES_DIR + "/" + filename;
    ofstream file(path, ios::binary);

    if (!file.is_open())
        return "Upload failed";

    file.write(content.c_str(), (streamsize)content.size());
    file.close();

    return "Upload successful";
}

string handleDeleteCommand(const string& filename)
{
    string path = SERVER_FILES_DIR + "/" + filename;

    if (!fs::exists(path))
        return "File not found";

    if (fs::remove(path))
        return "File deleted successfully";

    return "Delete failed";
}

string handleSearchCommand(const string& keyword)
{
    if (!fs::exists(SERVER_FILES_DIR))
        fs::create_directory(SERVER_FILES_DIR);

    string result;
    for (const auto& entry : fs::directory_iterator(SERVER_FILES_DIR))
    {
        string fname = entry.path().filename().string();
        if (fname.find(keyword) != string::npos)
            result += fname + "\n";
    }

    if (result.empty())
        return "No matching files found";

    return result;
}

string handleInfoCommand(const string& filename)
{
    string path = SERVER_FILES_DIR + "/" + filename;

    if (!fs::exists(path))
        return "File not found";

    auto size = fs::file_size(path);
    auto lastWrite = fs::last_write_time(path);

    stringstream ss;
    ss << "Filename: " << filename << "\n";
    ss << "Size: " << size << " bytes\n";
    ss << "Exists: yes\n";
    ss << "Last modified: available";

    return ss.str();
}

int getActiveClientsCount()
{
    int count = 0;
    lock_guard<mutex> lock(clientsMutex);
    for (const auto& c : clients)
    {
        if (c.socket != INVALID_SOCKET)
            count++;
    }
    return count;
}

// ---------------- CLIENT HANDLER ----------------
void handleClient(client_type client)
{
    char buffer[4096]; // NDRYSHUAR: buffer më i madh

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client.socket, buffer, sizeof(buffer), 0);

        if (bytes <= 0)
        {
            cout << "Client " << client.id << " disconnected.\n";

            lock_guard<mutex> lock(clientsMutex);
            closesocket(client.socket);
            clients[client.id].socket = INVALID_SOCKET;
            clients[client.id].role = "";
            break;
        }

        {
            lock_guard<mutex> lock(clientsMutex);
            clients[client.id].lastActivity = time(0);
        }

        string input(buffer, bytes);
        string response;

        // role parsing
        if (input.rfind("/role ", 0) == 0)
        {
            string roleValue = input.substr(6);

            {
                lock_guard<mutex> lock(clientsMutex);
                if (roleValue == "admin")
                    clients[client.id].role = "admin";
                else
                    clients[client.id].role = "read-only";
            }

            sendResponse(client.socket, "Role accepted");
            continue;
        }

        string clientRole;
        {
            lock_guard<mutex> lock(clientsMutex);
            clientRole = clients[client.id].role.empty() ? "read-only" : clients[client.id].role;
        }

        // permission control
        if (isAdminCommand(input))
        {
            if (clientRole != "admin")
            {
                if (!isReadAllowedCommand(input))
                {
                    sendResponse(client.socket, "Permission denied");
                    continue;
                }
            }
        }

        // command handling
        if (input == "/list")
        {
            response = handleListCommand();
            sendResponse(client.socket, response);
            continue;
        }
        else if (input.rfind("/read ", 0) == 0)
        {
            string filename = input.substr(6);
            response = handleReadCommand(filename);
            sendResponse(client.socket, response);
            continue;
        }
        else if (input.rfind("/download ", 0) == 0)
        {
            string filename = input.substr(10);
            response = handleDownloadCommand(filename);
            sendResponse(client.socket, response);
            continue;
        }
        else if (input.rfind("/delete ", 0) == 0)
        {
            string filename = input.substr(8);
            response = handleDeleteCommand(filename);
            sendResponse(client.socket, response);
            continue;
        }
        else if (input.rfind("/search ", 0) == 0)
        {
            string keyword = input.substr(8);
            response = handleSearchCommand(keyword);
            sendResponse(client.socket, response);
            continue;
        }
        else if (input.rfind("/info ", 0) == 0)
        {
            string filename = input.substr(6);
            response = handleInfoCommand(filename);
            sendResponse(client.socket, response);
            continue;
        }
        else if (input.rfind("/upload ", 0) == 0)
        {
            size_t newlinePos = input.find('\n');

            if (newlinePos == string::npos)
            {
                sendResponse(client.socket, "Invalid upload format");
                continue;
            }

            string firstLine = input.substr(0, newlinePos);
            string filename = firstLine.substr(8);
            string content = input.substr(newlinePos + 1);

            response = handleUploadCommand(filename, content);
            sendResponse(client.socket, response);
            continue;
        }

        // NDRYSHUAR: mesazh normal
        string msg = "Client #" + to_string(client.id) + ": " + input;
        cout << msg << endl;
        logMessage(msg);

        {
            lock_guard<mutex> lock(clientsMutex);
            messageCount++;
        }

        broadcast(msg, client.id);
        sendResponse(client.socket, "Message received");
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
                    c.role = "";
                }
            }
        }
    }
}

// ---------------- HTTP SERVER ----------------
void httpServer()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HTTP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);

    cout << "HTTP Server running on port " << HTTP_PORT << endl;

    while (true)
    {
        SOCKET clientSock = accept(sock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) continue;

        vector<string> ipsCopy;
        vector<string> logsCopy;
        int activeClients;
        int msgCountCopy;

        {
            lock_guard<mutex> lock(clientsMutex);

            activeClients = 0;
            for (const auto& c : clients)
            {
                if (c.socket != INVALID_SOCKET)
                {
                    activeClients++;
                    ipsCopy.push_back(c.ip.empty() ? "unknown" : c.ip);
                }
            }

            logsCopy = messageLog;
            msgCountCopy = messageCount;
        }

        string json =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{";

        json += "\"active_clients\":" + to_string(activeClients) + ",";
        json += "\"ip_addresses\":[";

        for (size_t i = 0; i < ipsCopy.size(); i++)
        {
            json += "\"" + escapeJson(ipsCopy[i]) + "\"";
            if (i + 1 < ipsCopy.size()) json += ",";
        }

        json += "],";
        json += "\"messages_count\":" + to_string(msgCountCopy) + ",";
        json += "\"messages\":[";

        for (size_t i = 0; i < logsCopy.size(); i++)
        {
            json += "\"" + escapeJson(logsCopy[i]) + "\"";
            if (i + 1 < logsCopy.size()) json += ",";
        }

        json += "]}";

        send(clientSock, json.c_str(), (int)json.size(), 0);
        closesocket(clientSock);
    }
}

// ---------------- MAIN SERVER ----------------
int main()
{
    if (!fs::exists(SERVER_FILES_DIR))
        fs::create_directory(SERVER_FILES_DIR);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo hints {}, * server = NULL;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, TCP_PORT, &hints, &server);

    SOCKET serverSocket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    bind(serverSocket, server->ai_addr, (int)server->ai_addrlen);
    listen(serverSocket, SOMAXCONN);

    cout << "TCP Server running on port " << TCP_PORT << endl;
    cout << "Server files directory: " << SERVER_FILES_DIR << endl;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].id = i;
        clients[i].socket = INVALID_SOCKET;
        clients[i].lastActivity = time(0);
        clients[i].ip = "";
        clients[i].role = "";
    }

    thread(timeoutChecker).detach();
    thread(httpServer).detach();

    while (true)
    {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);

        SOCKET incoming = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
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
            send(incoming, msg.c_str(), (int)msg.size(), 0);
            closesocket(incoming);
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

        {
            lock_guard<mutex> lock(clientsMutex);
            clients[id].socket = incoming;
            clients[id].lastActivity = time(0);
            clients[id].ip = ipStr;
            clients[id].role = "";
        }

        string welcome = "Connected. Your client ID is " + to_string(id);
        send(incoming, welcome.c_str(), (int)welcome.size(), 0);

        thread(handleClient, clients[id]).detach();
    }

    WSACleanup();
    return 0;
}