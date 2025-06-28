#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include<unistd.h>  // Add this header for sleep()


using namespace std;


//semaphore to control sleep and wake up
sem_t sem_empty;
sem_t full;
queue<int> q;
pthread_mutex_t lock;


void init_semaphore()
{
	sem_init(&sem_empty,0,5);
	sem_init(&full,0,0);
	pthread_mutex_init(&lock,0);
	
}

void * ProducerFunc(void * arg)
{	
	printf("%s\n",(char*)arg);
	int i;
	for(i=1;i<=10;i++)
	{
		sem_wait(&sem_empty);
		sleep(1);
		
		pthread_mutex_lock(&lock);		
		sleep(1);	
		q.push(i);
		printf("producer produced item %d\n",i);
		
		pthread_mutex_unlock(&lock);
	
		sem_post(&full);
	}

	return NULL;
}

void * ConsumerFunc(void * arg)
{
	printf("%s\n",(char*)arg);
	int i;
	for(i=1;i<=10;i++)
	{	
		sem_wait(&full);
 		
		pthread_mutex_lock(&lock);
			
		sleep(2);
		int item = q.front();
		q.pop();
		printf("consumer consumed item %d\n",item);	

		pthread_mutex_unlock(&lock);
		
		sem_post(&sem_empty);
	}

	return NULL;
}





int main(void)
{	
	pthread_t thread1;
	pthread_t thread2;
	
	init_semaphore();
	
	char * message1 = "i am producer";
	char * message2 = "i am consumer";	
	
	pthread_create(&thread1,NULL,ProducerFunc,(void*)message1 );
	pthread_create(&thread2,NULL,ConsumerFunc,(void*)message2 );

	
	while(1);
	return 0;
}
