#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>
#include <stdlib.h>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>

//returns the number of seconds between two timevals
double tdiff(struct timeval* t1, struct timeval*  t0)
{
  return (t1->tv_sec - t0->tv_sec) + (t1->tv_usec - t0->tv_usec)/1e6;
}

// global data
int* data;
int sum=0;
int sum_actual=0;
pthread_mutex_t sum_lock = PTHREAD_MUTEX_INITIALIZER;
int array_size;//tanis suggested this Global

// break a task into two if it is larger than  this size
int task_threshold=10000;
/*now we want to break it up with respect to a branching factor.*/
//TO DO as step 2:  set up size based upon the branching factor logic..this is a param that gets passed into main()

struct task
{
    int start;// inclusive
    int end;// exclusive
    int size;
    int num_branches;
};


std::queue<task> workqueue;
pthread_mutex_t queue_lock  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

int count = 1;// this will increment when a thread returns a value that will trigger the counter to increment
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;// this will help lock the global count variable when updating it.
pthread_cond_t count_threshold_cv = PTHREAD_COND_INITIALIZER;// this is the trigger condition


void* do_tasks(void* args)
{

  //do branching and mod here 
  
  while (1/*count < size*/)
    {
        bool has_task  = false;
        task my_task;
        
        while (! has_task)
        {
             pthread_mutex_lock(&queue_lock);

            if (workqueue.size() > 0)
            {
                my_task = workqueue.front();
                workqueue.pop();
                has_task = true; 
            }
            else
            {
	      /* Note that a call to pthread_cond_wait() automatically and atomically unlocks the associated mutex variable so that it can be used by Thread-B.*/
                pthread_cond_wait(&queue_not_empty, &queue_lock);
            }

            pthread_mutex_unlock(&queue_lock);
        }
        int end = my_task.end;
        int start = my_task.start;

        // if the task is small enough, do the work
        if ((end - start) < task_threshold)
        {
            // do the work
            int local_sum=0;
	    int local_count=0;

            for (int i=my_task.start; i<my_task.end; i++)
            {
                local_sum += data[i];
		
            }
	    // local_count++;
	    
            pthread_mutex_lock(&sum_lock);
            sum += local_sum;
            pthread_mutex_unlock(&sum_lock);

	    pthread_mutex_lock(&count_lock);
	    count--;
	    if(count == 0)
	    {
	      // notify main that all threads are done.
	      pthread_cond_broadcast(&count_threshold_cv);
	      printf("the sum is complete and count reached:%d\n",count);
	    }
	    pthread_mutex_unlock(&count_lock);
	    // pthread_exit(NULL);
        }
        else
        {
            // dynamic partitioning
	  //
	  //	    printf("breaking in half: %d to %d\n", start, end);
            int mid = (end+start)/2;

            task t1 = { start, mid };
            task t2 = {mid, end };

            pthread_mutex_lock(&queue_lock);
            workqueue.push(t1);
            workqueue.push(t2);
            pthread_cond_broadcast(&queue_not_empty);
            pthread_mutex_unlock(&queue_lock);

	    //here is where we count the number of tasks that are created
	    pthread_mutex_lock(&count_lock);
	    count ++;
	    printf("count = %d\n",count);
	    pthread_mutex_unlock(&count_lock);
	    
        }
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    int asize = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    if (argc > 3)
    {
        task_threshold = atoi(argv[3]);
    }

    // data.resize(asize);
    data = (int*)malloc(asize*sizeof(int));


    for (int i=0; i<asize; i++)
    {
        data[i] = i;
        sum_actual += data[i];
    }


    // for (int i=0; i<asize; i+=task_size)
    // {
    //     task newtask;
    //     newtask.start = i;

    //     if (i  + task_size < asize)
    //     {
    //         newtask.end = i + task_size;
    //     }
    //     else
    //     {
    //         newtask.end = asize;
    //     }

    //     workqueue.push(newtask);
    // }

    task starting_task = { 0, asize };
    workqueue.push(starting_task);

    std::vector<pthread_t> ids; //declare a vector of type pthread_t 
    ids.resize(num_threads);// allocate the vector's size to be the num_threads

    struct timeval t1, t0;
    gettimeofday(&t0, NULL);

    for (int i=0; i<num_threads; i++)
    {
      // do we 
      pthread_create(&ids[i], NULL, do_tasks, NULL); // why is the last arg NULL?
    }

    // for (int i=0; i<num_threads; i++)
    // {
    //     pthread_join(ids[i], NULL);
    // }
    pthread_cond_wait(&count_threshold_cv, &count_lock);
    // check condition variable ????
    for(int i = 0; i < num_threads; i++)
    {
      pthread_cancel(ids[i]);
    }
    //  sleep(5);
    
    /*
    for (int i=0; i<num_threads; i++)
    {
        pthread_cancel(ids[i]);
    }
    }*/
    gettimeofday(&t1, NULL);

    std::cout << "# of threads: " << num_threads  << " " << "Wall time: " << tdiff(&t1,&t0) << std::endl;

    std::cout << "expected sum: " << sum_actual << std::endl;
    std::cout << "sum: " << sum << std::endl;

    free(data);
    return 0;
}
