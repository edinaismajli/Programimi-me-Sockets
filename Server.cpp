#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ctime>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
namespace fs = filesystem;

// ================= CONFIG =================
const int PORT = 9000;
const int HTTP_PORT = 8080;
const int BUFFER_SIZE = 4096;
const int TIMEOUT_SECONDS = 30;
const int MAX_CONNECTIONS = 10;
const string FILE_DIR = "./server_files/";

// ================= GLOBAL DATA =================
vector<SOCKET> clients;
map<SOCKET, string> roles;
map<SOCKET, time_t> lastSeen;
map<SOCKET, string> clientIPs;
vector<string> logs;
vector<string> clientMessages;
mutex mtx;
int messageCount = 0;
bool serverRunning = true;

// ================= SEND FUNCTIONS =================
void sendWithEnd(SOCKET client, const string& msg) {
    string full = msg + "\nEND\n";
    send(client, full.c_str(), (int)full.size(), 0);
}

string receiveAll(SOCKET sock) {
    char buffer[BUFFER_SIZE];
    string result;

    while (true) {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        result.append(buffer, bytes);
        if (result.find("\nEND\n") != string::npos) break;
    }

    size_t pos = result.find("\nEND\n");
    if (pos != string::npos) result = result.substr(0, pos);
    return result;
}

// ================= FILE OPERATIONS =================
string listFiles() {
    string result;
    if (!fs::exists(FILE_DIR)) fs::create_directories(FILE_DIR);

    for (auto& entry : fs::directory_iterator(FILE_DIR)) {
        result += entry.path().filename().string() + "\n";
    }
    return result.empty() ? "No files in directory" : result;
}

string readFileContent(const string& filename) {
    string path = FILE_DIR + filename;
    ifstream file(path);
    if (!file) return "File not found: " + filename;

    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

string writeFileContent(const string& filename, const string& content) {
    string clean = fs::path(filename).filename().string();
    string path = FILE_DIR + clean;

    ofstream file(path, ios::binary);
    if (!file) return "Error writing file";

    file << content;
    file.close();
    return "File uploaded successfully: " + clean;
}

string deleteFileContent(const string& filename) {
    string path = FILE_DIR + filename;
    if (fs::remove(path)) return "File deleted: " + filename;
    return "Delete failed: " + filename;
}

string searchFiles(const string& keyword) {
    string result;
    if (!fs::exists(FILE_DIR)) return "No files found";

    for (auto& entry : fs::directory_iterator(FILE_DIR)) {
        string fname = entry.path().filename().string();
        if (fname.find(keyword) != string::npos) {
            result += fname + "\n";
        }
    }
    return result.empty() ? "No files matching '" + keyword + "'" : result;
}

string getFileInfo(const string& filename) {
    string path = FILE_DIR + filename;

    if (!fs::exists(path)) return "File not found: " + filename;

    uintmax_t size = fs::file_size(path);

    stringstream ss;
    ss << "Filename: " << filename << "\n";
    ss << "Size: " << size << " bytes\n";
    ss << "Path: " << path << "\n";

    return ss.str();
}

// ================= CLIENT HANDLER =================
void handleClient(SOCKET client) {
    while (serverRunning) {
        // Update last seen time
        {
            lock_guard<mutex> lock(mtx);
            lastSeen[client] = time(nullptr);
        }

        string request = receiveAll(client);

        if (request.empty()) {
            break;
        }

        // Log the message and print to server console
        {
            lock_guard<mutex> lock(mtx);
            logs.push_back(request);
            clientMessages.push_back("From " + clientIPs[client] + ": " + request);
            messageCount++;

            // Print to server console
            cout << "[" << clientIPs[client] << "] Command: " << request << endl;
        }

        // Handle role assignment
        if (request.rfind("/role ", 0) == 0) {
            lock_guard<mutex> lock(mtx);
            roles[client] = request.substr(6);
            string role = roles[client];
            sendWithEnd(client, "Role set to: " + role +
                (role == "admin" ? " (Full access - faster responses)" : " (Read only)"));
            continue;
        }

        // Get client role
        string role;
        {
            lock_guard<mutex> lock(mtx);
            role = roles[client];
        }

        // Priority for admin - faster response
        if (role == "admin") {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
        else {
            this_thread::sleep_for(chrono::milliseconds(200));
        }

        // Handle commands
        if (request == "/list") {
            sendWithEnd(client, listFiles());
        }
        else if (request.rfind("/read ", 0) == 0) {
            string filename = request.substr(6);
            sendWithEnd(client, readFileContent(filename));
        }
        else if (request.rfind("/upload ", 0) == 0) {
            size_t pos = request.find('\n');
            if (pos == string::npos) {
                sendWithEnd(client, "Invalid upload format");
                continue;
            }
            string filename = request.substr(8, pos - 8);
            string content = request.substr(pos + 1);
            sendWithEnd(client, writeFileContent(filename, content));
        }
        else if (request.rfind("/download ", 0) == 0) {
            string filename = request.substr(10);
            sendWithEnd(client, readFileContent(filename));
        }
        else if (request.rfind("/delete ", 0) == 0) {
            if (role != "admin") {
                sendWithEnd(client, "Permission denied. Admin only.");
                continue;
            }
            string filename = request.substr(8);
            sendWithEnd(client, deleteFileContent(filename));
        }
        else if (request.rfind("/search ", 0) == 0) {
            if (role != "admin") {
                sendWithEnd(client, "Permission denied. Admin only.");
                continue;
            }
            string keyword = request.substr(8);
            sendWithEnd(client, searchFiles(keyword));
        }
        else if (request.rfind("/info ", 0) == 0) {
            if (role != "admin") {
                sendWithEnd(client, "Permission denied. Admin only.");
                continue;
            }
            string filename = request.substr(6);
            sendWithEnd(client, getFileInfo(filename));
        }
        else {
            sendWithEnd(client, "Unknown command. Available: /list, /read, /upload, /download, /delete (admin), /search (admin), /info (admin)");
        }
    }

    // Cleanup on disconnect
    {
        lock_guard<mutex> lock(mtx);
        cout << "[DISCONNECT] Client " << clientIPs[client] << " (Role: " << roles[client] << ") disconnected\n";
        clients.erase(remove(clients.begin(), clients.end(), client), clients.end());
        roles.erase(client);
        lastSeen.erase(client);
        clientIPs.erase(client);
    }

    closesocket(client);
}

// ================= TIMEOUT CHECKER =================
void timeoutChecker() {
    while (serverRunning) {
        this_thread::sleep_for(chrono::seconds(10));

        lock_guard<mutex> lock(mtx);
        time_t now = time(nullptr);

        for (auto it = clients.begin(); it != clients.end();) {
            if (now - lastSeen[*it] > TIMEOUT_SECONDS) {
                cout << "[TIMEOUT] Closing inactive connection from: " << clientIPs[*it] << "\n";
                closesocket(*it);
                roles.erase(*it);
                lastSeen.erase(*it);
                clientIPs.erase(*it);
                it = clients.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

// ================= HTTP SERVER FOR MONITORING =================
void httpServer() {
    SOCKET httpSock = socket(AF_INET, SOCK_STREAM, 0);
    if (httpSock == INVALID_SOCKET) {
        cout << "HTTP socket creation failed\n";
        return;
    }

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(HTTP_PORT);
    hint.sin_addr.s_addr = INADDR_ANY;

    if (bind(httpSock, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cout << "HTTP bind failed on port " << HTTP_PORT << "\n";
        closesocket(httpSock);
        return;
    }

    listen(httpSock, 5);
    cout << "HTTP Server running on port " << HTTP_PORT << "\n";
    cout << "Access /stats for server statistics\n";

    while (serverRunning) {
        SOCKET client = accept(httpSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        char buffer[BUFFER_SIZE];
        int bytes = recv(client, buffer, BUFFER_SIZE - 1, 0);

        if (bytes > 0) {
            string request(buffer, bytes);

            // Handle /stats endpoint
            if (request.find("GET /stats") != string::npos || request.find("GET /stats ") != string::npos) {
                lock_guard<mutex> lock(mtx);

                // Build JSON response
                stringstream json;
                json << "{\n";
                json << "  \"status\": \"ok\",\n";
                json << "  \"active_connections\": " << clients.size() << ",\n";
                json << "  \"max_connections\": " << MAX_CONNECTIONS << ",\n";
                json << "  \"total_messages\": " << messageCount << ",\n";
                json << "  \"clients\": [\n";

                for (size_t i = 0; i < clients.size(); i++) {
                    json << "    {\n";
                    json << "      \"socket\": " << clients[i] << ",\n";
                    json << "      \"ip\": \"" << clientIPs[clients[i]] << "\",\n";
                    json << "      \"role\": \"" << roles[clients[i]] << "\"\n";
                    json << "    }";
                    if (i < clients.size() - 1) json << ",";
                    json << "\n";
                }

                json << "  ],\n";
                json << "  \"recent_messages\": [\n";

                int start = max(0, (int)clientMessages.size() - 10);
                for (int i = start; i < (int)clientMessages.size(); i++) {
                    json << "    \"" << clientMessages[i] << "\"";
                    if (i < (int)clientMessages.size() - 1) json << ",";
                    json << "\n";
                }

                json << "  ]\n";
                json << "}\n";

                string response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: application/json\r\n";
                response += "Access-Control-Allow-Origin: *\r\n";
                response += "Content-Length: " + to_string(json.str().length()) + "\r\n";
                response += "\r\n";
                response += json.str();

                send(client, response.c_str(), (int)response.length(), 0);
            }
            else {
                string response = "HTTP/1.1 404 Not Found\r\n";
                response += "Content-Type: text/plain\r\n";
                response += "\r\n";
                response += "Available endpoints:\n";
                response += "GET /stats - Server statistics\n";
                send(client, response.c_str(), (int)response.length(), 0);
            }
        }

        closesocket(client);
    }

    closesocket(httpSock);
}

// ================= MAIN SERVER =================
int main() {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        cout << "WSAStartup failed\n";
        return 1;
    }

    // Create file directory if not exists
    fs::create_directories(FILE_DIR);

    // Create main socket
    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        cout << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    hint.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cout << "Bind failed on port " << PORT << "\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    listen(server, SOMAXCONN);

    cout << "========================================\n";
    cout << "FILE SERVER RUNNING\n";
    cout << "========================================\n";
    cout << "Main Socket Port: " << PORT << "\n";
    cout << "HTTP Monitor Port: " << HTTP_PORT << "\n";
    cout << "Max Connections: " << MAX_CONNECTIONS << "\n";
    cout << "Timeout: " << TIMEOUT_SECONDS << " seconds\n";
    cout << "File Directory: " << FILE_DIR << "\n";
    cout << "========================================\n\n";

    // Start timeout checker thread
    thread timeoutThread(timeoutChecker);
    timeoutThread.detach();

    // Start HTTP server thread
    thread httpThread(httpServer);
    httpThread.detach();

    // Main accept loop
    while (serverRunning) {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET client = accept(server, (sockaddr*)&clientAddr, &addrLen);

        if (client == INVALID_SOCKET) {
            continue;
        }

        // Get client IP
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

        // Check connection limit
        {
            lock_guard<mutex> lock(mtx);
            if (clients.size() >= MAX_CONNECTIONS) {
                cout << "Connection limit reached. Rejecting client from: " << ipStr << "\n";
                string msg = "Server busy. Max connections (" + to_string(MAX_CONNECTIONS) + ") reached. Try later.\nEND\n";
                send(client, msg.c_str(), (int)msg.size(), 0);
                closesocket(client);
                continue;
            }

            clients.push_back(client);
            clientIPs[client] = ipStr;
            lastSeen[client] = time(nullptr);
            cout << "[CONNECT] New client connected from: " << ipStr << " (Total: " << clients.size() << ")\n";
        }

        // Start client handler thread
        thread clientThread(handleClient, client);
        clientThread.detach();
    }

    closesocket(server);
    WSACleanup();
    return 0;
}