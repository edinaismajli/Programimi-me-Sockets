int main()
{
    WSADATA wsaData;
    struct addrinfo hints;
    struct addrinfo* server = NULL;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;
    
    

    SOCKET incoming = accept(server_socket, NULL, NULL);

    if (incoming != INVALID_SOCKET)
    {
        client[temp_id] = { temp_id, incoming };
        
        std::string id = std::to_string(temp_id);
        send(incoming, id.c_str(), id.size(), 0);

        my_thread[temp_id] = std::thread(
            process_client,
            std::ref(client[temp_id]),
            std::ref(client),
            std::ref(my_thread[temp_id])
        );
    }
}