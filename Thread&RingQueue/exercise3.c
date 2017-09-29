#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <memory.h>

#define QUEUE_SIZE 128
#define BUF_SIZE 1024

//char buffer[BUF_SIZE];      /* Character buffer */
pthread_mutex_t mutex;
bool read_finish = false;
unsigned char rfd_name[80];
unsigned char wfd_name[] = "test.jpg";
long file_size = 0;
//ring buffer
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

//call from pthread
void *Reader_thread(void* buf)
{
    FILE *fp = NULL;
    size_t input_fd;    // Input and output file descriptors 
    unsigned char r_buffer[BUF_SIZE];
    unsigned char w_buffer[BUF_SIZE];
    fp = fopen(rfd_name, "rb");
    if(NULL == fp)
    {
        printf("\n read file fopen() Error!!!\n");
        pthread_exit(NULL);
    }
    //file size
    pthread_mutex_lock (&mutex);
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    pthread_mutex_unlock(&mutex);
    //read file  
    while(!feof(fp))
    {  
        if(!Queue_Is_Full(&queue))
        {
            input_fd = fread(r_buffer,BUF_SIZE,1,fp);
            pthread_mutex_lock (&mutex);
            Queue_Put(&queue,r_buffer);
            pthread_mutex_unlock(&mutex);
        }
    }
    pthread_mutex_lock (&mutex);
    read_finish = true;
    pthread_mutex_unlock(&mutex);
    printf("Read is finish!!\n");
    //close file
    fclose(fp);

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
        
    }
    //close file
    fclose(fp2);
    pthread_exit(NULL);
}




int main(int argc, char* argv[]) 
{
    clock_t start_time, end_time;
    float total_time = 0;
    pthread_t r_thread = -1,w_thread = -1;
    int rc = 0,rc2 = 0;
    
    Queue_Init(&queue);    
    printf("file name: ");
    scanf("%s", rfd_name);  //insert filename

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
    return (EXIT_SUCCESS);
}
