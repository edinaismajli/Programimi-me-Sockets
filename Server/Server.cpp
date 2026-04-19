#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define DEFAULT_PORT "8080"
#define MAX_CLIENTS 5

struct client_type
{
    int id;
    SOCKET socket;
};

// Placeholder for client processing function
int process_client(client_type& new_client, vector<client_type>& client_array, thread& t)
{
    return 0;
}

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Setup addrinfo
    struct addrinfo hints;
    struct addrinfo* server = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &server);

    cout << "Creating server socket..." << endl;
    SOCKET server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    if (server_socket == INVALID_SOCKET)
    {
        cerr << "Error: Failed to create socket. Code: "
             << WSAGetLastError() << endl;

        WSACleanup();
        return -1;
    }
    else
    {
        cout << "Socket created successfully! ID: "
             << server_socket << endl;
    }

    // Bind
    cout << "Binding socket to port " << DEFAULT_PORT << "..." << endl;

    int bindResult = bind(server_socket, server->ai_addr, (int)server->ai_addrlen);

    if (bindResult == SOCKET_ERROR)
    {
        cerr << "Error: Bind failed. Code: "
             << WSAGetLastError() << endl;

        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    cout << "Bind successful!" << endl;

    // Listen on port
    cout << "Starting to listen on port " << DEFAULT_PORT << "..." << endl;

    int listenResult = listen(server_socket, SOMAXCONN);

    if (listenResult == SOCKET_ERROR)
    {
        cerr << "Error: Listen failed. Code: "
             << WSAGetLastError() << endl;

        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    cout << "Server is now listening!" << endl;

    vector<client_type> client(MAX_CLIENTS);
    thread my_thread[MAX_CLIENTS];
    int temp_id = 0;

    while (true)
    {
        SOCKET incoming = accept(server_socket, NULL, NULL);

        if (incoming != INVALID_SOCKET)
        {
            // Limit clients
            if (temp_id >= MAX_CLIENTS)
            {
                cout << "Server full! Rejecting client..." << endl;

                string msg = "Server is full";
                send(incoming, msg.c_str(), msg.size(), 0);

                closesocket(incoming);
                continue;
            }

            cout << "Client connected! ID: " << temp_id << endl;

            client[temp_id] = { temp_id, incoming };

            string id = to_string(temp_id);
            send(incoming, id.c_str(), id.size(), 0);

            my_thread[temp_id] = thread(
                process_client,
                ref(client[temp_id]),
                ref(client),
                ref(my_thread[temp_id])
            );

            temp_id++;
        }
    }

    WSACleanup();
    return 0;
}