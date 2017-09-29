#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>  
#include <ctype.h>

unsigned char file_name[] = "qnap_security.txt";
#define MAX_READ_SIZE 100

typedef struct link_list_point
{
    struct link_list_point *next;
    struct link_list_point *pre;
    unsigned char *word;
} Point;

Point *create_point(int word_num,unsigned char *word)
{
    Point *new_point;
    new_point = (Point *) malloc( sizeof(Point) );
    new_point->word = (unsigned char *)malloc( word_num);
    memcpy(new_point->word,word,word_num);
    new_point->next = NULL;
    new_point->pre = NULL;
    return new_point;
}

Point *Add_point(Point *current,int word_num,unsigned char *word)
{
    Point *next_point = create_point(word_num,word);
    current->next = next_point;
    next_point->pre = current;
    return next_point;
}

int main(int argc, char* argv[]) 
{
    
    //-file open and read
    FILE *fp = NULL;
    long file_size = 0;
    fp = fopen(file_name,"r");
    if(NULL == fp)
    {
        printf("\n Read file fopen() error!\n");
        return 0;
    }

    //-use fread-
    /*fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);*/
    //-get file size-
    struct stat statbuff;  
    if(stat(file_name, &statbuff) < 0){  
        return file_size;  
    }else{  
        file_size = statbuff.st_size;  
        printf("file size: %lu bytes\n",file_size);
    }  

    int read_bytes = 0,left_file_size = file_size;
    unsigned char *str = (unsigned char *)malloc(MAX_READ_SIZE);
    
    bool first_flag = true,word_n_end_flag = false;
    Point *start = NULL, *end = NULL;
    Point *current_point = NULL;
    while(left_file_size!=0)
    {
        int temp_size = 0;
        if(left_file_size >= MAX_READ_SIZE)
        {
            temp_size = MAX_READ_SIZE;
            left_file_size -= MAX_READ_SIZE;
        }
        else
        {
            temp_size = left_file_size;
            left_file_size = 0;
        }
        
        read_bytes = fread(str,sizeof(unsigned char),temp_size,fp);
        printf("fread file size is %d bytes\n",read_bytes);
        //-find the longest word number-
        int temp_count = 0,i = 0,longest_word_num = 0;
        for(i = 0; i < temp_size; i++)
        {
            if( isalpha(str[i]) )
            {
                temp_count += 1;
                if(temp_count>longest_word_num)
                    longest_word_num = temp_count;
            
            }
            else
            {
                //printf("%d\n",longest_word_num);
                temp_count = 0;
            }

        }
        
        //-splite word-
        unsigned char *current_word = (unsigned char *)malloc(longest_word_num);
        //printf("here\n");
        int current_word_num = 0;
        //bool alpha_flag = false;
    
        i = 0;

        //printf("\n");
        while( i!= temp_size )
        {
            if( isalpha(str[i]) )
            {
                current_word[current_word_num] = str[i];
                current_word_num += 1;
                //word_n_end_flag = false;
                //printf("here\n");
            }
            else if(current_word_num!=0)
            {
                 //printf("here0\n");
                 //printf("\n%s\n",current_word);
                 if(first_flag)
                 {
                     //printf("here\n");
                     current_point = create_point(current_word_num,current_word);
                     //printf("here2\n");
                     start = current_point;
                     end = current_point;
                     first_flag = false;
                     //printf("%s ",current_point->word);
                     
                 }
                 else if( !word_n_end_flag )
                 {
                     current_point->word = ( unsigned char *)realloc(current_point->word, (strlen(current_point->word)+current_word_num));
                     unsigned char *word_tail_addr = current_point->word + strlen(current_point->word)*sizeof(unsigned char);
                     memcpy( word_tail_addr, current_word, current_word_num );
                 }
                 else
                 {
                     end = Add_point(current_point,current_word_num ,current_word);
                     current_point = current_point->next;
                     
                     //printf("%s ",current_point->pre->word);
                 }
                 //continued_flag = true;
                 word_n_end_flag = true;
                 memset(current_word,0x0,current_word_num);
                 current_word_num = 0;
                 //cut_flag = false;
                 //alpha_flag = true;
            }
            else
            {
                 word_n_end_flag = true;
            }
            i += 1;
        }
        
        
        if(current_word_num!=0 && !word_n_end_flag /*&& !alpha_flag*/)
        {
            current_point->word = ( unsigned char *)realloc(current_point->word, (strlen(current_point->word)+current_word_num));
            unsigned char *word_tail_addr = current_point->word + strlen(current_point->word)*sizeof(unsigned char);
            memcpy( word_tail_addr, current_word, current_word_num );
            //cut_flag = true;
            word_n_end_flag = false;
        }
        else if(current_word_num!=0 )
        {
            if(first_flag)
            {
                current_point = create_point(current_word_num,current_word);
                //printf("here2\n");
                start = current_point;
                end = current_point;
                first_flag = false;
                word_n_end_flag = false;
            }
            else
            {
                end = Add_point(current_point,current_word_num ,current_word);
                current_point = current_point->next;
                //cut_flag = true;
                word_n_end_flag = false;
            }
            
        }

        //printf("current_point word:%s %d\n",current_point->word,current_word_num);
        memset(str,0x0,temp_size);
        free(current_word);
        current_word = NULL;
    }//
        
    
    
    
    
    //-print result-
    printf("STRING : \n");
    current_point = start;
    int count = 0;
    while(NULL != current_point)
    {
        printf("%s ",current_point->word);
        current_point = current_point->next;
        count++;
    }
    printf("\n");
    printf("number of word: %d\n",count);
    printf("\ninverse string : \n");
    current_point = end;
    while(NULL != current_point)
    {
        printf("%s ",current_point->word);
        current_point = current_point->pre;
    }
    printf("\n");
    Point *temp = NULL; 
    current_point = start;
    
    while(NULL != current_point)
    {
        temp = current_point;
        free(current_point->word);
        current_point = current_point->next;
        free(temp);
    }
    //free(current_point);
    
    free(str);
    str = NULL;


   //printf("long size is %lu\n",sizeof(long));
    //printf("file size: %lu",(unsigned long)filelength(fp));
    //unsigned long filesize = -1; 
    
    fclose(fp);
    return 0;
}
