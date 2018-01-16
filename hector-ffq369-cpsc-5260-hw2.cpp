/*Author: Hector Suarez
* CPSC 5260
* HW2                   */
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
int num_branches=2;
/*now we want to break it up with respect to a branching factor.*/
//TO DO as step 2:  set up size based upon the branching factor logic..this is a param that gets passed into main()

struct task
{
    int start;// inclusive
    int end;// exclusive
};


std::queue<task> workqueue;
pthread_mutex_t queue_lock  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

int count = 1;// this will increment when a thread returns a value that will trigger the counter to increment
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;// this will help lock the global count variable when updating it.
pthread_cond_t count_threshold_cv = PTHREAD_COND_INITIALIZER;// this is the trigger condition

/*Worker Daemon that will be killed instead of joining.
* Each Daemon will check a cond var that will signal/broadcast 
* to the main that the overall task is complete. A global 'count'
* condition var is used to comunicate the latter to main.*/
void* do_tasks(void* args)
{ 
  while (1)
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
	   //   printf("the sum is complete and count reached:%d\n",count);
	    }
	    pthread_mutex_unlock(&count_lock);
	    // pthread_exit(NULL);
        }
        else
        {
	   int local_count = 0;
	   int branch_size = (end-start)/num_branches;
	   for (int b=0; b < (num_branches-1); b++)
	   {
	     task t = { start, (start + branch_size)};
	       pthread_mutex_lock(&queue_lock);
	       workqueue.push(t);
	       pthread_mutex_unlock(&queue_lock);
	       start += branch_size;
	       local_count ++;
	   }
	
	   task t = {start, end};
	   pthread_mutex_lock(&queue_lock);
	   workqueue.push(t);
	   pthread_cond_broadcast(&queue_not_empty);
	   pthread_mutex_unlock(&queue_lock);
	
            // dynamic partitioning
	  //	    printf("breaking in half: %d to %d\n", start, end);
	  /*int mid = (end+start)/2;

	    task t1 = { start, mid };
            task t2 = {mid, end };

            pthread_mutex_lock(&queue_lock);
            workqueue.push(t1);
            workqueue.push(t2);
            pthread_cond_broadcast(&queue_not_empty);
            pthread_mutex_unlock(&queue_lock);*/

	    //here is where we count the number of tasks that are created
	    pthread_mutex_lock(&count_lock);
	    count += local_count; //(branch_size -1); //because
	    
	    //  printf("count = %d\n",count);
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
    if(argc > 4)
    {
      num_branches = atoi(argv[4]);
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
      pthread_create(&ids[i], NULL, do_tasks, NULL);
    }
      
    pthread_cond_wait(&count_threshold_cv, &count_lock);
    for(int i = 0; i < num_threads; i++)
    {
      pthread_cancel(ids[i]);
    }
    gettimeofday(&t1, NULL);

    /*std::cout << "# of threads: " << num_threads << " \tBranch factor: " << num_branches <<"\t Threshold: " << task_threshold << "\t Wall time: " << tdiff(&t1,&t0) << std::endl;*/
    std::cout << num_threads << "\t" << task_threshold << "\t" << num_branches << "\t" << tdiff(&t1,&t0) << std::endl; 
    /* std::cout << "expected sum: " << sum_actual << std::endl;
       std::cout << "sum: " << sum << std::endl;*/
    free(data);
    return 0;
}//end main
