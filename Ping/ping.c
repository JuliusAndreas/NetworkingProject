#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

/**
 * Getting input (max pckt size, addreses and max wating time)
 * parse the input
 * connect to server do pinging
 * get information and print
 * 
 */

#define MAX_ADDRESSES_LEN 512
#define MAX_NUMBER_OF_ADDRESSES 10
#define MAX_ADDRESS_LEN 30
#define PORT_NO 0
#define PING_PKT_SIZE 64
#define RECV_TIMEOUT 1
#define PING_SLEEP_RATE 1000000


int pingloop = 1;

//struct for send_ping function parameters to do parallel pinging:
struct send_ping_parameters
{
    int sockfd;
    int max_waiting_time;
    int packet_size;
    struct sockaddr_in *addr_con;
    char *ip_addr;
    char temp_address[30];
};



//packet struct specialized for ping
struct ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_SIZE-sizeof(struct icmphdr)];
};

// Interrupt handler
void intHandler(int dummy)
{
    pingloop=0;
}

//Calculating the CheckSum
unsigned short checksum(void *b, int len){
    unsigned short *buffer = b;
    unsigned int sum=0;
    unsigned short result;  
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buffer++;
    if ( len == 1 )
        sum += *(unsigned char*)buffer;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con)
{
    printf("\nResolving DNS..\n");
    struct hostent *host_entity;
    char *ip=(char*)malloc(NI_MAXHOST*sizeof(char));
    int i;
  
    if ((host_entity = gethostbyname(addr_host)) == NULL)
    {
        //No ip found for host name
        return NULL;
    }
      
    //filling up address structure
    strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
  
    (*addr_con).sin_family = host_entity->h_addrtype;
    (*addr_con).sin_port = htons (PORT_NO);
    (*addr_con).sin_addr.s_addr  = *(long*)host_entity->h_addr;
  
    return ip;
}

void *send_ping(void *parameters){
    
    int ttl_val=64, msg_count=0, i, addr_len, flag=1, msg_received_count=0;
    struct send_ping_parameters args = *(struct send_ping_parameters*)parameters;
    int packet_size = args.packet_size;
    int max_waiting_time = args.max_waiting_time;
    int ping_sockfd = args.sockfd;
    struct sockaddr_in *ping_addr = args.addr_con;
    char *ping_ip = args.ip_addr;
    char *rev_host = args.temp_address;

    struct ping_pkt pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec=0, total_msec=0;
    struct timeval tv_out;
    tv_out.tv_sec = max_waiting_time;
    tv_out.tv_usec = 0;

    
    clock_gettime(CLOCK_MONOTONIC, &tfs);
  
      
    // set socket options at ip to TTL and value to 64,
    // change to what you want by setting ttl_val
    if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0)
    {
        printf("\nSetting socket options to TTL failed!\n");
        exit(EXIT_FAILURE);
    }
  
    else
    {
        printf("\nSocket set to TTL..\n");
    }

    // setting timeout of recv setting
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&tv_out, sizeof tv_out);
  
    //send icmp packet in an infinite loop
    while(pingloop)
    {
        //flag is whether packet was sent or not
        flag=1;
       
        //filling packet
        bzero(&pckt, sizeof(pckt));
          
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();
          
        for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
            pckt.msg[i] = i+'0';
          
        pckt.msg[i] = 0;
        pckt.hdr.un.echo.sequence = msg_count++;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
  
  
        usleep(PING_SLEEP_RATE);
  
        //send packet
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*) ping_addr, sizeof(*ping_addr)) <= 0)
        {
            printf("\nPacket Sending Failed!\n");
            flag=0;
        }
  
        //receive packet
        addr_len=sizeof(r_addr);
  
        if (recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count>1) 
        {
            printf("\nPacket receive failed!\n");
        }
  
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &time_end);
              
            double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec))/1000000.0;
            rtt_msec = (time_end.tv_sec-time_start.tv_sec) * 1000.0 + timeElapsed;
              
            //if packet was not sent, don't receive.
            if(flag)
            {
                if(!(pckt.hdr.type ==69 && pckt.hdr.code==0)) 
                {
                    printf("Error..Packet received with ICMP type %d code %d\n", pckt.hdr.type, pckt.hdr.code);
                }
                else
                {
                    printf("%d bytes from %s (%s) msg_seq=%d ttl=%d rtt = %Lf ms.\n", PING_PKT_SIZE, rev_host, ping_ip, msg_count,ttl_val, rtt_msec);
  
                    msg_received_count++;
                }
            }
        }    
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec))/1000000.0;
      
    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ timeElapsed;
                     
    printf("\n==%s ping statistics==\n", ping_ip);
    printf("\n%d packets sent, %d packets received, %f percent packet loss. Total time: %Lf ms.\n\n", msg_count, msg_received_count,((msg_count - msg_received_count)/msg_count) * 100.0,total_msec); 
}



int main(){
    
    char *unparsed_addresses = malloc(sizeof(char)*MAX_ADDRESSES_LEN);
    int address_index = 0;
    int max_waiting_time;
    int packet_size;
    int count = 0;
    int i = 0;
    char addresses[MAX_NUMBER_OF_ADDRESSES][MAX_ADDRESS_LEN];
    char *ip_addr = malloc(sizeof(char));
    int sockfd;
    struct sockaddr_in *addr_con = malloc(sizeof(struct sockaddr_in));


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

    //parsing process:
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

    pthread_t tids[address_index];
    struct send_ping_parameters parameters[address_index];

    for(int j = 0; j < address_index ; j++){
        char temp_address[30];
        int k;
        for(k = 0; addresses[j][k] != '\0' ; k++){
            temp_address[k] = addresses[j][k];
        }
        temp_address[k] = '\0';
        ip_addr = dns_lookup(temp_address, addr_con);

        if(ip_addr==NULL)
        {
            printf("\nhostname %s is not valid!\n", temp_address);
            return 0;
        }
        
        printf("\nTrying to connect to '%s' IP: %s\n",temp_address, ip_addr);
    
        //socket
        sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if(sockfd<0)
        {
            printf("\nSocket file descriptor failed!!\n");
            return 0;
        }else
            printf("\nSocket file descriptor %d was successfully received\n", sockfd);
    
        //catching interrupt
        signal(SIGINT, intHandler);    

        //send pings continuously
        parameters[j].addr_con = addr_con;
        parameters[j].ip_addr = ip_addr;
        parameters[j].max_waiting_time = max_waiting_time;
        parameters[j].packet_size = packet_size;
        parameters[j].sockfd = sockfd;
        for(int addr_index = 0; addr_index < 30 ; addr_index++){
            parameters[j].temp_address[addr_index] = temp_address[addr_index];
        }
        pthread_create(&tids[j], NULL, (void *)send_ping, (void *)&parameters[j]);
    }

    for(int join_counter = 0; join_counter < address_index; join_counter++){
        pthread_join(tids[join_counter], NULL);
    }

    return 0;
}



    



