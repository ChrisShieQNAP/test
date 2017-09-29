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
#include <time.h>
#include <memory.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>

#define QUEUE_SIZE 128
#define BUF_SIZE 1024

//char buffer[BUF_SIZE];      /* Character buffer */
pthread_mutex_t mutex;
bool read_finish = false;
//unsigned char rfd_name[80];
unsigned char wfd_name[] = "test.jpg";
long file_size = 0;
int server_port = 25480;
unsigned char server_ip[] = "192.168.0.1";
bool rw_error_flag = false;
//-ring buffer-
typedef struct 
{
    int head;
    int tail;
    unsigned char **buffer; 
} QUEUE; 

QUEUE queue; 

void Queue_Init(QUEUE *queue)
{
    queue->head = 0;
    queue->tail = 0;
    queue->buffer = (unsigned char **) malloc(sizeof(unsigned char *)*QUEUE_SIZE); 
    for(int i = 0;i<QUEUE_SIZE;i++)
        queue->buffer[i] = (unsigned char *) malloc(sizeof(unsigned char)*BUF_SIZE);  
} 

void Queue_Destroy(QUEUE *queue)
{
    if (queue->buffer != 0)
    {
        for(int i = 0; i<QUEUE_SIZE;i++)
            free(queue->buffer[i]);
        free(queue->buffer);
    }
} 

bool Queue_Is_Empty(QUEUE *queue)
{
    if(queue->head == queue->tail)
        return true;
    return false; 
} 

bool Queue_Is_Full(QUEUE *queue)
{
    if((queue->tail+1)%QUEUE_SIZE==queue->head)
        return true;
    return false; 
} 

bool Queue_Get(QUEUE *queue, unsigned char *data , int load_size)
{
    if(queue->head == queue->tail)
        return false;
    
    /*for(int i = 0;i<load_size;i++)
    {
    	*(data+i*sizeof(unsigned char)) = *(queue->buffer[queue->head]+i*sizeof(unsigned char));
        
    }*/
    memcpy(data, queue->buffer[queue->head], load_size * sizeof(unsigned char));
    queue->head = (queue->head+1)%QUEUE_SIZE; 

    return true;
} 

bool Queue_Put(QUEUE *queue, unsigned char *data)
{
    if(Queue_Is_Full(queue))
        return false; 
    /*for(int i = 0;i<BUF_SIZE;i++)
    {
    	*(queue->buffer[queue->tail]+i*sizeof(unsigned char)) = *(data+i*sizeof(unsigned char));
    }*/
    memcpy(queue->buffer[queue->tail], data, BUF_SIZE * sizeof(unsigned char));
    queue->tail=(queue->tail+1)%QUEUE_SIZE; 

    return true;
} 

//-call from pthread-
void *Reader_thread(void* buf)
{
    int sockfd = 0, n = 0,numbytes = 0;
    unsigned char recvBuff[1025];
    bool get_flag = true;
    fd_set readfds;
    struct timeval tv;
    struct sockaddr_in serv_addr; 
    tv.tv_sec = 0;
    tv.tv_usec = 1000000;
    
    //-socket connect-
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        rw_error_flag = true;
        pthread_exit(NULL);
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 
    int nSendBuf=32*1024;//设置为32K
    //setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(server_port); 
    
    if (inet_aton(server_ip, &serv_addr.sin_addr) == 0)   //check server ip
    {    
        printf("Server IP Address Error!\n");    
        exit(1);    
    }    

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       close(sockfd);
       rw_error_flag = true;
       pthread_exit(NULL);
    } 
    
    numbytes = read(sockfd, &file_size, sizeof(long));
    printf("File size is %d bytes\n",(int)file_size);
    
    //-Receive file data- 
    while (1)
    {
        if(!Queue_Is_Full(&queue))
        {
             //FD_ZERO(&readfds);
             //FD_SET(sockfd,&readfds);
             //select(sockfd,&readfds,NULL,NULL,&tv);
             numbytes = recv(sockfd, recvBuff, sizeof(recvBuff)-1,MSG_WAITALL);       
             if(numbytes == 0){
                 break;
             }
             pthread_mutex_lock (&mutex);
             Queue_Put(&queue,recvBuff);
             pthread_mutex_unlock(&mutex);
             bzero(recvBuff, sizeof(recvBuff));
             //get_flag = false;
            //select(0,NULL,NULL,NULL,&tv);
            
            //clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&ts,NULL);

         }
         //numbytes = fwrite(recvBuff, sizeof(unsigned char), numbytes, fp2);
         //printf(" ");

         if(rw_error_flag){
             //Release 
            close(sockfd);
            pthread_exit(NULL);
         }
    } 

    pthread_mutex_lock (&mutex);
    read_finish = true;
    pthread_mutex_unlock(&mutex);
    printf("Read is finish!!\n");
    //Release 
    close(sockfd);
    pthread_exit(NULL);
}
void *Write_thread(void* buf)
{
    FILE *fp2 = NULL;
    size_t output_fd;    // Input and output file descriptors 
    unsigned char w_buffer[BUF_SIZE];
    int load_size = BUF_SIZE;
    fp2 = fopen(wfd_name, "wb");
    if(NULL == fp2)
    {
        printf("\n write file fopen() Error!!!\n");
        rw_error_flag = true;
        pthread_exit(NULL);
    }
    //write file   
    while(!read_finish || !Queue_Is_Empty(&queue))
    {
        if(!Queue_Is_Empty(&queue))
        {
            pthread_mutex_lock (&mutex);
            if(file_size<BUF_SIZE*sizeof(unsigned char))   //prevent to write more data on file
            {
                load_size = file_size;
                unsigned char w_tail_buffer[load_size];
                Queue_Get(&queue, w_tail_buffer,load_size);
                output_fd = fwrite(w_tail_buffer,load_size,1,fp2);
            }
            else
            {
	        Queue_Get(&queue, w_buffer,load_size);
                file_size -= sizeof(unsigned char)*BUF_SIZE;
                output_fd = fwrite(w_buffer,BUF_SIZE,1,fp2);
            }
            pthread_mutex_unlock(&mutex);
	}
        if(rw_error_flag){
             //Release 
            fclose(fp2);
            pthread_exit(NULL);
         }
    }
    //close file
    fclose(fp2);
    pthread_exit(NULL);
}

//socket mode
bool server_mode(unsigned char *file_name)
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 
    bool stc_buffer_full_flat = true;
    unsigned char buf[2049];  
    time_t ticks; 
    int numbytes = 0;
    long file_size = 0;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    //int test = 0;
    bool client_ok = false;
    //printf("file name is : %s\n",file_name);
    //-file open-
    FILE *fp = NULL;
    fp = fopen(file_name, "rb");
    if(NULL == fp)
    {
        printf("\n read file fopen() Error!!!\n");
        return 0;
    }
    //-get file size-
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    //test = (int)file_size;
    printf("File size is ( %d bytes)\n",(int)file_size);
    //-socket start-
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    

    int status = 0 ,on = 1;
    status = setsockopt(listenfd, SOL_SOCKET,SO_REUSEADDR,(const char *) &on, sizeof(on));

    if (-1 == status)
    {
        perror("setsockopt(...,SO_REUSEADDR,...)");
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buf, '0', sizeof(buf)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 
 
    //-Connect-
    if ( -1 == (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)))
    {
          perror("accept"); 
          return 1;
    }
    printf("Client is connect\n");

    write(connfd, &file_size, sizeof(file_size));  //send file size
    //-read file and send to buffer-
    while(!feof(fp))
    {
        numbytes = fread(buf, sizeof(unsigned char), sizeof(buf)-1, fp);
        numbytes = write(connfd, buf, numbytes);
    }

    fclose(fp);
    close(connfd);
    close(listenfd);
    
    //closesocket(listenfd);
    return 1;

}

bool client_mode()
{
    clock_t start_time, end_time;
    float total_time = 0;
    pthread_t r_thread = -1,w_thread = -1;
    int rc = 0,rc2 = 0;
    
    Queue_Init(&queue);    
    //printf("file name: ");
    //scanf("%s", rfd_name);  //insert filename

    start_time = clock(); /* mircosecond */
    pthread_mutex_init (&mutex,NULL);
    rc = pthread_create(&r_thread, NULL, Reader_thread, NULL);
    if (rc)
    {
      printf("Read ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
    rc2 = pthread_create(&w_thread, NULL, Write_thread, NULL);
    if (rc2)
    {
      printf("Write ERROR; return code from pthread_create() is %d\n", rc2);
      exit(-1);  
    }
    //wait
    pthread_join(r_thread,NULL);
    pthread_join(w_thread,NULL);
    //Release
    Queue_Destroy(&queue);
 
    end_time = clock();
    total_time = (float)(end_time - start_time)/CLOCKS_PER_SEC;
    printf("Time : %f sec \n", total_time);
    return true;
}


int main(int argc, char* argv[]) 
{
    if(1==argc)
    {
        printf("ERROR : Choose scoket mode\n");
        return 0;
    }
    else if( !strcmp(argv[1],"server") )
    {
        //printf("I'm here\n");
        if(2 == argc)
        {
            printf("ERROR : Enter file name\n");
            return 0;
        }
        //printf("argv[3] is : %s\n",argv[2]);
        server_mode(argv[2]);
    }
    else if( !strcmp(argv[1],"client") )
    {
        //printf("client!!\n");
        client_mode();
    }

    return (EXIT_SUCCESS);
}
