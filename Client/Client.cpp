#include <iostream>
#include <Ws2tcpip.h>
#include <String>
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

 
 


}