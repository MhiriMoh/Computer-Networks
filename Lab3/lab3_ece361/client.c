/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   client.c
 * Author: mhirimoh
 *
 * Created on March 3, 2018, 12:54 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4000"
#define CLIENTPORT "5555"
#define BUFFLEN 100

/*
 * 
 */
int main(void) {
 int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytesrec;
    char buff[BUFFLEN];
    struct sockaddr_storage response;
    struct timeval end, beginto, endto;

//    if (argc != 3) {
//        fprintf(stderr,"usage: talker hostname message\n");
//        exit(1);
//    }

    char * message = "hello world";
    char * message2 = "alex suffers from ed";
    int numbytes1, numbytes2;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
 
    if ((rv = getaddrinfo(NULL, SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    
    socklen_t resplen = sizeof(response);

    double to2, to, tprop;
    

    gettimeofday(&beginto,NULL);

    numbytes1 = sendto(sockfd, message, strlen(message), 0,
             p->ai_addr, p->ai_addrlen); 

    if (numbytes1 == -1) {
        perror("talker: sendto");
        exit(1);
    }

    numbytesrec = recvfrom(sockfd, buff,BUFFLEN-1, 0,
           (struct sockaddr *) &response, &resplen);   

    memset(&buff[0], '\0', sizeof(buff));

    gettimeofday(&endto, NULL);
    
    
    
    
    to = (((endto.tv_sec*1e6) + endto.tv_usec) - ((beginto.tv_sec*1e6) + beginto.tv_usec));
    
    
    
    
    gettimeofday(&beginto,NULL);

    numbytes2 = sendto(sockfd, message2, strlen(message2), 0,
             p->ai_addr, p->ai_addrlen); 

    if (numbytes2 == -1) {
        perror("talker: sendto");
        exit(1);
    }

    numbytesrec = recvfrom(sockfd, buff,BUFFLEN-1, 0,
           (struct sockaddr *) &response, &resplen);   

    memset(&buff[0], '\0', sizeof(buff));

    gettimeofday(&endto, NULL);

    to2 = (((endto.tv_sec*1e6) + endto.tv_usec) - ((beginto.tv_sec*1e6) + beginto.tv_usec));

    freeaddrinfo(servinfo);
    
    double timediff = to-to2;
    
    double R = 2*(8*(numbytes1-numbytes2))/(timediff*1e-6);
    
    tprop = (to - 2*8*numbytes1/R)/2;
    
    tprop += (to2 - 2*8*numbytes2/R)/2;
    
    tprop = tprop/2;
    
    //printf("talker: sent %d bytes to %s\n", numbytes, NULL);
    printf("R = %lf bps \n", R);
    //printf("talker: received %d bytes from %s\n", numbytesrec, NULL);
    //printf("to = %lf s \n", to*1e-6);
    //printf("tf = %lf us \n", tf);
    printf("tprop = %lf us \n", tprop);
    close(sockfd);

    return 0;
    return (EXIT_SUCCESS);
}

