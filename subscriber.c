#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "helper.h"

#define BUFLEN 1501

/* functie ajutatoare pentru situatia in care uit care e ordinea argumentelor */
void usage(char *file)
{
	fprintf(stderr,"Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER> \n",file);
	exit(0);
}

int main(int argc, char **argv)
{	
	// in cazul in care argumentele sunt gresite
	if (argc != 4) {
		usage(argv[0]);
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	char buffer[BUFLEN];
	int port = 0;
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);

    // creez un socket TCP
    int sockfd_TCP = socket(PF_INET, SOCK_STREAM, 0);
    DIE(sockfd_TCP == -1, "create TCP socket failed");
    	
    // dezactivez Neagle algorithm
    char flag = 0;
	int result = setsockopt(sockfd_TCP, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));    
 	DIE(result < 0, "TCP_NODELAY");

    // completez in serv_addr adresa serverului, familia de adrese
    // si portul pentru conectare
    serv_addr.sin_family = AF_INET;
    inet_aton(argv[2], &(serv_addr.sin_addr));
    port = atoi(argv[3]);
    DIE(port == 0, "port");
    serv_addr.sin_port = htons(port);

    // creez conexiunea catre server
    int ret = connect(sockfd_TCP, (struct sockaddr*)&serv_addr, socket_len);
    DIE(ret == -1, "connect failed");
	
	// trimit id-ul pentru a verifica daca exista deja acest id 
	message mess;
    memset(mess.topic, 0, 51);
    strcpy(mess.topic, argv[1]);
    ret = send(sockfd_TCP, &mess, sizeof(mess), 0);
    DIE(ret < 0, "send");

    // primesc mesajul care imi spune daca e valid sau nu id-ul
    memset(mess.topic, 0, 51);
    ret = recv(sockfd_TCP, &mess, sizeof(mess), 0);
	DIE(ret < 0, "receive");
	
	// daca exista deja, se inchide clientul
	if (strstr(mess.topic, "already")) {
		goto finish;
	}


    fd_set read_fds, tmp_fds;

	// se goleste multimea de descriptori de citire 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se adauga multimea de descriptori de citire
	int zero = 0;
	FD_SET(zero, &read_fds);
	FD_SET(sockfd_TCP, &read_fds);
	int fd_max = sockfd_TCP;
    
    while (1) {

    	tmp_fds = read_fds;

		ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret == -1, "select failed");

		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {
					
					// citesc de la stdin si trimit catre destinatie
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					
					if (strncmp(buffer, "exit", 4) == 0) { 
						goto finish;
					}

					if (!strstr(buffer, "subscribe")) {
						continue;
					}

					// formez mesajul
					int j = 0;
					char msg[51];
					while (buffer[j] != ' ') {
						j++;
					}

					j++;
					int k = 0;
					while (buffer[j] != '\0') {
						msg[k] = buffer[j];
						j++;
						k++;
					}
					msg[k] = '\0';

					int snd = send(sockfd_TCP, msg, strlen(msg), 0);
					DIE(snd < 0, "send");

					if (strstr(buffer, "unsubscribe")) {
						printf("Unsubscribed from topic.\n");
						fflush(stdout);
						continue;
					}

					if (strstr(buffer, "subscribe")) {
						printf("Subscribed to topic.\n");
						fflush(stdout);
						continue;
					}					
					
				} else if (i == sockfd_TCP) {
					
					// primesc date de la server (la ceea ce sunt abonat)
					message m;
					ret = recv(sockfd_TCP, &m, sizeof(message), 0);
					DIE(ret < 0, "receive");
					
					if (strcmp(m.topic, "exit") == 0) {
						goto finish;
					}

					printf("%s:%d - %s - ", m.saddr, m.port, m.topic);
					fflush(stdout);
			   		switch (m.tip_date) {
			   			case 0: printf("INT - %lld\n", m.int_number); 
			   			break;
			   			case 1: printf("SHORT_REAL - %f\n", (double)m.short_real_number / 100);
			   			break;
			   			case 2: printf("FLOAT - %lf\n", m.float_number);
			   			break;
			   			case 3: printf("STRING - %s\n", m.string);
			   			break;
			   		}
			   		fflush(stdout);
				}
			}
		}
    }

finish:
    // inchid conexiunea si socketul creat
    shutdown(sockfd_TCP, 2);
    close(sockfd_TCP);

	
	return 0;
}