/*
  线程池
*/

#include <pthread.h> 
#include <stddef.h>  //size_t ,NULL
#include <stdlib.h>  //malloc
#include <stdio.h> //fprintf
#include <errno.h> 
#include <string.h> //strerror
typedef struct task{
  void  (*function)(void* arg); 
  void *arg;
  struct task* prev;
} task;

typedef struct taskqueue{
  pthread_mutex_t rw_lock;//整个成员的锁
  pthread_cond_t not_empty_cond;//非空通知.
  task* head;
  task* tail;
  volatile size_t task_num;
} taskqueue;


typedef struct thpool {
  pthread_t* threads; //线程id数组
  volatile size_t total_thread_num; //线程数量

  volatile size_t working_thread_num; //正在执行任务的线程数量
  taskqueue tasks; //任务列表
  pthread_mutex_t tplock;//用于修改working_thread_num
} thpool;


static volatile int thpool_keepalive;

#define log_debug(M, ...) {fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n",\
                                __FILE__,\
                                __LINE__,\
                                (errno == 0 ? "None" : strerror(errno)), ##__VA_ARGS__);}


static void* thread_loop(struct thpool* thpool_ptr);

thpool * thpool_init(size_t thread_num){
  thpool_keepalive = 1;

  if(thread_num < 1)
    return NULL;
  thpool *thpool_ptr = (thpool *) malloc(sizeof(struct thpool));
  if(thpool_ptr == NULL){
    log_debug("malloc");
    return NULL;
  }
  thpool_ptr->threads = (pthread_t*) malloc(thread_num* sizeof(pthread_t));
    if(thpool_ptr->threads == NULL){
    log_debug("malloc");
    return NULL;
  }
  pthread_mutex_init(&thpool_ptr->tplock,NULL);
  pthread_mutex_init(&thpool_ptr->tasks.rw_lock,NULL);
  pthread_cond_init(&thpool_ptr->tasks.not_empty_cond,NULL);
  // pthread_cond_init(&thpool_ptr->tpcond, NULL);

  thpool_ptr->total_thread_num = thread_num;
  thpool_ptr->working_thread_num = 0;

  thpool_ptr->tasks.task_num = 0;
  thpool_ptr->tasks.head =NULL;
  thpool_ptr->tasks.tail =NULL;

  for(size_t i =0; i<thread_num;++i){
    pthread_create(&thpool_ptr->threads[i],NULL,(void *)thread_loop,thpool_ptr);
    pthread_detach(thpool_ptr->threads[i]);
  }
  return thpool_ptr;
}


int thpool_add_task(thpool* thpool_ptr, void (*function)(void*), void* arg){
  task* task_ = (task*) malloc(sizeof(struct task));
  if(task_ == NULL){
    log_debug("malloc");
    return -1;
  }
  task_->function = function;
  task_->arg = arg;
  taskqueue * t = &thpool_ptr->tasks;
  pthread_mutex_lock(&t->rw_lock);
  if(t->task_num )
  {
    t->tail->prev = task_;
    t->tail = task_;
  }
  else
  {
    t->head = task_;
    t->tail = task_;
  }
  t->task_num++;
  pthread_mutex_unlock(&t->rw_lock);
  //立即通知
  pthread_cond_signal(&t->not_empty_cond);
  return 0;
}

static task * get_task(taskqueue *tasks){
  pthread_mutex_lock(&tasks->rw_lock);
  
	task* task_p = tasks->head;

	switch(tasks->task_num){

		case 0:
		  			break;
		case 1: 
					tasks->head = NULL;
					tasks->tail  = NULL;
					tasks->task_num = 0;
					break;
		default:
					tasks->head = task_p->prev;
					tasks->task_num--;
	}
  pthread_mutex_unlock(&tasks->rw_lock);
  return task_p;
}


static void* thread_loop(struct thpool* thpool_ptr){
  taskqueue * t = &thpool_ptr->tasks;
  while(thpool_keepalive){
    //等待有任务
    pthread_mutex_lock(&t->rw_lock);
    while(t->task_num<1){
      pthread_cond_wait(&t->not_empty_cond, &t->rw_lock);
    }
    //应该立即解锁. 加快其他线程进入取任务
    pthread_mutex_unlock(&t->rw_lock);
    if(thpool_keepalive){

      pthread_mutex_lock(&thpool_ptr->tplock);
			thpool_ptr->working_thread_num++;
			pthread_mutex_unlock(&thpool_ptr->tplock);

      task* task_ = get_task(t);
      if(task_){
        void (*function)(void*) = task_->function;
        void*  arg = task_->arg;
        function(arg);
        free(task_);
      }
      pthread_mutex_lock(&thpool_ptr->tplock);
			thpool_ptr->working_thread_num--;
			pthread_mutex_unlock(&thpool_ptr->tplock);
    }

  }
  pthread_mutex_lock(&thpool_ptr->tplock);
  thpool_ptr->total_thread_num--;
  pthread_mutex_unlock(&thpool_ptr->tplock);
  return NULL;
}