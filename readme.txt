***************************
Nume: Anghel Mihai-Gabriel
Grupa: 323CD
***************************

**Structuri folosite** 
 - message: Aceasta structura ajuta la generarea unui mesaj tip prin
            intermediul caruia pot comunica server-ul si clientii UDP/TCP.
	    Structura contine topicul, tipul de date, datele respective,
            adresa ip a clientului UDP si portul pe care comunica acesta.

 - struct_topic: Este compusa din doua campuri: topicul si un camp sf care
     		 semnifica faptul ca subscriber-ul a ales ca server-ul sa 
 		 foloseasca principiul store and forward pentru topicul
		 respectiv.

 - subscriber: Aceasta structura ne ajuta la nivelul server-ului pentru
               a tine o evidenta a clientilor TCP activi, a listei de 
	       structuri de tipul de mai sus, a mesajelor care se salveaza 
	       intr-o lista pentru a putea fi trimise catre client in
               momentul in care acesta devine activ (doar daca acesta opteaza
	       pentru acest lucru).

Structurile sunt definite in fisierul 'helper.h' impreuna cu Macro-ul DIE.


****Server****

Server-ul urmeaza scheletul din laboratoarele realizate.
Stiu ca este contraindicat sa folosim variabile globale, insa folosirea
acestora m-au ajutat la lizibilitate si la simplificarea unor functii, in plus
am fost atent si am testat functionalitatea in diferite moduri pentru a ma
asigura ca nu apare vreo problema cauzata de acest lucru.

Pentru realizarea functionalitatii server-ului, am creat mai multe functii a
caror scop este descris in program.

Flow:
 - dezactivez buffeing-ul la afisare
 - aloc dinamic o lista de subscriberi
 - aloc dinamic o lista de topicuri cu lungimi de maxim 50 de caractere
 - deschid un socket UDP
 - creez un socket TCP
 - dezactivez algoritmul Neagle
 - creez o multime de descriptori de citire
 - apoi se intra intr-o bucla while(1) pana la primirea unei comenzi 'exit'
 - daca fd(file descriptor) este 0, atunci se asteapta citirea de la stdin
   a unui mesaj de 'exit' si in acelasi timp se notifica toti subscriberii
   ca trebuie sa isi inchida conexiunea cu server-ul
 - daca fd este egal cu fd-ul socket-ului UDP, se primeste mesaj de la un
   client UDP, se genereaza un mesaj tip message, se actualizeaza lista
   de topicuri si se trimite mesajul catre subscriberii abonati la topicul
   respectiv, sau se salveaza mesajul in listele subscriberilor care vor sa
   primeasca mesajul cand vor deveni activi.
 - daca fd este egal cu fd-ul socketu-ului TCP, inseamana ca un client TCP
   vrea sa se conecteze la server. Daca clientul vrea sa se conecteze cu un
   id deja existent, si id-ul este activ, atunci se trimite un mesaj 
   clientului TCP prin care i se transmite ca nu se poate conecta. Daca
   id-ul este inactiv, inseamna ca un client se reconecteaza si in acest
   moment ii sunt transmise toate mesajele trimise in timp ce era inactiv
   (daca a optat pentru asta).
 - daca fd este diferit de cele de mai sus, se primeste un mesaj de la un
   client TCP prin care server-ul e anuntat fie ca subscriber-ul paraseste 
   conexiunea, fie ca acesta doreste sa se aboneze / dezaboneze la un topic.



****Subscriber****

Si in cazul subscriber-ului am urmat scheletul din laborator.
Functionalitatea acestuia este mai simpla motiv pentru care voi trece la flow.

Flow:
 - dezactivez buffeing-ul la afisare
 - creez un socket TCP
 - dezactivez algoritmul Neagle
 - se face conectarea la server
 - trimit catre server id-ul subscriber-ului si primesc un mesaj de la server
   care imi spune daca este valid sau nu. Daca id-ul este folosit deja, se
   inchide clientul.
 - creez o multime de descriptori de citire
 - se intra intr-o bucla while(1) pana la primirea unei comenzi 'exit' fie
   de la server, fie de la stdin
 - pentru fd = 0, se primeste de la stdin o comanda 'exit', subscribe sau
   unsubscribe, in functie de stdin, se genereaza un mesaj si se trimite
   server-ului.
 - daca fd este egal cu sockfd-ul TCP, se primeste un mesaj tip message
   de la server cu informatiile la care subscriber-ul este abonat si se 
   afiseaza.


