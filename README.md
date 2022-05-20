# Passive-Active-File-Share
Univercity Project showcasing sockets and multy threading

The objective of this exercise was to create a system where a client can communicate with a server and either **actively** or **passively** send files of a specific file type (.pdf/.doc/.xlsx) to each dedicated server.

# Active Share
On **active share** the client must send directly the file to each one of the servers.

# Passive Share
On **passive share** the client only sends the file once to a single server and then the rest of the sharing is handled by the servers themselves.

# The Approach
There is a master server called **SecretaryServer** and to start with the client only knows its port and address and the server knows the port and adress of each server, its job is to communicate with both the client and the servers and establish a connection between them

In the **active share** the client talks with the **SecretaryServer** and the server provides the client with the credentials of all the servers that accept his file type so that the client can then send the files to each server

In the **passive share** the client sends the file only once to the **SecretaryServer** and then the servers handle the rest of the transfers
