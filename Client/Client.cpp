#include <iostream>
#include<ws2tcpip.h>
#include <Ws2tcpip.h>
#include <string>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const string SERVER_IP="127.0.0.1";
const int SERVER_PORT=8080;
const int BUFFER_SIZE=4096;


bool isAdminCommand(const string& command){
    return command == "/list" ||
           command.rfind("/read ", 0) == 0 ||
           command.rfind("/upload ", 0) == 0 ||
           command.rfind("/download ", 0) == 0 ||
           command.rfind("/delete ", 0) == 0 ||
           command.rfind("/search ", 0) == 0 ||
           command.rfind("/info ", 0) == 0;
}

void printMenu(const string& role){
     cout << "\n========== MENU ==========\n";
    cout << "Roli: " << role << endl;
    cout << "/help                    - shfaq komandat\n";
    cout << "/exit                    - mbyll klientin\n";
    cout << "Mesazh normal            - dergo tekst te serveri\n";

  if(role == "admin"){
     cout << "/list                    - liston file-at ne server\n";
        cout << "/read <filename>         - lexon file nga serveri\n";
        cout << "/upload <filename>       - dergon file ne server\n";
        cout << "/download <filename>     - shkarkon file nga serveri\n";
        cout << "/delete <filename>       - fshin file ne server\n";
        cout << "/search <keyword>        - kerkon file ne server\n";
        cout << "/info <filename>         - merr info per file\n";
  }
  else{
    cout<<"Ky klient ka vetem read() permission.\n";
  }
  cout<<"==================================\n\n";
}

bool sendText(SOCKET sock,const string& text){
    int sendResult=send(sock,text.c_str(),(int)text.size(),0);
    return sendResult != SOCKET_ERROR;
}

bool reciveText(SOCKER sock,string& response){
    char buf[BUFFER_SIZE];
    ZeroMemory(buf,BUFFER_SIZE);

    int bytesReceived =recv(sock,buf,BUFFER_SIZE,0);

    if(bytesRecived>0){
        response=string(buf,0,bytesReceived);
        return true;
    }
    else if(bytesReceived==0){
        response="Serveri e mbylli lidhjen."
        return false;
    }
    else{
        response ="Gabim ne recv(),Err #"+to_string(WSAGetLastError());
        return false;
    }
}

bool uploadFile(SOCKET sock,const string& filename){

    ifstream file(filename,ios::binary);
    if(!file.is_open()){
        cerr<<"Nuk u hap file-i:"<<filename<<endl;
        return false;
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    string request="/upload"+filename+"\n"+content;

    if(!sendText(sock,request)){
        cerr<<"Gabim gjate upload-it,Err #"<<WSAGetLastError()<<endl;
        return false;
    }
    return true;
}

bool saveDownloadedFile(const string& filename,const string& content){
    ofstream outFile(filename,ios::binary);
    if(!outFile.is_open())
    return false

    outFile.write(content.c_str(),content.size());
    outFile.close();
    return true;
}


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
        int sendResult=send(sock,userInput.c_str(),(int)userInput.size()+1.0);
        if(sendResult != SOCKET_ERROR){
            ZeroMemory(buf,4096);
            int bytesReceived=recv(sock,buf,4096,0);
            if(vytesReceived>0){
                cout<<"SERVER>"<<string(buf,0,bytesReceived)<<endl;
            }
        }

        }
    }
    while(userInput.size()>0);
   }

