#include<stdio.h>
#include<string> //strlen
#include<stdlib.h> //strlen
#include<sys/socket.h>
#include <bits/errno.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h> //write
#include<pthread.h> //for threading
#include<cstdlib>
#include <string.h>
#include "directories.h"
#include "createDB.h"
#include <sqlite3.h>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

void raspunde(void *);
void *treat(void *);
void commands(char msg_client_user[100], struct thread_data tdl);
#define PORT 5600
#define LENGTH 512
sqlite3 *clients_db;

typedef struct thread_data{
    int id_thread; //the id of the thread
    int client_descriptor; //the descriport returned by accept
}thread_data;
int connection(){
    //Threads declarations
    pthread_t threads[10000];
    int counter=0;
    //Socket declarations
    int socket_descriptor, client_socket;
    socklen_t client_length;
    struct sockaddr_in server, from;

    //baza de date
    int rc;
    rc = sqlite3_open("clients.db", &clients_db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(clients_db));
        exit(0);
    }else{
        fprintf(stdout, "Opened database successfully\n");
    }


    //Create socket
    ;
    if((socket_descriptor = socket(AF_INET, SOCK_STREAM,0))== -1) {
        perror("Error at creating the socket");
    }
    printf("Socket created...%d\n", socket_descriptor);

    //Using th option SO_REUSEADDR
    int on=1;
    setsockopt(socket_descriptor,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

    //Preparing the data structures
    bzero (&server, sizeof (server));
    bzero (&from, sizeof (from));

    //We prepare the sockaddr_in structure
    server.sin_family = AF_INET; //Family of sockets
    server.sin_addr.s_addr = INADDR_ANY; //Acept any address
    server.sin_port = htons (PORT); //PORT

    //Bind
    if(bind(socket_descriptor, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Bind failed!");
        return 0;
    }
    printf("Bind done.\n");

    //Listen for clients
    if(listen(socket_descriptor,2)<0){
        perror("Listen failed!");
        return 0;
    }


    //Serve the clients using threads
    while(1){
        int  client;
        client_length = sizeof(from);

        printf("[Server] Waiting at port %d...\n", PORT);

        if((client = accept (socket_descriptor, (struct sockaddr *) &from, &client_length )) <0){
            perror("Accept failed!");
            return 0;
        }
        thread_data * thread_data_pointer;
        thread_data_pointer = (struct thread_data*) malloc(sizeof(struct thread_data));
        thread_data_pointer->id_thread = counter++;
        thread_data_pointer->client_descriptor = client;

        printf("Client descriptor: %d\n",thread_data_pointer->client_descriptor);
        pthread_create(&threads[counter], NULL, &treat, thread_data_pointer );
    }

}

int main(){
    connection();
}

void *treat(void *arg){
    struct thread_data tdl;
    tdl= *((struct thread_data*) arg);
    printf ("[thread]- %d -Waiting for the message...\n", tdl.id_thread);
    pthread_detach(pthread_self());
    raspunde((struct thread_data*) arg);
    printf("\nEnd of client %d\n", tdl.id_thread);
    //Closing the connection
    close(tdl.client_descriptor);
    return NULL;
}
void raspunde(void * arg){
    char msg_client[100];
    char msg_response[200]="\n[server] Comenzi: \n1) Utilizati comanda |connect| pentru a va conecta la srv. \n2) Utilizati comanda |quit| pentru a inchide aplicatia. ";
    char msg_client_password[100];
    char msg_client_user[100];
    struct thread_data tdl;
    tdl= *((struct thread_data*) arg);
    string parola_decriptata="";
    //printf("Entered in thread -%d-\n", tdl.id_thread);

    //Trimitm mesajul la client
    if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
        printf("[thread]-%d-\n", tdl.id_thread);
        perror("Failed to write() to client!\n");
    }
    //Citim conditia
    if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
        printf("[thread]-%d-", tdl.id_thread);
        perror("Failed to read()!\n");
    }
    if(strcmp(msg_client,"quit")==0){
        printf("Userul a apelat comanda |quit|\n");
        return;
    }

    //Verificam daca clientul doreste sa se conecteze
    if(strcmp(msg_client,"connect")==0) {
        //Cerere user
        bzero(msg_response, 200);
        bzero(msg_client, 100);
        strcat(msg_response, "[server]Introduceti un username: ");
        if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
            printf("[thread]-%d-\n", tdl.id_thread);
            perror("Failed to write() to client!\n");
        }
        //Citim userul
        if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
            printf("[thread]-%d-", tdl.id_thread);
            perror("Failed to read()!\n");
        }
        //Salvam userul
        strcpy(msg_client_user,msg_client);
        bzero(msg_response, 200);
        bzero(msg_client, 100);
        //Cerere parola
        strcat(msg_response, "[server]Introduceti o parola: ");
        if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
            printf("[thread]-%d-\n", tdl.id_thread);
            perror("Failed to write() to client!\n");
        }
        //Citim parola
        if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
            printf("[thread]-%d-", tdl.id_thread);
            perror("Failed to read()!\n");
        }
        //Decriptam parola si o salvam
        parola_decriptata = encryptDecrypt(string(msg_client));
        bzero(msg_response, 200);
        bzero(msg_client, 100);
        //Cautare username in baza de date
        if (search_user(clients_db, parola_decriptata, string(msg_client_user)) == 1) {
            printf("\nUser gasit in baza de date.\n");
            //Accesare director principal + ls
            commands(msg_client_user,tdl);
            return;
        }
        //Introducem userul in baza de date
        else {
            printf("Userul nu a fost gasit in baza de date\n");
            //Cerere introducere user in baza de date
            bzero(msg_response, 200);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server]Userul nu a fost gasit in baza de date.\n Introduceti un user pentru crearea unui cont: ");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim noul user
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            //Salvam noul user
            strcpy(msg_client_user,msg_client);
            bzero(msg_response, 200);
            bzero(msg_client, 100);
            //Cerere parola noua
            strcat(msg_response, "[server]Introduceti o parola: ");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim parola noua
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            //Decriptam noua parola si o salvam
            parola_decriptata = encryptDecrypt(string(msg_client));
            bzero(msg_response, 200);
            bzero(msg_client, 100);

            //Inseram datele in baza de date
            insertData(clients_db, string(msg_client_user), parola_decriptata);
            //creare director principal
            make_directory(msg_client_user);
            //ls + comenzi
            commands(msg_client_user,tdl);
            return;
        }
    }
    return;

}

void upload(struct thread_data tdl, string f_name){
    char msg_client[100];
    char msg_response[20000];
    char revbuf[LENGTH];
    FILE *fp = fopen(f_name.c_str(), "a");
    if(fp == NULL) printf("File %s cannot be opened.\n", f_name);
    else
    {
        bzero(revbuf, LENGTH);
        ssize_t f_block_sz = 1;
        int success = 0;

        while(success == 0)
        {
            while(1)
            {
                if(strcmp(msg_client,"stop") == 0){ break;}

                f_block_sz = read(tdl.client_descriptor, revbuf, LENGTH);
                if(f_block_sz < 0)
                {
                    printf("Receive file error.\n");
                    break;
                }
                int write_sz = fwrite(revbuf, sizeof(char), f_block_sz, fp);
                if(write_sz < f_block_sz)
                {
                    printf("File write failed.\n");
                    break;
                }
                bzero(revbuf, LENGTH);

                if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                    printf("[thread]-%d-", tdl.id_thread);
                    perror("Failed to read()!\n");
                }
            }
            printf("ok!\n");
            success = 1;
            fclose(fp);
        }
    }

}

void commands(char msg_client_user[100], struct thread_data tdl){
    char msg_client[100];
    char msg_response[20000];
    char first_directory[200];
    strcat(first_directory,msg_client_user);
    string listing_string = "";
    ssize_t bytesRead = 1;
    ssize_t bytesSent;
    char buffer[20000];
    size_t bufferLength=20000;

    //Cerere comanda user
    bzero(msg_response, 20000);
    bzero(msg_client, 100);
    strcat(msg_response, "\n[server]Aveti posibilitatea urmatoarelor comenzi:\n 1) list - listarea fisierelor sau a directoarelor dintr-un director\n 2) upload - pentru upload de fisiere intr-un director\n 3) download - pentru a downloada un fisier de pe server\n 4) mkdir - pentru crearea unui director\n 5) access - pentru a accesa un director\n 6) back - pentru a ne intoarce dintr-un director\n 7) delete_d - pentru a sterge un director\n 8) copy_d - pentru a copia un directorul curent intr-un director existent\n 9) copy_f - pentru a copia un fisier\n 10) delete_f - pentru a sterge un fisier\n ");
    if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
        printf("[thread]-%d-\n", tdl.id_thread);
        perror("Failed to write() to client!\n");
    }

    while(1) {
        int delete_used=0;
        //Citim comanda de la client
        if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
            printf("[thread]-%d-", tdl.id_thread);
            perror("Failed to read()!\n");
        }
        if (strcmp(msg_client, "quit") == 0) {
            return;
        }

        if (strcmp(msg_client, "upload") == 0) {
            //Cerere nume fisier pentru upload
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server] Introduceti numele fisierului pe care doriti sa-l uploadati:");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim comanda de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }

            upload(tdl,string(msg_client));

            strcat(msg_response, "\n[server] Fisier transferat.");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
        }
        if (strcmp(msg_client, "download") == 0) {
            return;
        }
        if (strcmp(msg_client, "mkdir") == 0) {
            //Cerere nume director
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server]Introduceti numele noului director: ");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim comanda de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            strcat(msg_client_user, "/");
            strcat(msg_client_user, msg_client);

            make_directory(msg_client_user);
        }
        if (strcmp(msg_client, "access") == 0) {
            DIR *directory_pointer;

            //Cerere nume director
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server]Introduceti numele directorului de accesat: ");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim comanda de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            strcat(msg_client_user, "/");
            strcat(msg_client_user, msg_client);

            directory_pointer = opendir(msg_client_user);
            //Nu exista directorul specificat
            if(directory_pointer == NULL){
                string back_path="";
                string back_path2="";
                back_path = string(msg_client_user);
                reverse(back_path.begin(),back_path.end());
                back_path2 = back_path.substr(back_path.find("/",0)+1,back_path.length());
                reverse(back_path2.begin(),back_path2.end());
                strcpy(msg_client_user,back_path2.c_str());
                printf("\n Back to: %s \n", msg_client_user);
            }
        }
        if (strcmp(msg_client, "back") == 0) {
            string back_path="";
            string back_path2="";
            back_path = string(msg_client_user);
            reverse(back_path.begin(),back_path.end());
            back_path2 = back_path.substr(back_path.find("/",0)+1,back_path.length());
            reverse(back_path2.begin(),back_path2.end());
            strcpy(msg_client_user,back_path2.c_str());
            printf("\n Back to: %s \n", msg_client_user);
        }
        if (strcmp(msg_client, "delete_d") == 0 && strcmp(msg_client_user,first_directory)!=0) {
            delete_used=1;
            //Trimitem confirmarea stergerii directorului
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server]Director sters. ");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            delete_directory(msg_client_user);
            //Auto back
            string back_path="";
            string back_path2="";
            back_path = string(msg_client_user);
            reverse(back_path.begin(),back_path.end());
            back_path2 = back_path.substr(back_path.find("/",0)+1,back_path.length());
            reverse(back_path2.begin(),back_path2.end());
            strcpy(msg_client_user,back_path2.c_str());
            printf("\n Back to: %s \n", msg_client_user);
        }
        if (strcmp(msg_client, "copy_d") == 0) {
            //Salvam directorul curent
            char curent_directory[200];
            bzero(curent_directory,200);
            strcat(curent_directory,msg_client_user);
            //Cerere path director pentru copiere
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server]Introduceti path-ul catre directorul existent pentru copiere:\n 1) Folositi simbolul / pentru a copia in directoarele existente in alte directoare. ");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim directorul de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            copy_directory(curent_directory,msg_client);
        }
        if (strcmp(msg_client, "delete_f") == 0) {
            //Salvam directorul curent
            char curent_directory[200];
            bzero(curent_directory,200);
            strcat(curent_directory,msg_client_user);

            //Cerere nume fisier pentru stergere
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "[server]Alegeti numele fiserului impreuna cu extensia sa pentru stergere.\nSelectati un fisier din directorul curent.");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim fiserul de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            //Adaugam fisierul la path
            strcat(curent_directory, "/");
            strcat(curent_directory, msg_client);
            remove(curent_directory);
        }
        if (strcmp(msg_client, "copy_f") == 0) {
            //Salvam directorul curent
            char curent_directory[200];
            bzero(curent_directory,200);
            strcat(curent_directory,msg_client_user);

            //Cerere nume fisier pentru copiere
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "[server]Alegeti numele fiserului impreuna cu extensia sa pentru copiere.\nSelectati un fisier din directorul curent.");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim fiserul de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            //Adaugam fisierul la path
            strcat(curent_directory, "/");
            strcat(curent_directory, msg_client);
            //Cerere path director pentru copiere fisier
            bzero(msg_response, 20000);
            bzero(msg_client, 100);
            strcat(msg_response, "\n[server]Introduceti path-ul catre directorul existent pentru copiere:\n 1) Folositi simbolul / pentru a copia in directoarele existente in alte directoare.\n 2) Adagati numele fisierului si extensia sa la path.");
            if (write(tdl.client_descriptor, msg_response, sizeof(msg_response)) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
            //Citim directorul de la client
            if (read(tdl.client_descriptor, msg_client, sizeof(msg_client)) <= 0) {
                printf("[thread]-%d-", tdl.id_thread);
                perror("Failed to read()!\n");
            }
            copy_file(curent_directory,msg_client);
        }

        if (strcmp(msg_client, "list") == 0) {
            //listare
            listing_string = listing(msg_client_user);
            //Trimitem listarea catre client
            if (write(tdl.client_descriptor, listing_string.c_str(), listing_string.length()) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }
        }
        else if(delete_used==0) {
            //Listare + Comenzi
            listing_string ="\n[server]Aveti posibilitatea urmatoarelor comenzi:\n 1) list - listarea fisierelor sau a directoarelor dintr-un director\n 2) upload - pentru upload de fisiere intr-un director\n 3) download - pentru a downloada un fisier de pe server\n 4) mkdir - pentru crearea unui director\n 5) access - pentru a accesa un director\n 6) back - pentru a ne intoarce dintr-un director\n 7) delete_d - pentru a sterge un director\n 8) copy_d - pentru a copia un directorul curent intr-un director existent\n 9) copy_f - pentru a copia un fisier\n 10) delete_f - pentru a sterge un fisier\n ";
            listing_string += listing(msg_client_user);
            //Trimitem listarea catre client
            if (write(tdl.client_descriptor, listing_string.c_str(), listing_string.length()) <= 0) {
                printf("[thread]-%d-\n", tdl.id_thread);
                perror("Failed to write() to client!\n");
            }


        }

    }

}