#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <signal.h>

unsigned int* total_array;
void exit_()
{
    for(int i = 0; i < 94; i++)
    {
            printf("char ’%c’ : %u times\n",i + 32, total_array[i]);
    }
}

// MINIMAL ERROR HANDLING FOR EASE OF READING

int main(int argc, char *argv[])
{    
    int s, new_s, dont_egnore;
    unsigned int  pcc_total[94] = {0};
    total_array = pcc_total;
    unsigned int  bytes_read;
    uint32_t num_of_bytes, counter;
     
    struct sigaction sa;
    sa.sa_handler = exit_;
    sigaction(SIGINT, &sa, NULL);

  
    if(argc != 2)
    {   
        perror("\nServer Error : number of arguments is not valid \n");
        return 1;
    }  
    
    struct sockaddr_in sin = {.sin_family = AF_INET, 
	                          .sin_addr = htonl(INADDR_ANY), 
		                      .sin_port = htons(atoi(argv[1]))};

    s = socket(AF_INET, SOCK_STREAM, 0);
    
    /*struct sockaddr_in sin;
    memset( &sin, 0, sizeof(struct sockaddr_in));

    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(argv[1]));
    sin.sin_addr.s_addr = htonl(INADDR_ANY);*/
    
    /* int flag = 1;  
    if (-1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {  
        Perror("setsockopt fail");  
    } */ 
    
    if(0 != bind(s, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)))
    {
        perror("\nServer Error : Bind Failed. %s \n");
        return 1;
    }
    
    if(0 != listen(s, 10))
    {
        perror("\nServer Error : Listen Failed. %s \n");
        return 1; 
    }
    
    struct sockaddr_in client;
    
    while(1)
    {
 
        socklen_t addr_size = sizeof(client);
        new_s = accept(s,(struct sockaddr *) &client, &addr_size);
        //new_s = accept(s, (struct sockaddr*) &client, sizeof(struct sockaddr_in));
        if(new_s < 0)
        {
            perror("\nServer Error : Accept Failed. %s \n");
            return 1;            
        }
        
        bytes_read = read(new_s, &num_of_bytes, sizeof(uint32_t));
        
        if(bytes_read <= 0)
        {
            num_of_bytes = 0;
        }
        
        //data_buffer[bytes_read] = '\0';
        num_of_bytes = ntohl(num_of_bytes);
        char* data_buff1 = (char*)malloc(num_of_bytes); 
        //TODO print num of chars?
        
        dont_egnore = read(new_s, (void*)data_buff1, num_of_bytes);
        
        counter = 0;
        for(int i=0; i< num_of_bytes; ++i)
	    {
			if((int)data_buff1[i] >=32 || (int)data_buff1[i] <= 126)
			{
				counter++;
			}
		}

        counter = htonl(counter);
        dont_egnore = write(new_s, &counter, sizeof(uint32_t));
        if(0)
        {
            printf("%d", dont_egnore);
        }
        
        for(int i=0; i< num_of_bytes; ++i)
        {
		    if((int)data_buff1[i] >=32 || (int)data_buff1[i] <= 126)
		    {
			    pcc_total[(int)data_buff1[i] -32]++;
		    }
	    }
        
    }
    
    close(new_s);
}



