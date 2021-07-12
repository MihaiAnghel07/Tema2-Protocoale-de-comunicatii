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

#define BUFLEN 1500
int subscribers_len = 0;
int max_len = 1000;
int max_len_topics = 1000;
int topics_len = 0;
int subscriber_max_messages = 1000;
int max_topics_per_subs = 1000;

/* functia adauga un topic in lista de topicuri
   intoarce lista actualizata */
char** add_topic(char **topics, char *topic) 
{
	// in cazul in care se atinge capacitatea maxima
	if (topics_len == max_len_topics) {
		max_len_topics *= 2;
		char **tmp = (char**)realloc(topics, max_len_topics * sizeof(char*));
		
		if (tmp != NULL) {
			topics = tmp;
		}

		for (int i = topics_len; i < max_len_topics; i++) {
			topics[i] = (char*)malloc(51 * sizeof(char));
		}
	}

	strcpy(topics[topics_len], topic);
	topics_len++;
	
	return topics;
}

/* functia verifica daca subscriberu-ul este abonat la
   topicul primit ca parametru */
int is_subscribed_to(subscriber s, char *topic)
{
	for (int i = 0; i < s.nr_topics; i++) {
		if (strcmp(s.topics[i].topic, topic) == 0) {
			return 1;
		}
	}

	return 0;
}

/* functia adauga un nou topic la care subscriber-ul va
   fi abonat */
struct_topic* add_topic_to_subs(subscriber *s, char *topic, int sf)
{	

	if (!is_subscribed_to(*s, topic)) {

		if (s->nr_topics == max_topics_per_subs) {
			// trebuie sa maresc capacitatea
			max_topics_per_subs *= 2;
			struct_topic *tmp = (struct_topic *)realloc(s->topics, max_topics_per_subs * sizeof(struct_topic));
			
			if (tmp != NULL) {
				s->topics = tmp;
			}
		}
		
		strcpy(s->topics[s->nr_topics].topic, topic);
		s->topics[s->nr_topics].sf = sf;
		s->nr_topics++; 
	}

	return s->topics;
}

/* functia elimina un topic din lista de topicuri urmarite
   de subscriber */
void delete_topic_from_subs(subscriber *s, char *topic)
{
	
	for (int i = 0; i < s->nr_topics; i++) {
		if (strcmp(s->topics[i].topic, topic) == 0) {
			for (int j = i; j < s->nr_topics - 1; j++) {
				s->topics[j] = s->topics[j + 1];
			}

			break;
		}
	}

	s->nr_topics--;
}

/* functia adauga un mesaj in lista de mesaje ce urmeaza sa 
   fie trimise catre subscriber cand acesta devine activ */
message* add_message_to_subs(subscriber *s, message msg)
{
	if (s->nr_messages == subscriber_max_messages) {
		//trebuie sa realocam memorie
		subscriber_max_messages *= 2;
	    message *m = realloc(s->messages, subscriber_max_messages * sizeof(message));
		if (m != NULL) {
  			s->messages = m;
		}
	}

	s->messages[s->nr_messages] = msg;
	s->nr_messages++;

	return s->messages;		   				
}

/* verifica daca un topic exista sau nu in lista de topicuri */
int exist_topic(char **topics, char *topic)
{
	for (int i = 0; i < topics_len; i++) {
		if (strcmp(topics[i], topic) == 0) {
			return 1;
		}
	}
	return 0;
}

/* se primeste sockfd-ul si se intoarce id-ul aferent al 
   subscriber-ului */
char* get_id(subscriber *subscribers, int sockfd)
{	
	for (int i = 0; i < subscribers_len; i++) {
		if (subscribers[i].sockfd == sockfd) {
			return subscribers[i].id;
		}
	}

	return NULL;
}

/* adauga un subscriber in lista (sau il activeaza) */
subscriber* add_subscriber(subscriber *subscribers, char *id, int sockfd)
{	
	// in cazul in care se atinge dimensiunea alocata, se realoca
	// o dimensiune dubla
	if (subscribers_len == max_len) {
		max_len *= 2;
		subscriber *s = realloc(subscribers, max_len * sizeof(subscriber));
		if (s != NULL) {
			subscribers = s;
		}
	}

	// daca nu se gaseste nicun subscriber cu sockfd in lista,
	// inseamna ca este un client nou ce trebuie adaugat in lista
	if (get_id(subscribers, sockfd) == NULL) {
		
		strcpy(subscribers[subscribers_len].id, id);
		subscribers[subscribers_len].sockfd = sockfd;
		subscribers[subscribers_len].topics = (struct_topic *)calloc(max_topics_per_subs, sizeof(struct_topic));
		for (int i = 0; i < max_topics_per_subs; i++) {
			subscribers[subscribers_len].topics[i].sf = -1;
		}

		subscribers[subscribers_len].nr_topics = 0;
		subscribers[subscribers_len].nr_messages = 0;
		subscribers[subscribers_len].messages = (message*)malloc(subscriber_max_messages * sizeof(message)); 
		subscribers[subscribers_len].active = 1;
		subscribers_len++;

	} else {
		// altfel inseamna ca exista si doar trebuie sa il activez
		int i = 0;
		for (i = 0; i < subscribers_len; i++) {
			if (strcmp(subscribers[i].id, id) == 0) {
				subscribers[i].active = 1;
				break;
			} 
		}

		// ne va ajuta in main, cand trimitem mesajele catre un subscriber
		// ce a fost inactiv si a optat sa nu piarda mesajele
		subscriber s = subscribers[i];
		subscribers[i] = subscribers[subscribers_len - 1];
		subscribers[subscribers_len - 1] = s;
		
	}
	
	return subscribers;
}

/* scoate un subscriber din lista (defapt doar il face inactiv)*/
void delete_subscriber(subscriber *subscribers, int sockfd)
{
	// practic doar il dezactivez
	for (int i = 0; i < subscribers_len; i++) {
		if (subscribers[i].sockfd == sockfd) {
			subscribers[i].active = 0;
			break;
		}
	}
}

/* verifica daca exista subscriber-ul cu id-ul respectiv in lista */
int exist_subscriber(subscriber *subscribers, char *id) 
{
	// se considera ca un subscriber exista in lista atunci cand 
	// este in lista si este activ
	for (int i = 0; i < subscribers_len; i++) {
		if (strcmp(subscribers[i].id, id) == 0 && subscribers[i].active == 1) {
			return 1;
		}
	}
	return 0;
}

/* se dezaboneaza subscriber-ul cu sockfd-ul corespunzator de la 
   topicul primit ca parametru */
void unsubscribe(int sockfd, char *topic, subscriber* subscribers)
{
	for (int i = 0; i < subscribers_len; i++) {
		if (subscribers[i].sockfd == sockfd) {
			delete_topic_from_subs(&subscribers[i], topic);
			break;
		}
	}
}

/* se aboneaza subscriber-ul cu sockfd-ul corespunzator la 
   topicul primit ca parametru */
void subscribe(int sockfd, char *topic, char** topics, int sf, subscriber* subscribers)
{
	// in cazul in care nu exista topicul in lista, trebuie sa il adaug
	if (!exist_topic(topics, topic)) {
		topics = add_topic(topics, topic);
	}

	for (int i = 0; i < subscribers_len; i++) {
		if (subscribers[i].sockfd == sockfd) {
			subscribers[i].topics = add_topic_to_subs(&subscribers[i], topic, sf);
			break;
		}
	}
}

/* functia formeaza un mesaj cu structura 'message' cu informatiile primite
   in buffer si from_station */
void create_msg(message *msg, unsigned char *buf, struct sockaddr_in from_station) 
{
	// formez topic-ul
	int i = 0;
    while (buf[i] != '\0') {
    	msg->topic[i] = buf[i];
    	i++;
    }
    msg->topic[i] = '\0';
    
    //formez tipul de date	
    msg->tip_date = buf[50];

    // daca payload-ul contine un INT
    if (buf[50] == 0) {

    	uint32_t nr1 = 0;
    	nr1 = (uint32_t)buf[52] << 24 |
      		  (uint32_t)buf[53] << 16 |
      		  (uint32_t)buf[54] << 8  |
      		  (uint32_t)buf[55];
   
        signed long long int nr = nr1;
        // salvez numarul
   		if (buf[51] == 1) {
   			msg->int_number = -nr;

    	} else {
    		msg->int_number = nr;
   		}
   	}

    // daca payload-ul contine un short_real	
    if (buf[50] == 1) {

    	uint16_t nr2 = 0;
    	nr2 =  (uint16_t)buf[51] << 8 | (uint16_t)buf[52];
    	msg->short_real_number = nr2;
    }

    // daca payload-ul contine un float
    if (buf[50] == 2) {

    	uint32_t nr3 = 0;
   	 	uint8_t nr4 = 0;
    	nr3 = (uint32_t)buf[52] << 24 |
      		  (uint32_t)buf[53] << 16 |
      		  (uint32_t)buf[54] << 8  |
      		  (uint32_t)buf[55];
      	    
        nr4 = (uint8_t)buf[56];
      	double exp = 1;
        for (int j = 0; j < nr4; j++) {
      	    exp /= 10;
      	}

      	double nr5 = (double)nr3 * exp;
        if (buf[51] == 1) {
      	   	msg->float_number = (double)-nr5;
      	 
        } else {

        	msg->float_number = (double)nr5;
      	}
   	}

   	// daca payload-ul contine un string
    if (buf[50] == 3) {
    	i = 51;
    	int j = 0;
    	while (buf[i] != '\0' && i < BUFLEN) {
    
    		msg->string[j] = buf[i];
    		i++;
    		j++;
    	}
    	msg->string[j] = '\0';
    }
					
    strcpy(msg->saddr, inet_ntoa(from_station.sin_addr));
    msg->port = ntohs(from_station.sin_port);
}

/* functie ajutatoare pentru situatia in care uit care e ordinea argumentelor */
void usage(char *file)
{
	fprintf(stderr,"Usage: %s server_port \n",file);
	exit(0);
}

int main(int argc, char **argv)
{

	// in cazul in care argumentele sunt gresite
	if (argc != 2) {
		usage(argv[0]);
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	
	struct sockaddr_in sockaddr_UDP, from_station, serv_addr_TCP, subs_addr;
	unsigned char buf[BUFLEN];
	char exit_command[5], buffer[100];		
	int port = 0, ret;

	
	// in listele urmatoare voi salva subscriberii si toate topicurile 
	// ce apar in server
	subscriber *subscribers = malloc(max_len * sizeof(subscriber));
	char **topics = (char**)malloc(max_len_topics * sizeof(char*));
	for (int i = 0; i < max_len_topics; i++) {
		topics[i] = (char*)malloc(50 * sizeof(char));
	}
	
	// Deschidere socket UDP
	int sockid_UDP = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sockid_UDP == -1, "ERROR: cannot open socket UDP");
	
	// Setare struct sockaddr_in pentru a asculta pe portul respectiv 
	sockaddr_UDP.sin_family = AF_INET;
	port = atoi(argv[1]);
	DIE(port == 0, "port");
	sockaddr_UDP.sin_port = htons(port);
	inet_aton("127.0.0.1", &(sockaddr_UDP.sin_addr)); 
	
	// Legare proprietati de socket
 	ret = bind(sockid_UDP, (struct sockaddr *)&sockaddr_UDP, sizeof(sockaddr_UDP));
 	DIE(ret == -1, "ERROR: cannot bind UDP");

 	// creez un socket TCP
 	int sockid_TCP = socket(PF_INET, SOCK_STREAM, 0);
    DIE(sockid_TCP == -1, "ERROR: cannot open socket TCP\n");

    // dezactivez Neagle algorithm
    char flag = 0;
	int result = setsockopt(sockid_TCP, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));    
 	DIE(result < 0, "TCP_NODELAY");

    // Setare struct sockaddr_in pentru a asculta pe portul respectiv 
    memset((char *) &serv_addr_TCP, 0, sizeof(serv_addr_TCP));
    serv_addr_TCP.sin_family = AF_INET;
	port = atoi(argv[1]);
	DIE(port == 0, "port");
	serv_addr_TCP.sin_port = htons(port);
	inet_aton("127.0.0.1", &(serv_addr_TCP.sin_addr)); 
	serv_addr_TCP.sin_addr.s_addr = INADDR_ANY;

	// Legare proprietati de socket TCP
	ret = bind(sockid_TCP, (struct sockaddr *)&serv_addr_TCP, sizeof(serv_addr_TCP));
 	DIE(ret == -1, "ERROR: cannot bind TCP");

 	ret = listen(sockid_TCP, 10);
	DIE(ret < 0, "ERROR: listen");

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	// se goleste multimea de descriptori de citire (read_fds) 
	// si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) 
	// in multimea read_fds
	int zero = 0;            // pentru citire de la stdin
	FD_SET(zero, &read_fds);
	FD_SET(sockid_UDP, &read_fds);
	FD_SET(sockid_TCP, &read_fds);
	
	if (sockid_TCP > sockid_UDP) {
		fdmax = sockid_TCP;	
	} else {
		fdmax = sockid_UDP;
	}
	
	
	while (1) {

		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				
				if (i == 0) {
					
					// citesc de la tastatura
					//se asteapta o comanda de exit de la tastatura
					memset(exit_command, 0, 5);
					fgets(exit_command, 5, stdin);
					
					if (strcmp(exit_command, "exit") == 0) {
    					
    					message m;
    					strcpy(m.topic, exit_command);
    					
    					// se trimite semnalul catre toti subscriberii ca sa se inchida	
    					for (int j = 0; j < subscribers_len; j++) {
    						if (subscribers[j].active == 1) {
    							int snd = send(subscribers[j].sockfd, &m, sizeof(m), 0);
    							DIE(snd < 0, "send");
    						}
    					}

    					goto finish;
    				}

    			} else if (i == sockid_UDP) {

    				// se primesc mesajele de la clientii UDP
					unsigned int addrlen = sizeof(struct sockaddr_in);
					ret = recvfrom(sockid_UDP, buf, sizeof(buf), 0, (struct sockaddr *)&from_station, (socklen_t*)&addrlen);
 					DIE(ret == -1, "ERROR: cannot read UDP buffer");
					
					// formez mesajul din informatiile primite in buffer si cu
					// adresa si portul clientului UDP
			    	message msg;
			   		create_msg(&msg, buf, from_station);
			   		
			   		if (!exist_topic(topics, msg.topic)) {
			   			topics = add_topic(topics, msg.topic);
			   		}

			   		// trebuie sa trimit mesajul catre subscriberii activi
			   		// si sa pun mesajele in array-ul messages al subscriberilor
			   		// inactivi
			   		for (int j = 0; j < subscribers_len; j++) {
						
						if (is_subscribed_to(subscribers[j], msg.topic)) {
			   				// daca e activ trimit direct mesajul
			   				if (subscribers[j].active == 1) {
			   			
			   					ret = send(subscribers[j].sockfd, &msg, sizeof(msg), 0);
			   					DIE(ret < 0, "send");		
			   				
			   				} else if (subscribers[j].active == 0) {
			   					// in functie de alegerea clientului la momentul in care
			   					// s-a abonat, aplicam store-and-forward
			   					for (int k = 0; k < subscribers[j].nr_topics; k++) {
			   						if (strcmp(subscribers[j].topics[k].topic, msg.topic) == 0) {
			   							if (subscribers[j].topics[k].sf == 1) {
			   								subscribers[j].messages = add_message_to_subs(&subscribers[j], msg);
			   								break;
			   							}
			   						}
			   					}

			   				}
			   			}
			   		}

				} else if (i == sockid_TCP) {

					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					socklen_t subs_len = sizeof(subs_addr);
					int newsockfd = accept(sockid_TCP, (struct sockaddr *) &subs_addr, &subs_len);
					DIE(newsockfd < 0, "accept");

					// primesc id-ul, verific daca exista deja si trimit un mesaj 
					// de notificare catre client
					message mess;
					memset(mess.topic, 0, 51);
					int n = recv(newsockfd, &mess, sizeof(mess), 0);
					DIE(n < 0, "recv");

					char *id = mess.topic;
					// daca nu exista acest id in lista (sau nu e activ)
					if (!exist_subscriber(subscribers, id)) { 
						
						// se adauga noul socket intors de accept() la multimea 
						// descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
						
						// adaug noul subscriber in lista (sau il activez)
						subscribers = add_subscriber(subscribers, id, newsockfd);
						printf("New client %s connected from %s:%d.\n",
							id, inet_ntoa(subs_addr.sin_addr), ntohs(subs_addr.sin_port));
    					fflush(stdout);

						// trimit un mesaj catre subscriber prin care ii spun
						// ca s-a realizat conexiunea
						message mess;
						memset(mess.topic, 0, 51);
    					strcpy(mess.topic, "!exist");
						ret = send(newsockfd, &mess, sizeof(mess), 0);
    					DIE(ret < 0, "send");

    					// ii trimit clientului toate mesajele primite in timp
    					// ce era inactiv (daca este cazul)
    					if (subscribers[subscribers_len - 1].nr_messages > 0) {
    		
    						int j = 0;
    						while (j < subscribers[subscribers_len - 1].nr_messages) {
								
								message m = subscribers[subscribers_len - 1].messages[j];
    							ret = send(newsockfd, &m, sizeof(m), 0);
    							DIE(ret < 0, "send");
    							j++;
    						}
    						
    						subscribers[subscribers_len - 1].nr_messages = 0;
    					}
					
					} else {

						// trimit un mesaj catre subscriber prin care ii spun
						// ca id-ul exista deja
						printf("Client %s already connected.\n", id);
						fflush(stdout); 
						memset(buffer, 0, 15);
						sprintf(buffer, "%s", "already exist");
						
						ret = send(newsockfd, buffer, strlen(buffer), 0);
    					DIE(ret < 0, "send");
					}

				} else {

					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					int n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {

						// conexiunea clientului TCP s-a inchis
						printf("Client %s disconnected.\n", get_id(subscribers, i));
						fflush(stdout);
						delete_subscriber(subscribers, i);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					
					} else {

						// primesc mesaj de la clientii TCP
						// primesc topicul si sf, daca exista un ' ', inseamna ca
						// este comanda de subscribe, altfel e comanda de unsubscribe
						if (strchr(buffer, ' ')) {
							// extrag topicul
							char topic[51];
							memset(topic, 0, 51);
							strncpy(topic, buffer, strlen(buffer) - 3);
							char sf[2];
							sf[0] = buffer[strlen(buffer) - 2];
							sf[1] = '\0';
							
							subscribe(i, topic, topics, atoi(sf), subscribers);

						} else {

							unsubscribe(i, buffer, subscribers);
						}
					}
				}
			}
		}
	}

finish:
    /*Dealocam memoria alocata anterior*/
	for(int i = 0; i < subscribers_len; i++) {
		free(subscribers[i].topics);
	}
	free(subscribers);

	for (int i = 0; i < max_len_topics; i++) {
		free(topics[i]);
	}
	free(topics);

	/*Inchidere socket*/	
	close(sockid_UDP);
		

	return 0;
}