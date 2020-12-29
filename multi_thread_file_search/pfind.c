#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>/*for opendir*/
#include <signal.h> /*for sigaction*/

// ----------------------------- STRUCTURE DEFENITIONS --------------------------------- //
struct Qnode{
    char* dir;
    struct Qnode *next;
};
typedef struct Qnode Qnode;
struct Queue{
    Qnode *first, *last;
    int full;
};
typedef struct Queue Queue;
struct Thread_data
{
    Queue* queue;
    char* term;
    int match_file_counter;
};
typedef struct Thread_data Thread_data;
// ----------------------------- END OF STRUCTURES ------------------------------------ //
// ----------------------------- VARS DEFINETIONS-------------------------------------- //
pthread_mutex_t lock;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int thread_counter;
int *count;
int error_threads;
int amnt_threads = 0;
Queue* queue_ptr;
pthread_t* thread_array;
int flag = 1;
// ----------------------------- END OF VARS ------------------------------------------ //
//  --------------------------- FUNCTION DEFINITIONS----------------------------------- //
void cancel_threads()
{
    for (size_t i = 0; i < amnt_threads; ++i)
    {
        if (pthread_self() != thread_array[i])
            pthread_cancel(thread_array[i]);
    }
}
void done_working(int signal)
{
    cancel_threads();
    pthread_mutex_destroy(&lock);
    printf("Done searching, found %d files\n", *count);
    (error_threads == amnt_threads) ?  exit(1) :  exit(0);
}
void sigint_handler(int signal)
{
    cancel_threads();
    pthread_mutex_destroy(&lock);
    printf("Search stopped, found %d files\n", *count);
    exit(0);
}
Queue* create_queue()
{
    Queue *queue = (Queue*)malloc(sizeof(Queue)); //TODO malloc works??
    queue->first = queue->last = NULL;
    queue->full = 0;
    return queue;
}
Qnode* create_node(char *string)
{
    Qnode* new_node = (Qnode*)malloc(sizeof(Qnode)); //TODO malloc works??
    new_node->dir = (char*)malloc(strlen(string) + 1); //TODO malloc works??
    strcpy(new_node->dir, string);
    new_node->next = NULL;
    return new_node;
}
void delete_node(Qnode* node)
{
    free(node->dir);
    free(node);
}
int queue_empty(Queue* queue)
{
    return !(queue->full);
}
void free_queue(Queue* queue)
{
    Qnode* temp;
    if(!queue_empty(queue))
    {
        while(queue->first)
        {
            temp = queue->first;
            delete_node(temp);
        }
    }
}

char* dequeue(Queue* queue)
{
    pthread_mutex_lock(&lock);
    while(queue_empty(queue))
    {
        pthread_cond_wait(&cond, &lock);
    }
    __sync_fetch_and_add(&thread_counter, 1);
    Qnode* tmp_node;
    char* tmp_dir = (char*)malloc(strlen(queue->first->dir) + 1); //TODO malloc works??
    strcpy(tmp_dir, queue->first->dir);
    tmp_node = queue->first;
    queue->first = queue->first->next;
    delete_node(tmp_node);
    queue->full--;
    pthread_mutex_unlock(&lock);
    return tmp_dir;
}
void enqueue(Queue* queue, char* directory)
{
    pthread_mutex_lock(&lock);
    Qnode* new_node = create_node(directory);
    if(queue_empty(queue))
    {
        queue->first = new_node;
        queue->last = new_node;
    }
    queue->last->next = new_node;
    queue->last = new_node;
    queue->full++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}
char contains_term(char* term, char* dir)
{
    int term_len, dir_len, term_holder = 0;
    term_len = strlen(term);
    dir_len = strlen(dir);
    if(term_len > dir_len)
    {
        return 0;
    }
    for(int i = 0; i < dir_len; i++)
    {
        if(term[term_holder] == dir[i])
        {
            term_holder++;
        }
        else if(term_holder)
        {
            term_holder = 0;
        }
        if(term_holder == term_len)
        {
            return 1;
        }
    }
    return 0;
}
char file_is_dir(char* path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}
void itarate_over_file(Thread_data *thread_data ,DIR* dp)
{
    struct dirent *entry = NULL;
    while((entry = readdir(dp)))
    {
        if(*(entry->d_name) != '.')
        {
            if(file_is_dir(entry->d_name))
            {
                enqueue(thread_data->queue, entry->d_name);
            }
            else
            {
                if(contains_term(thread_data->term ,entry->d_name))
                {
                    printf("%s\n", entry->d_name);
                    __sync_fetch_and_add(&(thread_data->match_file_counter),1);
                }
            }
        }
    }
}
int need_to_exit()
{
    if(!thread_counter && queue_empty(queue_ptr))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


void thread_flow(Thread_data* thread_data)
{
    DIR *dp = NULL;
    while(1)
    {
        char* dir = dequeue(thread_data->queue);
        dp = opendir(dir);
        if(dp)
        {
            flag = 0;
            itarate_over_file(thread_data, dp);
            closedir(dp);
            __sync_fetch_and_sub(&thread_counter, 1);
            if(need_to_exit())
            {
                raise (SIGUSR1);
            }
        }
        else
        {
            perror("ERROR: could not open file\n");
            __sync_fetch_and_add(&error_threads, 1);
            __sync_fetch_and_add(&thread_counter, 1);
               
            if(need_to_exit() || flag)
            {
                raise (SIGUSR1);
            }  
        }
    }
}
//  ----------------------------- END OF FUNCTIONS----------------------------------- //
int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        perror("Error : number of arguments is not valid \n");
        return 1;
    }

    struct sigaction cancel_sa;
    cancel_sa.sa_handler = done_working;
    sigaction(SIGUSR1, &cancel_sa, NULL);
    struct sigaction sigint_sa;
    sigint_sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sigint_sa, NULL);
    
    Queue* queue = create_queue(); // holds directories
    queue_ptr = queue;
    
    Thread_data thread_data;
    thread_data.queue = queue;
    thread_data.term = (char*)malloc(strlen(argv[2]) + 1);
    strcpy(thread_data.term, argv[2]);
    thread_data.match_file_counter = 0;
    
    count = &thread_data.match_file_counter;
    
    amnt_threads = atoi(argv[3]);
    enqueue(queue, argv[1]);
    thread_array = (pthread_t*)malloc(amnt_threads*sizeof(pthread_t));
    
    for(int i = 0; i < atoi(argv[3]); i++)
    {
        pthread_create(&thread_array[i], NULL, (void* (*)(void*))thread_flow, &thread_data);
        
    }
    for (size_t i = 0; i < amnt_threads; ++i)
    {
        pthread_join(thread_array[i], NULL);
    }
    /*create N searching threads // TODO:argv[3] - each thread removesdirs from Q & searches name
    exit():
        if error in thread - print stderr and exit thread - dont exit program!
        1. no more dirs in Q and all threads are idle - return 0
        2. all searching threads died - do to an ERROR - return 1
        3. SIGINT -thread exit gracefully - pthread_cancel(),pthread_testcancel() return 0
        before exiting:
            if sigint "Search stopped, found %d files\n"
            else: "Done searching, found %d files\n"
            no need to free!!!
    searching thread flow():
        1. deque the head dir, if empty - sleep untill non-empty
        2. itarate through each file in dir:
            if "." or ".." - ignore
            if file os dir - add to tail of Q
            if contains search term (case-sensitive)!! print path of file "path"\n
        3. when no more - go to 1
     Q- suppose a dir is inserted to an empty Q, k threads are sleeping one of k must
            thread is sleeping - only if was empty*/
    return 1;
}
