# File Server System using TCP (C++)

## 1. Hyrje

Ky projekt paraqet implementimin e një sistemi **klient-server** duke përdorur protokollin **TCP** në gjuhën programuese C++. Qëllimi kryesor është të demonstrohen konceptet bazë të komunikimit në rrjet, menaxhimit të shumë klientëve dhe kontrollit të qasjes përmes roleve të ndryshme.

Sistemi përbëhet nga një server qendror dhe disa klientë që lidhen me të për të kryer operacione mbi file-at në server.

---

## 2. Arkitektura e Sistemit

Sistemi ndjek modelin klasik **Client-Server**, ku:

- Serveri është përgjegjës për:
  - pranimin e lidhjeve hyrëse
  - përpunimin e kërkesave
  - menaxhimin e file-ve
- Klientët dërgojnë komanda dhe marrin përgjigje nga serveri

Përveç socket-it kryesor TCP, serveri implementon edhe një **HTTP server të thjeshtë** për monitorim.

---

## 3. Teknologjitë e Përdorura

- Gjuha programuese: C++
- Biblioteka për rrjet: Winsock2
- Multithreading: `std::thread`
- Menaxhimi i file-ve: `<filesystem>`
- HTTP server bazik

---

## 4. Funksionalitetet

### 4.1 Menaxhimi i Klientëve
- Serveri mbështet lidhje të shumëfishta njëkohësisht
- Çdo klient trajtohet në një thread të veçantë
- Ekziston kufizim për numrin maksimal të klientëve

### 4.2 Role të Përdoruesve
- **Admin**
  - Qasje e plotë (read, write, delete, search, info)
  - Kohë përgjigjeje më e shpejtë
- **User**
  - Qasje e kufizuar (read, download)

### 4.3 Operacionet mbi File
- Listimi i file-ve në server
- Leximi i përmbajtjes së file-ve
- Ngarkimi (upload) i file-ve në server
- Shkarkimi (download) i file-ve
- Fshirja e file-ve (vetëm admin)
- Kërkimi sipas emrit të file-ve
- Shfaqja e informacionit për file

### 4.4 Menaxhimi i Lidhjeve
- Timeout për klientët joaktiv
- Mbyllje automatike e lidhjeve të papërdorura
- Kufizim i lidhjeve të reja kur arrihet maksimumi

### 4.5 Monitorimi përmes HTTP
- Serveri ofron endpoint:
  
  ```
  GET /stats
  ```

- Kthen:
  - numrin e lidhjeve aktive
  - IP adresat e klientëve
  - numrin total të mesazheve
  - mesazhet e fundit

---

## 5. Komandat e Mbështetura

### Admin

```
/list
/read <filename>
/upload <filename>
/download <filename>
/delete <filename>
/search <keyword>
/info <filename>
/exit
```

### User

```
/list
/read <filename>
/download <filename>
/exit
```

---

## 6. Ekzekutimi i Projektit

### 6.1 Kompilimi

```bash
g++ Server.cpp -o server -lws2_32
g++ Client.cpp -o client -lws2_32
```

### 6.2 Nisja

```bash
./server
./client
```

---

## 7. Konfigurimi

Parametrat kryesorë:

- Porti TCP: `9000`
- Porti HTTP: `8080`
- IP e serverit: `127.0.0.1`
- Numri maksimal i klientëve: `10`
- Timeout: `30 sekonda`

---


## 8. Përfundim

Ky projekt demonstron në mënyrë praktike:

- Komunikimin përmes TCP sockets
- Arkitekturën klient-server
- Menaxhimin e threads
- Kontrollin e qasjes përmes roleve
- Implementimin e një HTTP serveri bazik
