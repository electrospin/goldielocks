#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

//using namespace std;
double tdiff(struct timeval* t1, struct timeval* t0)
{
    return (t1->tv_sec - t0->tv_sec) + (t1->tv_usec - t0->tv_usec)/1e6;
}

//vector<int> data;
int* data;
int sum=0;
int sum_actual=0;
pthread_mutex_t sum_lock = PTHREAD_MUTEX_INITIALIZER;

struct task
{
    int start;                  // inclusive
    int end;                    // exclusive
};

std::queue<task> workqueue;
pthread_mutex_t queue_lock  = PTHREAD_MUTEX_INITIALIZER;

void* do_tasks(void* args)
{
    bool keepgoing  = true;

    while (keepgoing)
    {
        task my_task;

        pthread_mutex_lock(&queue_lock);

        if (workqueue.size() > 0)
        {
            my_task = workqueue.front();
            workqueue.pop();
        }
        else
        {
            keepgoing = false;
        }

        pthread_mutex_unlock(&queue_lock);

        if (keepgoing)
        {
            int local_sum=0;

            for (int i=my_task.start; i<my_task.end; i++)
            {
                local_sum += data[i];
            }


            pthread_mutex_lock(&sum_lock);
            sum += local_sum;
            pthread_mutex_unlock(&sum_lock);
        }
    }
    return NULL;
    
}


int main(int argc, char *argv[])
{
    int asize = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int task_size=atoi(argv[3]);



    // data.resize(asize);
    data = (int*)malloc(asize*sizeof(int));


    for (int i=0; i<asize; i++)
    {
        data[i] = i;
        sum_actual += data[i];
    }


    for (int i=0; i<asize; i+=task_size)
    {
        task newtask;
        newtask.start = i;

        if (i  + task_size < asize)
        {
            newtask.end = i + task_size;
        }
        else
        {
            newtask.end = asize;
        }

        workqueue.push(newtask);
    }

    std::vector<pthread_t> ids;
    ids.resize(num_threads);

    struct timeval t1, t0;
    gettimeofday(&t0, NULL);



    for (int i=0; i<num_threads; i++)
    {
        pthread_create(&ids[i], NULL, do_tasks, NULL);
    }

    for (int i=0; i<num_threads; i++)
    {
        pthread_join(ids[i], NULL);
    }

    gettimeofday(&t1, NULL);

    std::cout << num_threads  << ": " << tdiff(&t1,&t0) << std::endl;


    // cout << "expected sum: " << sum_actual << endl;
    // cout << "sum: " << sum << endl;

    free(data);
    return 0;
}
