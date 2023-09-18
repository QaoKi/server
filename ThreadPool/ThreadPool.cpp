#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
using namespace std;

#ifdef _WIN32
//在windows下编译
typedef unsigned long int pthread_t;	
typedef mutex pthread_mutex_t;
typedef condition_variable pthread_cond_t;

#endif


typedef struct {
	void* (*function)(void*);		//函数指针，回调函数
	void* arg;						//回调函数的参数	
}threadpool_task_t;

/*描述线程池相关信息*/
struct threadpool_t
{
	pthread_mutex_t lock;				//用于锁住本结构体
	pthread_mutex_t thread_counter;		//用于操作busy_thr_num时使用
	pthread_cond_t queue_full;		    //当任务队列满时，添加任务的线程阻塞，等待此条件变量
	pthread_cond_t queue_empty;			//任务队列为空时，阻塞线程

	pthread_t* threads;					//存放线程池中每个线程的tid。数组
	pthread_t  adjust_tid;				//存放管理者线程tid，用于监测当前线程池使用情况
	threadpool_task_t* task_queue;		//任务队列

	int min_thr_num;					//线程池最小线程数
	int max_thr_num;					//线程池最大线程数
	int live_thr_num;					//当前存活线程个数
	int busy_thr_num;					//忙状态线程个数
	int wait_exit_thr_num;				//要销毁的线程个数

	int queue_front;					//task_queue队头下标
	int queue_rear;						//task_queue队尾下标
	int queue_size;						//task_queue队中实际任务数
	int queue_max_size;					//task_queue队列中可容纳任务数上限

	int shutdown;						//标志位，线程池使用状态，true或false
};

/*
	线程池中各个工作线程
*/
void* threadpool_thread(void* arg)
{
	threadpool_t* pool = (threadpool_t*)arg;
	threadpool_task_t task;

	while (true)
	{
		//刚创建出线程，等待任务队列里有任务，否则阻塞等待任务队列里有任务后再唤醒接收任务
		//上锁
		pthread_mutex_lock(&(pool->lock));

		/*
		queue_size == 0 说明没有任务，调用 wait 阻塞在条件变量上，
		若有任务，跳过该while
		*/ 
		while ((pool->queue_size == 0) && (!pool->shutdown))
		{
			printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
			//阻塞在queue_not_empty，并对pool->lock解锁
			pthread_cond_wait(&(pool->queue_empty), &(pool->lock));
			//清除指定数目的空闲线程，如果要结束的线程个数大于0，结束线程
			if (pool->wait_exit_thr_num > 0)
			{
				pool->wait_exit_thr_num--;
				
				//如果线程池里线程个数大于最小值时可以结束当前线程
				if (pool->live_thr_num > pool->min_thr_num)
				{
					printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
					pool->live_thr_num--;
					pthread_mutex_unlock(&(pool->lock));
					//线程自行结束
					pthread_exit(NULL);	
				}
			}
		}

		//如果shutdown为true，要关闭线程池里的每个线程，自行退出处理
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->lock));
			printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
			//线程自行结束
			pthread_exit(NULL);
		}

		//从任务队列里获取任务，是一个出队操作
		//从队列头开始获取任务
		task.function = pool->task_queue[pool->queue_front].function;
		task.arg = pool->task_queue[pool->queue_front].arg;

		//出队，模拟环形队列,让queue_front指针后移
		pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
		pool->queue_size--;

		//通知可以有新的任务添加进来
		pthread_cond_broadcast(&(pool->queue_full));

		//任务取出后，立即将线程池锁释放
		pthread_mutex_unlock(&(pool->lock));

		//执行任务
		printf("thread 0x%s start working\n", (unsigned int)pthread_self());
		//要对忙线程数量操作，加锁
		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thr_num++;
		pthread_mutex_unlock(&(pool->thread_counter));
		//执行回调函数任务
		(*(task.function))(task.arg);

		//任务处理结束
		printf("thread 0x%x end working\n", (unsigned int)pthread_self());
		//处理掉一个任务，忙状态线程数-1
		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thr_num++;
		pthread_mutex_unlock(&(pool->thread_counter));
	}
	pthread_exit(NULL);
}

//管理者线程的工作
void* adjust_thread(void* arg)
{
	threadpool_t* pool = (threadpool_t*)arg;
	while (!pool->shutdown)
	{
		//每十秒检测一次
		sleep(10);
		pthread_mutex_lock(&(pool->lock));
		int queue_size = pool->queue_size;	//任务的数量
		int live_thr_num = pool->live_thr_num;	//存活的线程数
		pthread_mutex_unlock(&(pool->lock));

		pthread_mutex_lock(&(pool->thread_counter));
		int busy_thr_num = pool->busy_thr_num;	//忙状态的线程数量，这个变量有自己的锁
		pthread_mutex_unlock(&(pool->thread_counter));

		/*
		根据一定的算法，判断当线程数量不够时，创建新的线程
		threadpool_create的时候，是按最小线程数创建的线程
		如果当前任务数大于空闲的线程数，且存活的线程数小于最大线程个数时，增加
		*/
		if (queue_size > (live_thr_num - busy_thr_num) && live_thr_num < pool->max_thr_num)
		{
			pthread_mutex_lock(&(pool->lock));
			int add = 0;

			//一次增加任务数个线程
			for (int i = 0; i < pool->max_thr_num && add < queue_size 
							&& pool->live_thr_num < pool->max_thr_num ; i++)
			{
				if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i]))
				{
					pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool);
					add++;
					pool->live_thr_num++;
				}
			}
			pthread_mutex_unlock(&(pool->lock));
		}
		//销毁多余的空闲线程，算法：忙线程*2 小于 存活的线程数，并且存活的线程数大于最小线程数时

		if (busy_thr_num * 2 < live_thr_num && live_thr_num > pool->min_thr_num)
		{
			pthread_mutex_lock(&(pool->lock));
			pool->wait_exit_thr_num = live_thr_num - busy_thr_num * 2;
			pthread_mutex_unlock(&(pool->lock));
			//一次销毁空间 live_thr_num - busy_thr_num * 2
			for (int i = 0; i < live_thr_num - busy_thr_num * 2; i++)
			{
				//通知处在空闲状态的线程，他们会自行终止
				pthread_cond_signal(&(pool->queue_empty));
			}
		}
	}
	return NULL;
}

/*
	创建线程池
	min_thr_num：最小线程数
	max_thr_num：最大线程数
	queue_max_size：任务队列数量，和最大线程数应该一致
*/
threadpool_t* threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
	if (min_thr_num <= 0)
		min_thr_num = 4;
	if (max_thr_num > 100)
		max_thr_num = 100;
	if (queue_max_size < max_thr_num || queue_max_size > 100)
		queue_max_size = max_thr_num;
	//使用 do{ } while(0) 的方式，只执行一次，并且如果代码出错，可以随时跳出来
	threadpool_t* pool = NULL;

	do 
	{
		//为线程池分配空间
		if ((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL)
		{
			//分配失败
			printf("malloc threadpool fail");
			break;		//跳出do while
		}
		//初始化
		pool->min_thr_num = min_thr_num;
		pool->max_thr_num = max_thr_num;
		pool->busy_thr_num = 0;
		pool->live_thr_num = min_thr_num;	//活着的线程数，初值 = 最小线程数
		pool->queue_size = 0;
		pool->queue_max_size = queue_max_size;
		pool->queue_front = 0;
		pool->queue_rear = 0;
		pool->shutdown = false;				//不关闭线程池

		//根据最大线程上线数，给工作线程数组开辟空间，并清零
		pool->threads = (pthread_t*)malloc(sizeof(pthread_t)*max_thr_num);
		if (pool->threads == NULL)
		{
			printf("malloc threads fail");
			break;
		}
		memset(pool->threads, 0, sizeof(pthread_t)*max_thr_num);

		//队列开辟空间
		pool->task_queue = (threadpool_task_t*)malloc(sizeof(threadpool_task_t)*queue_max_size);
		if (pool->task_queue == NULL)
		{
			printf("malloc task_queue fail");
			break;
		}

		//初始化互斥锁、条件变量
		if (pthread_mutex_init(&(pool->lock), NULL) != 0
				|| pthread_mutex_init(&(pool->thread_counter), NULL) != 0
				|| pthread_cond_init(&(pool->queue_empty), NULL) != 0
				|| pthread_cond_init(&(pool->queue_full), NULL) != 0
			)
		{
			printf("init the lock or cond fail");
			break;
		}

		//启动 min_thr_num 个工作线程
		for (int i = 0; i < min_thr_num; i++)
		{
			pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool);
			printf("start thread 0x%x...\n", (unsigned int)pool->threads[i]);
		}
		//启动管理者线程
		pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void*)pool);
		
		return pool;
	} while (0);

	//前面代码调用失败时，释放pool存储空间
	threadpool_free(pool);
}

//向线程池中添加一个任务
int threadpool_add(threadpool_t* pool, void*(*function)(void*), void* arg)
{
	pthread_mutex_lock(&(pool->lock));

	//==为真，队列已经满，调wait阻塞
	while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown))
	{
		pthread_cond_wait(&(pool->queue_full), &(pool->lock));
	}
	if (pool->shutdown) {
		pthread_mutex_unlock(&(pool->lock));
	}
	//清空工作线程调用的回调函数的参数 arg
	if (pool->task_queue[pool->queue_rear].arg != NULL) {
		free(pool->task_queue[pool->queue_rear].arg);
		pool->task_queue[pool->queue_rear].arg = NULL;
	}
	//添加任务到任务队列里
	pool->task_queue[pool->queue_rear].function = function;
	pool->task_queue[pool->queue_rear].arg = arg;
	//队尾指针移动，模拟环形
	pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
	pool->queue_size++;

	//添加完任务后，队列不为空，唤醒线程池中等待处理任务的线程
	pthread_cond_signal(&(pool->queue_empty));
	pthread_mutex_unlock(&(pool->lock));

	return 0;
}

int threadpool_destroy(threadpool_t* pool)
{
	if (pool == NULL)
		return -1;

	pool->shutdown = true;
	//先销毁管理线程
	pthread_join(pool->adjust_tid, NULL);

	for (int i = 0; i < pool.live_thr_num; i++ + )
	{
		//通知所有的空闲线程
		pthread_cond_broadcast(&(pool->queue_empty));
	}
	for (int i = 0; i < pool->live_thr_num; i++)
	{
		pthread_join(pool->threads[i], NULL);
	}
	threadpool_free(pool);

	return 0;
}

int threadpool_free(threadpool_t* pool)
{
	if (pool == NULL)
		return -1;
	if (pool->task_queue)
		free(pool->task_queue);
	if (pool->threads)
	{
		free(pool->threads);
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_mutex_lock(&(pool->thread_counter));
		pthread_mutex_destroy(&(pool->thread_counter));
		pthread_cond_destroy(&(pool->queue_empty));
		pthread_cond_destroy(&(pool->queue_full));
	}

	free(pool);
	pool = NULL;
	return 0;
}

int is_thread_alive(pthread_t tid)
{
	//发0号信号，测试线程是否存活
	int kill_rc = pthread_kill(tid, 0);	
	if (kill_rc == ESRCH)
		return false;

	return true;
}

int main()
{


	system("pause");
	return 0;
}