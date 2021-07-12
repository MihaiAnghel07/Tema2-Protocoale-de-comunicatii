CFLAGS = -Wall

# Portul pe care asculta serverul
PORT_SERVER = 12345

# Id-ul clientului
# Adresa IP a serverului
ID_CLIENT = 
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.c
server: server.c

# Compileaza subscriber.c
subscriber: subscriber.c

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT_SERVER}

# Ruleaza clientul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber
