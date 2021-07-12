
#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

typedef struct {
  char topic[50];
  char tip_date;
  signed long long int int_number;
  uint16_t short_real_number;
  double float_number;
  char string[1500];
  char saddr[10];
  int port;
} message;

typedef struct {
  char topic[50];
  char sf;
}struct_topic;

typedef struct {
  char id[10];
  int sockfd; 
  struct_topic *topics;
  int nr_topics;
  message *messages;
  int nr_messages;
  char active;
} subscriber;



