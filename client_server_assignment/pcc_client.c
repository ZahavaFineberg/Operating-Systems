#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>


        
int main(int argc, char* argv[])
{
    

    int s, dont_egnore;
    long length ;
    uint32_t amnt_bytes, recv_counted;
    char *buffer;
    if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nERROR : Could not create socket");
        return 1;
    }
    
    /*struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(argv[2]));
    //inet_aton(argv[1], &sin.sin_addr.s_addr);
    inet_aton(argv[1], &sin.sin_addr);*/
    
    struct in_addr serverIP;
        if(0 == inet_aton(argv[1], &serverIP)) 
        {
		    perror("\n Error : inet_aton failed\n");	
		    return 1;
	    }
    
    struct sockaddr_in sin = {.sin_family = AF_INET, 
	                          .sin_addr = serverIP, 
		                      .sin_port = htons(atoi(argv[2]))};
    
    printf("Client: connecting...\n");
    if(connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        perror("\nClient ERROR : Connection failed");
        return 1;
    }
    
    printf("Client: Conneccted.\n");
    
    FILE *fptr;
    fptr = fopen(argv[3],"rb");
    if(fptr)
    {
        fseek(fptr, 0, SEEK_END);
        length = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);
        buffer = malloc(length + 1);
        if(buffer)
        {
            dont_egnore = fread(buffer, 1, length, fptr);
        }
        else
        {
            perror("\nClient ERROR : Malloc did not succed");
            return 1;
        } 
    }
    else
    {
        perror("\nClient ERROR : Could not open the file");
        return 1;
    }
    
    fclose(fptr);
    buffer[length] = '\0';
    
    amnt_bytes = htonl(length);
    if(write(s, &amnt_bytes, sizeof(amnt_bytes)) == -1)
    {
        perror("Client ERROR : Problem with writing... \n");
    }
    
    if( -1 == write(s, buffer, ntohl(amnt_bytes)))
    {
        perror("Client ERROR : Problem with writing... \n");
    }    
    
    dont_egnore = read(s,&recv_counted, sizeof(uint32_t));    
    if(0)
    {
        printf("%d",dont_egnore);
    }
	printf("# of printable characters: %u\n",ntohl(recv_counted));

    close(s);
    
    return 0;
}      
        

     

