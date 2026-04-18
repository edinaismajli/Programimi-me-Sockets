#include <iostream>
#include <Ws2tcpip.h>
#include <string>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;


int main(){

   string ipAdress= "127.0.0.1";
   int port=8080;

   WSADATA data;
    WORD ver = MAKEWORD(2, 2);
    int WSResult = WSAStartup(ver, &data);


    if(WSResult !=0)
    {
        cerr << "Can't start Winsock, Err #" << WSResult << endl;
        return 1;
    }

 SOCKET sock=socket(AF_INET,SOCK_STREAM,0);
 if(sock==INVALID_SOCKET){
    cerr<<"Cant create Socket,Err#"<<WSAGetLastError()<<endl;
    WSACleanup();
    return 1;
 }
    

 sockaddr_in hint={};
   hint.sin_family=AF_INET;
   hint.sin_port=htons(port);
  hint.sin_addr.S_un.S_addr=inet_addr(ipAdress.c_str());


  int connResult=connect(sock,(sockaddr*)&hint,sizeof(hint));
  if(connResult==SOCKET_ERROR){

      cerr<<"Can't connect to server,Err#"<<WSAGetLastError()<<endl;
      closesocket(sock);
      WSACleanup();
      return 1;
   }

   char buf[4096];
   string userInput;


   do{
    cout<<"Send message to server:";
    getline(cin,userInput);


    if(userInput.size()>0){
        int sendResult=send(sock,userInput.c_str(),(int)userInput.size()+1,0);
        if(sendResult != SOCKET_ERROR){
            ZeroMemory(buf,4096);
            int bytesReceived=recv(sock,buf,4096,0);
            if(bytesReceived>0){
                cout<<"SERVER>"<<string(buf,0,bytesReceived)<<endl;
            }
        }

        }
    }
    while(userInput.size()>0);


 closesocket(sock);
 WSACleanup();

    return 0;

   }

