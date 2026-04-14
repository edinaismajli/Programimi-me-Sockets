#include <iostream>
#include<string>
#include<winsock2.h>
#include<ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") 

using namespace std;

const int PORT = 8080; 
const int BUFFER_SIZE = 4096;

int main(){
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket == INVALID_SOCKET){
        cout<<"Gabim ne krijimin e socketit!"<<endl;
        return 1;
    }

    string serverIP;
    cout<<"Shkruani IP e serverit: ";
    getline(cin, serverIP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;   
    serverAddr.sin_port = htons(PORT);
    
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    if(serverAddr.sin_addr.s_addr == INADDR_NONE){
        cout<<"IP e serverit eshte e pavlefshme!"<<endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    
    if(connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
        cout<<"Lidhja me serverin dështoi!"<<endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout<<"Lidhja me serverin u realizua me sukses!"<<endl;


    //merr ide klientit nga serveri
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

    string clientID= "";
    if(bytesReceived> 0){
        clientID = string(buffer,0, bytesReceived);
        cout<<"ID e klientit: "<<clientID<<endl;
    }else{
        cout<<"Gabim ne marrjen e ID se klientit!"<<endl;
    }



while (true) {
    cout << "\n>";
    getline(cin,message);

    if (message ==" /quit") {
        cout << "Po dilni nga klienti...\n";
        break;
    }

    send(clientSocket, message.c_str(), message.size(), 0);

    ZeroMemory(buffer, BUFFER_SIZE);
    int bytesRecived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

    if(bytesRecived >0) {
        cout << "Server:" << string(buffer, 0, bytesRecived) << endl;
    } else {
        cout<< "Serveri nuk ktheu pergjigjje ose linja u mbyll.\n";
        break;
    }
}
  





       
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    
        

}
