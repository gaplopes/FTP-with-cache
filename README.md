# FTP-with-cache

This is a client-server file transfer protocol, which allows the client to perform basic operations such as transferring a file to the server, listing the files on the server and getting a file from the server.
Moreover, a cache is also placed between the client and the file server to reduce the load on the server.
This cache will respond to requests from clients instead of the server if it has the requested files.

The project is a part of the course "Introduction to Communication Networks" at the University of Coimbra for the 2015/2016 academic year for the Informatics Engineering degree.

_Note:_ The project statement is in Portuguese and can be found in the pdf file "Project Statement.pdf".

--------------------

The following instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

## Run the client

To compile the client, run the following command in the `Cliente` directory:

```
gcc cliente.c –lm –o cliente
```

To run the client, you need to run the following command:

```
./cliente <host> <username> <password>
```

Note that the `<host>` is the IP address of the server, `<username>` is the username of the user and `<password>` is the password of the user.

The client will then prompt you for a command. The available commands are:

```
1 - List Files
2 - Upload File
3 - Download File
4 - Delete File
0 - Logout
```

## Run the server

To compile the server, run the following command in the `Servidor` directory:

```
gcc servidor.c –lm –o servidor
```

To run the server, you need to run the following command:

```
./servidor
```

## Run the cache

To compile the cache, run the following command in the `Cache` directory:

```
gcc cache.c –lm –o cache
```

To run the cache, you need to run the following command:

```
./cache <host>
```

--------------------
## Authors

* Gonçalo Lopes
* Paulo Cruz