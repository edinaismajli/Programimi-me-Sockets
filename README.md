#  TCP Client-Server System (C++)

##  Përshkrimi
Ky projekt implementon një sistem **Client-Server me TCP në C++**, i cili mbështet shumë klientë, role të ndryshme dhe menaxhim të file-ve në server.

Realizuar sipas kërkesave të projektit.

---

##  Teknologjitë
- C++
- Winsock2
- Multithreading
- Filesystem
- Visual Studio

---

##  Serveri
- IP: 127.0.0.1  
- Port: 9000  
- Max klientë: 5  
- Timeout: 60 sekonda  

- Multi-client support  
- Role-based access  
- File management  
- Logging (server_log.txt)  
- HTTP server (port 8080)  

---

## Klienti
- Lidhet me server  
- Role: admin / read-only  
- Dërgon dhe merr mesazhe  

---

##  Komandat

### Admin
/list  
/read <filename>  
/upload <filename>  
/download <filename>  
/delete <filename>  
/search <keyword>  
/info <filename>  

### Read-only
/list  
/read <filename>  
/download <filename>  
/search <keyword>  
/info <filename>  

---

##  HTTP Monitor
http://localhost:8080

---

##  Ekzekutimi (Visual Studio)
1. Create Empty C++ Project  
2. Shto Server.cpp dhe Client.cpp  
3. Run serverin (F5)  
4. Run ≥ 4 klientë  

---

## Struktura
/Project  
├── Server.cpp  
├── Client.cpp  
├── server_files/  
├── server_log.txt  
└── README.md  


## Shënim
Projekt për lëndën "Rrjetat Kompjuterike".
