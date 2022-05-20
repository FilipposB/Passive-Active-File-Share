FILIPPOS BAGORDAKIS

Date : 27/12/2020

Exercise : passive active application

OS : Ubuntu 20.04.1 LTS

--HOW TO RUN--
There is a Make file and a sample of each file type by typing "make" in the terminal it should run as long as "gnome-terminal --"
is supported by your system.

In case the Make file doesn't work:


--Compile--

####All servers are multi threaded and have mutexes so -lpthread is required####

gcc -o Server.c NET.c -lpthread
gcc -o Client.c NET.c
gcc -o SecretaryServer.c NET.c -lpthread


--Run--

./SecretaryServer -(1)#FILE_NAME#- -(2)#FILE_TYPE#(1 : pdf| 2 : word| 3 : excel)- -(3)#UPLOAD_METHOD#(1 active| 2 passive)


 -SecretaryServer.c-
 This is the main server, it has to keep up a connection with all the active servers, all servers register here.
Clients can ask this server where to send their files and the servers can ask it where to send their file in case of a passive upload
No files pass though here only information about where the files should go.


 -NET.c-
 This custom library I built includes a few convenient network functions, like read/send_message, send/receive_file etc


--Important--
For ease of use SecretaryServer address and port have been added to the NET.h
There isn't any problem implementing them in the future as arguments if required.

All the included example files (pdf, word, excel)have been downloaded from this website -> https://file-examples.com/
