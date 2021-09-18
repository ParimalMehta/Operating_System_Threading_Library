#include "mythread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

int threadID = 0;

struct ThreadNode* mainThread = NULL;
struct ThreadNode* runningThread = NULL;

typedef struct ThreadNode{

   struct ThreadNode* next;
   ucontext_t* context;
   int id;
   int joinAll;
   int join;
   struct Queue* childThread;
   struct ThreadNode* parent;

}ThreadNode;

typedef struct Queue
{
    struct ThreadNode* frontNode;
    struct ThreadNode* rearNode;
    int count;

}Queue;

Queue* readyQueue = NULL;
Queue* blockedQueue = NULL;
ucontext_t Maincntx;

typedef struct Semaphore
{
    int initialValue;
    struct Queue* semaphoreQueue;

}Semaphore;



Queue* createNewQueue()
{

    Queue* q_variable = (Queue *)malloc(sizeof(Queue));
    q_variable->frontNode = NULL;
    q_variable->rearNode = NULL;

    q_variable->count = 0;

    return q_variable;

}

void enQueue(Queue* queue, ThreadNode* thread)
{
 
    if(queue->frontNode == NULL && queue->rearNode == NULL)
    {
        queue->frontNode = thread;
        queue->rearNode = thread;
       
    }
    else
    {
        queue->rearNode->next = thread;
        queue->rearNode = thread;
    }
    queue->count++;

}

ThreadNode* deQueue(Queue* queue)
{
    if(queue->frontNode == NULL && queue->rearNode == NULL)
    {
        printf("Queue empty");
        return NULL;
    }
    else
    {
        ThreadNode* temp = queue->frontNode;
        queue->frontNode = queue->frontNode->next;
        queue->count--;
        if(queue->count ==0)
            queue->rearNode = NULL;
        return temp;
    }
}

int countQueue(Queue* queue)
{
    int count=0;
    if(queue->frontNode==NULL)
    {
        return 0;
    }    
    else
    {
        ThreadNode* temp = queue->frontNode;
        while(temp!=NULL)
        {
            count++;
            temp = temp->next;
        }
        return count;
    }
}

void displayQueue(Queue* queue)
{   
    if(queue->frontNode==NULL)
    {
       printf("\n Queue empty");
    }    
    else
    {
        printf("\n Queue elements are => ");
        ThreadNode* temp = queue->frontNode;
        while(temp!=NULL)
        {
            printf(" \n id => %d",temp->id);
            temp = temp->next;
        }
       
    }
    printf("\n");
}

void removeFromQueue(Queue* queue,ThreadNode* thread)
{
    if(queue->frontNode==NULL && queue->rearNode == NULL)
    {
        return;
    }
    else if(queue->frontNode==thread)
    {
        queue->frontNode = queue->frontNode->next;
        return;
    }
    else
    {
        ThreadNode* current = queue->frontNode;
        ThreadNode* current_prev = queue->frontNode;
        while (current!=NULL)
        {
            current_prev = current;
            current = current->next;
            if(current==thread)
            {
                if(current->next == NULL)
                {
                    current_prev->next=NULL;
                    queue->rearNode = current_prev;
                }
                else
                {
                    current_prev->next = current->next;
                }
                return;
            }
        }
        
    }

}

void queueTraverseDeleteNode(Queue* queue, ThreadNode* threadToDelete)
{
    ThreadNode* t1 = queue->frontNode;
    while(t1!=NULL)
    {
        if(t1->parent == threadToDelete)
        {
            t1->parent = NULL;
        }
        t1 = t1->next;
    }
}


void MyThreadExit()
{
    ThreadNode* currentThread = runningThread;   
   
  

    if(currentThread->parent!=NULL)
    {               

        if((currentThread->join==1) || ((currentThread -> parent->joinAll==1) && ((currentThread->parent->childThread->frontNode==currentThread ))))
        {  
            removeFromQueue(blockedQueue,currentThread->parent);
            currentThread->parent->next=NULL;           
            enQueue(readyQueue,currentThread->parent);
                       
        }
        removeFromQueue(currentThread->parent->childThread,currentThread);
        currentThread->parent=NULL;
        currentThread->next=NULL;
    }

    if(countQueue(currentThread->childThread)>0)
    {
         ThreadNode* temp = currentThread->childThread->frontNode;
         temp = currentThread->childThread->frontNode;
            temp->parent = NULL;

        queueTraverseDeleteNode(readyQueue,currentThread);
        queueTraverseDeleteNode(blockedQueue,currentThread);
         
    }
    currentThread->childThread = NULL;    
    free(currentThread->context);
    free(currentThread);
   
    if(countQueue(readyQueue)!=0)
    {
       
        runningThread = deQueue(readyQueue);
        runningThread->next = NULL;
      
        if(setcontext(runningThread->context)==-1)
        {
            printf("\n Setcontext error.");
        }
       
    }
    else{
       // printf(" \n Number of threads => %d", threadID);
         
        if (setcontext(&Maincntx) == -1) {
			printf("\n Setcontext error.");
		}
    }
}



void MyThreadYield()
{
  
    if(countQueue(readyQueue)!=0)
    {            

        // if(runningThread->parent == NULL)
        // {
        //     printf("\n Current thread ID => %d , with parent NULL, number of childs => %d ", runningThread->id, countQueue(runningThread->childThread));
        // }
        // else
        // {
        //      printf("\n Current thread ID => %d , with parent ID => %d ", runningThread->id, runningThread->parent->id);
        // }
        ThreadNode* nextThread = deQueue(readyQueue);
        nextThread->next = NULL;
        ThreadNode* currentThread = runningThread;
        enQueue(readyQueue,currentThread);



        runningThread = nextThread;
        if(swapcontext(currentThread->context,nextThread->context)==-1)
        {
            printf("Error in swapcontext");
        }
       
    }
    else
    {
        printf("Ready queue empty.");
    }
}

int MyThreadJoin(MyThread thread)
{
    ThreadNode* child = (ThreadNode* ) thread;
    ThreadNode* currentThread = runningThread;

    if(child->parent == currentThread)
    {
        child->join = 1;
        ThreadNode * nextThread = deQueue(readyQueue);
        nextThread->next = NULL;
        enQueue(blockedQueue, runningThread);
        runningThread = nextThread;
        if(swapcontext(currentThread->context,nextThread->context)==-1)
        {
            printf("Error in swapcontext");
        }
        return 0;
    }
    return -1;
}

void MyThreadJoinAll()
{
    if(countQueue(runningThread->childThread)!=0)
    {
        ThreadNode* currentThread = runningThread;
        currentThread->joinAll = 1;
        ThreadNode * nextThread = deQueue(readyQueue);
        nextThread->next = NULL;
            runningThread = nextThread;
       
            enQueue(blockedQueue, currentThread);

            if(swapcontext(currentThread->context,nextThread->context)==-1)
            {
                printf("Error in swapcontext");
            }        
    }

}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
    ThreadNode* newThread = (ThreadNode *)malloc(sizeof(ThreadNode));
    newThread->id = ++threadID;

  
    ucontext_t * newContext = malloc(sizeof(ucontext_t));
    if (getcontext(newContext) == -1) {
		printf("getcontext error");
	}
    newContext->uc_link = 0;
	newContext->uc_stack.ss_sp = malloc(16384);
	newContext->uc_stack.ss_size = 16384;
	newContext->uc_stack.ss_flags = 0;

    makecontext(newContext, (void*) start_funct, 1, args);

    newThread->context = newContext;
    newThread->parent = runningThread;
    newThread->childThread = createNewQueue();
    newThread->join = 0;
    newThread->joinAll = 0;
    newThread->next = NULL;

    enQueue(runningThread->childThread,newThread);
    enQueue(readyQueue,newThread);

    return (MyThread) newThread;
}


void MyThreadInit(void(*start_funct)(void *), void *args)
{
    readyQueue = createNewQueue();
    blockedQueue = createNewQueue();
   
    struct ThreadNode* newThread = (ThreadNode* ) malloc(sizeof(ThreadNode));

    ucontext_t * newContext = malloc(sizeof(ucontext_t));

	if (getcontext(newContext) == -1) {
		printf("getcontext error");
	}

	newContext->uc_link = 0;
	newContext->uc_stack.ss_sp = malloc(16384);
	newContext->uc_stack.ss_size = 16384;
	newContext->uc_stack.ss_flags = 0;

	makecontext(newContext, (void*) start_funct, 1, args);

    newThread->context = newContext;
    newThread->parent = NULL;
    newThread->childThread = createNewQueue();
    newThread->id = ++threadID;
    newThread->join = 0;
    newThread->joinAll = 0;
    newThread->next = NULL;

    runningThread = newThread;


    if(swapcontext(&Maincntx,newThread->context)==-1)
    {
        printf("Error in swapcontext");
    }
    return;

}

MySemaphore MySemaphoreInit(int initialValue)
{
    struct Semaphore* newSemaphore = (Semaphore* )malloc(sizeof(Semaphore));
    newSemaphore->initialValue=initialValue;
    newSemaphore->semaphoreQueue=createNewQueue();
    return (MySemaphore) newSemaphore;
}

void MySemaphoreSignal(MySemaphore sem)
{
    Semaphore* semaPhoreToProcess = (Semaphore* ) sem;
    semaPhoreToProcess->initialValue = semaPhoreToProcess ->initialValue + 1;
    if(semaPhoreToProcess->initialValue <= 0)
    {
        enQueue(readyQueue, deQueue(semaPhoreToProcess->semaphoreQueue));
    }
}

void MySemaphoreWait(MySemaphore sem)
{
    Semaphore* semaPhoreToProcess = (Semaphore* ) sem;
    semaPhoreToProcess->initialValue = semaPhoreToProcess ->initialValue - 1;
    if(semaPhoreToProcess->initialValue < 0)
    {
        ThreadNode* nextThread = deQueue(readyQueue);
        ThreadNode* currentThread = runningThread;

        enQueue(semaPhoreToProcess->semaphoreQueue,currentThread);
       
        runningThread = nextThread;
        if(swapcontext(currentThread->context,nextThread->context)==-1)
        {
            printf("Error in swapcontext");
        }
        
    }
}

int MySemaphoreDestroy(MySemaphore sem)
{
    Semaphore* semaPhoreToProcess = (Semaphore* ) sem;
    if(countQueue(semaPhoreToProcess -> semaphoreQueue) == 0)
    {
        semaPhoreToProcess->semaphoreQueue = NULL;
        free(semaPhoreToProcess);
        return 0;
    }
    else
    {
        return -1;
    }
}


