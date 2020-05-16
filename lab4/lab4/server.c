#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "5000"   // port we're listening on

struct userentry{
    char user[256];
    int userfd;
};

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;
   
    struct userentry userlist[10];
    
    char* port = argv[1];
    
    if (argc != 2) {
        fprintf(stderr,"usage: server args error\n");
        exit(1);
    }
    
       
    for(int z=0; z<10; z++){
        strcpy(userlist[z].user, " ");
        userlist[z].userfd = 0;
    }
    

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    int numbytes, connectedsockets = 0;
    
    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } 
                    
                    else {
                        int error = 0;
                        char tempmessage[256];
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        numbytes = recv(newfd, buf, sizeof(buf), 0);
                        buf[numbytes] = '\0';
                        
                        for(int z=0; z<10; z++){
                            
                            if((strcmp(userlist[z].user, " ") == 0))
                                connectedsockets = z;
                            if(strcmp(buf,userlist[z].user) == 0){
                                strcpy(tempmessage, "error");

                                if (send(newfd, tempmessage, sizeof(tempmessage), 0) == -1) {
                                        perror("send");
                                }
                                close(newfd);
                                FD_CLR(newfd, &master); 
                                error = 1;
                            }
                        }
                        
                        if(error ==0){
                            strcpy(userlist[connectedsockets].user, buf);
                            userlist[connectedsockets].userfd = newfd;


                            strcpy(tempmessage, "New User Connected: ");
                            strcat(tempmessage,userlist[connectedsockets].user);
                            strcat(tempmessage, "\n");



                            // we got some data from a client
                                for(j = 0; j <= fdmax; j++) {
                                    // send to everyone!
                                    if (FD_ISSET(j, &master)) {
                                        // except the listener and ourselves
                                        if (j != listener && j != newfd){

                                            if (send(j, tempmessage, sizeof(tempmessage), 0) == -1) {
                                                perror("send");
                                            }
                                        }
                                    }
                                }

                            //printf("client # %d has name %s \n", newfd, latestuser->user);
                            printf("User: %s connected!\n", buf);
                        }
                    }
                } 
                
                else {
                   
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                       
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } 
                        
                        else {
                            perror("recv");
                        }
                        
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } 
                    
                    else {
                        
                        buf[nbytes] = '\0';
                        int list = 0;
                        int exit = 0;
                        if((strcmp(buf, "list\n") == 0)){
                             list = 1;
                        }
                        if((strcmp(buf, "exit\n") == 0)){
                             exit = 1;
                        }
                        
                        char* tok = " ";
                        char* tempbuff[256];
                        strcpy(tempbuff, buf);
                        char* command = strtok(tempbuff, tok);
                        int isuser = 0;
                        char tempuser[256];
                        char tempreceiver[256];
                        int tempsock;
                        
                        for(int z=0; z<10; z++){
                            if(i == userlist[z].userfd)
                                strcpy(tempuser, userlist[z].user);
                            
                            if((strcmp(command, userlist[z].user) == 0)){
                                    isuser = 1;
                                    strcpy(tempreceiver, userlist[z].user);
                                    tempsock = userlist[z].userfd;
                                }
                        }
                        
                        if(exit){
                            char tempmessage[256];
                            strcpy(tempmessage, "exit");
                            
                            if (FD_ISSET(i, &master)){
                                if(send(i, tempmessage, sizeof(tempmessage), 0) == -1) {
                                    perror("send");
                                }
                            }
                            int killed;
                            for(int z=0; z<10; z++){
                                if(i == userlist[z].userfd)
                                    killed = z;
                            }
                            strcpy(userlist[killed].user, " ");
                            userlist[killed].userfd = 0;
                            close(i); // bye!
                            FD_CLR(i, &master); // remove from master set
                        }
                        else if(list){
                            char tempmessage[256];
                            strcpy(tempmessage, "User List:");
                            for(int z=0; z<10; z++){
                                
                                if(strcmp(userlist[z].user, " ") != 0){
                                    strcat(tempmessage, " ");
                                    strcat(tempmessage,userlist[z].user);
                                    strcat(tempmessage, ",");
                                }
                            }
                            strcat(tempmessage, "\n");

                            
                            //real life hack methods, wasnt getting the message 
                            //after one send so we made it two and everything worked out
                            if(send(i, tempmessage, sizeof(tempmessage), 0) == -1) {
                                perror("send");
                            }
                            if(send(i, tempmessage, sizeof(tempmessage), 0) == -1) {
                                perror("send");
                            }
                        }
                        else if(isuser){
                            char tempmessage[256];
                            int length = strlen(tempreceiver) +1;
                            strcpy(tempmessage, &buf[length]);
                            char message[256];
                            strcpy(message,tempuser);
                            strcat(message, ": ");
                            strcat(message, tempmessage);

                            char finalmessage[256];
                            strcpy(finalmessage, message);
                            
                            
                            if (FD_ISSET(tempsock, &master)){
                                if (send(tempsock, message, sizeof(message), 0) == -1) {
                                            perror("send");
                                        }
                            }
                        }
                        else if((strcmp(command, "broadcast") == 0)){
                            
                            char tempmessage[256];
                            strcpy(tempmessage, &buf[10]);

                            
                            
                            for(int z=0; z<10; z++){
                                if(i == userlist[z].userfd){
                                    strcpy(tempuser, userlist[z].user);
                                    break;
                                }
                            }
                            char message[256];
                            strcpy(message,tempuser);
                            //strcat(message, tempuser);
                            strcat(message, ": ");
                            strcat(message, tempmessage);

                            char finalmessage[256];
                            strcpy(finalmessage, message);

                            
                            // we got some data from a client
                            for(j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j != listener && j != i){

                                        if (send(j, message, sizeof(message), 0) == -1) {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                        
                        
                        }
                        
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}
