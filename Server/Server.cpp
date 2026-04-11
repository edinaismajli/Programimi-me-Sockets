SOCKET incoming = accept(server_socket, NULL, NULL);

if (incoming != INVALID_SOCKET)
{
    client[temp_id] = { temp_id, incoming };
}