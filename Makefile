start:
	clear
	gcc -o SecretaryServer SecretaryServer.c NET.c -lpthread
	gcc -o Server Server.c NET.c -lpthread
	gcc -o Client Client.c NET.c
	gnome-terminal -- ./SecretaryServer
	gnome-terminal -- ./Server 8554 excel
	gnome-terminal -- ./Server 8555 word
	gnome-terminal -- ./Server 8556 excel
	gnome-terminal -- ./Server 8524 pdf
	gnome-terminal -- ./Server 8512 word
	gnome-terminal -- ./Server 85547 pdf
	gnome-terminal -- ./Server 85559 excel
	gnome-terminal -- ./Server 85565 pdf
	gnome-terminal -- ./Client example.xlsx 3 2
	gnome-terminal -- ./Client example.pdf 1 2
	gnome-terminal -- ./Client example.doc 2 2
	gnome-terminal -- ./Client example.pdf 1 2
	gnome-terminal -- ./Client example.pdf 1 2
	gnome-terminal -- ./Client example.doc 2 2
	gnome-terminal -- ./Client example.pdf 1 1
	gnome-terminal -- ./Client example.pdf 1 2
	gnome-terminal -- ./Client example.doc 2 2
	gnome-terminal -- ./Client example.pdf 1 1
	gnome-terminal -- ./Client example.pdf 1 2
	gnome-terminal -- ./Client example.xlsx 3 2
	gnome-terminal -- ./Client example.xlsx 3 1
	gnome-terminal -- ./Client example.pdf 1 2
	gnome-terminal -- ./Client example.xlsx 3 2
	gnome-terminal -- ./Client example.pdf 1 1
