#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_ADRESSES_LEN 512

int main(){
    
    char *unparsed_addresses = malloc(sizeof(char)*MAX_ADRESSES_LEN);
    int counter;
    int max_waiting_time;
    int packet_size;

    fputs("Please enter the addresses you want to be pinged:", stdout);
    fgets(unparsed_addresses, MAX_ADRESSES_LEN, stdin);
    fputs("Please enter maximum waiting time:", stdout);
    scanf("%d",&max_waiting_time);
    fputs("Please enter size of the packet you want to be sent:", stdout);
    scanf("%d",&max_waiting_time);

    return 0;
}