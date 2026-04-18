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
    cerr<<"Cant create Socket,Err#"<<WSAGetlastError()<<endl;
    WSACleanup();
    return 1;
 }
    



}