int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

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