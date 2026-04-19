#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const string SERVER_IP = "127.0.0.1";
const int PORT = 9000;
const int BUFFER_SIZE = 4096;

// ================= RECEIVE MESSAGE =================
string receiveAll(SOCKET sock) {
    char buffer[BUFFER_SIZE];
    string result;

    while (true) {
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;

        result.append(buffer, bytes);

        if (result.find("\nEND\n") != string::npos) {
            break;
        }
    }

    size_t pos = result.find("\nEND\n");
    if (pos != string::npos) {
        result = result.substr(0, pos);
    }

    return result;
}

// ================= SEND MESSAGE =================
bool sendMessage(SOCKET sock, const string& message) {
    string msgWithEnd = message + "\nEND\n";
    int sent = send(sock, msgWithEnd.c_str(), (int)msgWithEnd.size(), 0);
    return sent != SOCKET_ERROR;
}

// ================= UPLOAD FILE =================
void uploadFile(SOCKET sock, const string& filename) {
    ifstream file(filename, ios::binary);

    if (!file.is_open()) {
        cout << "Error: File '" << filename << "' not found!\n";
        return;
    }

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    string request = "/upload " + filename + "\n" + content;
    sendMessage(sock, request);

    string response = receiveAll(sock);
    cout << response << endl;
}

// ================= DOWNLOAD FILE =================
void downloadFile(SOCKET sock, const string& filename) {
    string request = "/download " + filename;
    sendMessage(sock, request);

    string content = receiveAll(sock);

    if (content.find("File not found") != string::npos) {
        cout << content << endl;
        return;
    }

    ofstream file(filename, ios::binary);
    file << content;
    file.close();

    cout << "Downloaded: " << filename << " (" << content.size() << " bytes)\n";
}

// ================= SHOW MENU =================
void showAdminMenu() {
    cout << "\n========== ADMIN COMMANDS ==========\n";
    cout << "/list                 - List all files\n";
    cout << "/read <filename>      - Read file content\n";
    cout << "/upload <filename>    - Upload file to server\n";
    cout << "/download <filename>  - Download file from server\n";
    cout << "/delete <filename>    - Delete file from server\n";
    cout << "/search <keyword>     - Search files by keyword\n";
    cout << "/info <filename>      - Get file information\n";
    cout << "/exit                 - Disconnect\n";
    cout << "====================================\n";
}

void showUserMenu() {
    cout << "\n========== USER COMMANDS ==========\n";
    cout << "/list                 - List all files\n";
    cout << "/read <filename>      - Read file content\n";
    cout << "/download <filename>  - Download file from server\n";
    cout << "/exit                 - Disconnect\n";
    cout << "===================================\n";
}

// ================= MAIN =================
int main() {
    // Initialize Winsock
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        cout << "WSAStartup failed!\n";
        return 1;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Socket creation failed!\n";
        WSACleanup();
        return 1;
    }

    // Server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

    // Connect to server
    cout << "Connecting to server " << SERVER_IP << ":" << PORT << "...\n";
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connection failed! Make sure server is running.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server successfully!\n\n";

    // Select role
    cout << "Select your role:\n";
    cout << "1. Admin (Full access - faster responses)\n";
    cout << "2. Regular User (Read only)\n";
    cout << "Choice: ";

    int choice;
    cin >> choice;
    cin.ignore();

    string role = (choice == 1) ? "admin" : "user";

    // Send role to server
    sendMessage(sock, "/role " + role);
    string roleResponse = receiveAll(sock);
    cout << roleResponse << endl;

    // Show appropriate menu
    if (role == "admin") {
        showAdminMenu();
    }
    else {
        showUserMenu();
    }

    cout << "\nEnter commands (type /exit to quit):\n";

    // Command loop
    string command;
    while (true) {
        cout << "\n> ";
        getline(cin, command);

        if (command == "/exit") {
            cout << "Disconnecting...\n";
            break;
        }

        // Handle commands
        if (command.rfind("/upload ", 0) == 0) {
            string filename = command.substr(8);
            uploadFile(sock, filename);
        }
        else if (command.rfind("/download ", 0) == 0) {
            string filename = command.substr(10);
            downloadFile(sock, filename);
        }
        else {
            // Send command to server
            sendMessage(sock, command);
            string response = receiveAll(sock);
            cout << response << endl;
        }
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();

    cout << "Disconnected. Goodbye!\n";

    return 0;
}