#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>
using namespace std;

vector<int> array;
int sum =0;
pthread_mutext_t sum_lock = PTHREAD_MUTEX_INITIALIZER;

struct task
{
  int start;  //inclusive
  int end;  //exclusive
};

queue<task> workqueue;
pthread_mutext_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

void* do_task(void* args){
   bool keepgoing =true;
   while(keepgoing){
      
      task my_task;
      pthread_mutex_lock(&queue_lock);
      if (work.size() >0){
         my_task = workqueue.front();
         workqueue.pop();
      }
      else{
         keepgoing = false;
      }
      
      pthread_mutex_unlock(&queue_lock);
      if(keepgoing){
         int local_sum =0;
         for(int i=my_task)
      }
   }
}


int main(int argc, char* argv[])
{
   int asize = atoi(argv[1]);
   int num_threads = atoi(argv[2]);
   int task_size = atoi(argv[3]);
   
   for (int i=0; i<asize; i++){
      array[i] =i;
      sum_actual +=array[i];
   }
   
   for (int i=0; i<asize; i += task_size){
      task newtask;
      newtask.start = i;
      
      if(i + task_size < asize)
         newtask.end = i +task_size;
      else
         newtask.end = asize;
         
      workqueue.push(newtask);
   }
   
   vector<pthread> ids;
   ids.resize(num_threads);
   for(int i=0; i< num_threads;i++){
      pthread_join(&ids[i],NULL);
   }
   
   cout << "expected sum: " << sum_actual << endl;
   cout << "actual sum: " << sum << endl
   return 0;
}
