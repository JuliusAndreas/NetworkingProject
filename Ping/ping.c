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

/**
 * Getting input (max pckt size, addreses and max wating time)
 * connect to server do pinging
 * get information and print
 * 
 */

#define MAX_ADDRESSES_LEN 512
#define MAX_NUMBER_OF_ADDRESSES 10
#define MAX_ADDRESS_LEN 30

//packet struct specialized for ping
struct ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

// Calculating the CheckSum
unsigned short checksum(void *b, int len){
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;  
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main(){
    
    char *unparsed_addresses = malloc(sizeof(char)*MAX_ADDRESSES_LEN);
    int address_index = 0;
    int max_waiting_time;
    int packet_size;
    int count = 0;
    int i = 0;
    char addresses[MAX_NUMBER_OF_ADDRESSES][MAX_ADDRESS_LEN];

    if(unparsed_addresses == NULL){
        fputs("failed to allocate memory.", stderr);
        exit(EXIT_FAILURE);
    }

    memset(unparsed_addresses, '\0', MAX_ADDRESSES_LEN);

    fputs("Please enter the addresses you want to be pinged:", stdout);
    fgets(unparsed_addresses, MAX_ADDRESSES_LEN, stdin);
    fputs("Please enter maximum waiting time:", stdout);
    scanf("%d",&max_waiting_time);
    fputs("Please enter size of the packet you want to be sent:", stdout);
    scanf("%d",&packet_size);

    for(;;){
        if(unparsed_addresses[i] == ' '){
            i++;
            continue;
        } else if(unparsed_addresses[i] == '\0'){
            break;
        } else if(unparsed_addresses[i] == '\n'){
            break;
        } else{
            int char_index = 0;
            while(unparsed_addresses[i] != ' ' && unparsed_addresses[i] != '\n' && unparsed_addresses[i] != '\0'){
                addresses[address_index][char_index] = unparsed_addresses[i];
                char_index++;
                i++;
            }
            addresses[address_index][char_index] = '\0';
            address_index++;
        }
        count++;
        i++;
    }

    



    return 0;
}