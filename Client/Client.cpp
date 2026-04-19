#include <iostream>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <string>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const string SERVER_IP="127.0.0.1";
const int SERVER_PORT=9000;
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

bool receiveText(SOCKET sock,string& response){
    char buf[BUFFER_SIZE];
    ZeroMemory(buf,BUFFER_SIZE);

    int bytesReceived =recv(sock,buf,BUFFER_SIZE,0);

    if(bytesReceived>0){
        response=string(buf,0,bytesReceived);
        return true;
    }
    else if(bytesReceived==0){
        response="Serveri e mbylli lidhjen.";
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
   string request = "/upload " + filename + "\n" + content;

    if(!sendText(sock,request)){
        cerr<<"Gabim gjate upload-it,Err #"<<WSAGetLastError()<<endl;
        return false;
    }
    return true;
}

bool saveDownloadedFile(const string& filename,const string& content){
    ofstream outFile(filename,ios::binary);
    if(!outFile.is_open())
    return false;

    outFile.write(content.c_str(),content.size());
    outFile.close();
    return true;
}


int main(){

   string ipAdress= SERVER_IP;
   int port=SERVER_PORT;

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

cout<<"Zgjedh rolin e klientit (admin/user): ";
cout<<"1. Admin\n";
cout<<"2. read-only user\n";
cout<<"Zgjedhja: ";

int choice;
cin>>choice;
cin.ignore();

string role;
if(choice==1)
    role="admin";
    else
    role="read-only";

    DWORD timeout;
    if(role=="admin")
        timeout=3000;
    else
        timeout=7000;

    
setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));


 sockaddr_in hint={};
   hint.sin_family=AF_INET;
   hint.sin_port=htons(port);

hint.sin_addr.S_un.S_addr = inet_addr(ipAdress.c_str());

if (hint.sin_addr.S_un.S_addr == INADDR_NONE)
{
    cerr << "IP adresa nuk eshte valide!\n";
    closesocket(sock);
    WSACleanup();
    return 1;
}

  int connResult=connect(sock,(sockaddr*)&hint,sizeof(hint));
  if(connResult==SOCKET_ERROR){

      cerr<<"Can't connect to server,Err#"<<WSAGetLastError()<<endl;
      closesocket(sock);
      WSACleanup();
      return 1;
   }
 
   cout<< "Connected to server:" << ipAdress<< ":" << port << endl;
   string introRole = "/role " + role;
sendText(sock, introRole);

string introResponse;
if (receiveText(sock, introResponse))
{
    cout << "SERVER> " << introResponse << endl;
}
   printMenu(role);








   string userInput;

   while (true)
   {
    cout<< ">";
    getline(cin,userInput);

    if(userInput.empty()){
        cout<< "Shkruaj nje messazh ose komandë!\n";
        continue;
    }

    if(userInput=="/exit"){
       cout << "Klienti po mbyllet...\n";
        break;;
    }
    if(userInput=="/help"){
         printMenu(role);
        continue;
   }

   if (role!="admin" && isAdminCommand(userInput)){
        cout<<"Gabim: ky klient ka vetem read() permission.\n";
        continue;
   }

   
   if (role == "admin" && userInput.rfind("/upload ", 0) == 0)
        {
            string filename = userInput.substr(8);

            if (filename.empty())
            {
                cout << "Perdorimi: /upload <filename>\n";
                continue;
            }

            if (!uploadFile(sock, filename))
                continue;

            string response;
            if (receiveText(sock, response))
                cout << "SERVER> " << response << endl;
            else
                cout << response << endl;

            continue;
        }

        if(!sendText(sock,userInput)){
            cerr<<"Send failed,Err #"<<WSAGetLastError()<<endl;
            break;}

        string response;
        if(!receiveText(sock,response)){  
            cout<<response<<endl;
            break;
        }

        if (role == "admin" && userInput.rfind("/download ", 0) == 0)
        {
            string filename = userInput.substr(10);

            if (!filename.empty())
            {
                if (response == "File not found" || response == "Permission denied")
{
    cout << "SERVER> " << response << endl;
}
else
{
    if (saveDownloadedFile(filename, response))
        cout << "File u ruajt lokalish si: " << filename << endl;
    else
        cout << "Nuk u ruajt file-i. Permbajtja:\n" << response << endl;
}
            }
            else
            {
                cout << "Perdorimi: /download <filename>\n";
            }
        }
        else
        {
            cout << "SERVER> " << response << endl;
        }
    }

    
closesocket(sock);
    WSACleanup();
    return 0;
}


  