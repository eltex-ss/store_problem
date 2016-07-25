#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>

#define BUYERS_COUNT 3
#define BUYERS_MIN_NEED 500
#define BUYERS_RANGE 120

#define STORES_COUNT 5
#define MAX_PRODUCT_COUNT 120

#define MESSAGE_LENGTH 80

struct Store {
  int             product_count;
  pthread_mutex_t mutex;
};


/*========================*/
/*                        */
/*  Global variables.     */
/*                        */

int buyers_num[BUYERS_COUNT]; /*  Need to determine which buyer. */ 

pthread_t       loader_thread;
pthread_mutex_t loader_mutex;

pthread_t       end_up_thread;
pthread_mutex_t end_up_mutex;

struct Store    stores[STORES_COUNT];
int             buyers[BUYERS_COUNT];

int buyers_done = 0;

int log_file_d;
/*========================*/
/*                        */
/*  Interface.            */
/*                        */
int   Min(int a, int b);
void  CleanUp(void);
void  WaitForEnd(void);
void  OpenLogFile(void);

/*  Stores. */
void  InitializeStores(void);
void  OpenStores(void);
int   GetProducts(void);

/*  Buyers. */
void InitializeBuyers(void);
void *BuyProduct(void *n);

/*  Loader. */
void LaunchLoader(void);
void *RestoreProduct(void *n);

/*========================*/
/*                        */
/*  Definitions.          */
/*                        */
int Min(int a, int b) {
  return a < b ? a : b;
}

void InitializeStores(void)
{
  int i;
  char message[MESSAGE_LENGTH];

  for (i = 0; i < STORES_COUNT; ++i) {
    pthread_mutex_init(&stores[i].mutex, NULL);
    stores[i].product_count = GetProducts();
    sprintf(message, "Store %d contain %d products\n", i,
            stores[i].product_count);
    if (write(log_file_d, message, strlen(message)) <= 0) {
      perror("Can't write in log file");
      exit(1);
    }
  }
}

void InitializeBuyers(void)
{
  int i;

  for (i = 0; i < BUYERS_COUNT; ++i) {
    buyers[i] = BUYERS_MIN_NEED + rand() % BUYERS_RANGE;
  }
}

int GetProducts(void)
{
  return rand() % MAX_PRODUCT_COUNT;
}

void *BuyProduct(void *n)
{
  int buyer_num = *((int *) n);
  int store_num;
  int product_sold;
  char message[MESSAGE_LENGTH];

  while (buyers[buyer_num] > 0) {
    store_num = rand() % STORES_COUNT;
    pthread_mutex_lock(&stores[store_num].mutex);

    sprintf(message, "Buyer %d enters the %d store\n", buyer_num, store_num);
    if (write(log_file_d, message, strlen(message)) <= 0) {
      perror("Can't write in log file");
      exit(-1);
    }
    product_sold = Min(stores[store_num].product_count, buyers[buyer_num]);
    stores[store_num].product_count -= product_sold;
    buyers[buyer_num] -= product_sold;
    
    sprintf(message, "Buyer %d leaves the %d store\n", buyer_num, store_num);
    if (write(log_file_d, message, strlen(message)) <= 0) {
      perror("Can't write in log file");
      exit(-1);
    }
    sprintf(message, "Product stayed: %d\n", stores[store_num].product_count);
    if (write(log_file_d, message, strlen(message)) <= 0) {
      perror("Can't write in log file");
      exit(-1);
    }
    if (stores[store_num].product_count > 0) {
      pthread_mutex_unlock(&stores[store_num].mutex);
    } else {
      pthread_mutex_unlock(&loader_mutex);
    }
  }

  ++buyers_done;
  if (buyers_done == BUYERS_COUNT) {
    pthread_mutex_unlock(&end_up_mutex);
    pthread_mutex_unlock(&end_up_mutex);
  }

  sprintf(message, "Successfull exit, thread %d\n", buyer_num);
  write(log_file_d, message, strlen(message));
  pthread_exit(NULL);
}

void *RestoreProduct(void *n)
{
  int i;
  char message[MESSAGE_LENGTH];

  sprintf(message, "Loader is waiting for job\n");
  if (write(log_file_d, message, strlen(message)) <= 0) {
    perror("Can't write in log file");
    exit(-1);
  } 
  pthread_mutex_lock(&loader_mutex);
  while (buyers_done != BUYERS_COUNT) { 
    for (i = 0; i < STORES_COUNT; ++i) { 
      if (stores[i].product_count == 0) {
        sprintf(message, "Loader restore product in %d store\n", i);
        if (write(log_file_d, message, strlen(message)) <= 0) {
          perror("Can't write in log file");
          exit(-1);
        } 
        
        stores[i].product_count = GetProducts();
        pthread_mutex_unlock(&stores[i].mutex);
      }
    }
    pthread_mutex_lock(&loader_mutex);
  }

  pthread_exit(NULL);
}

void OpenStores(void)
{
  int i;
  pthread_t buyers_thread[BUYERS_COUNT];
 
  for (i = 0; i < BUYERS_COUNT; ++i) {
    buyers_num[i] = i;
    pthread_create(&buyers_thread[i], NULL,
                   BuyProduct, (void *) &buyers_num[i]);
  }
}

void LaunchLoader(void)
{
  pthread_mutex_init(&loader_mutex, NULL);

  pthread_create(&loader_thread, NULL, RestoreProduct, (void *) NULL);
}

void CleanUp(void)
{
  int i;
  char message[] = "All buyers are served\n";

  pthread_mutex_destroy(&loader_mutex);
  pthread_mutex_destroy(&end_up_mutex);
  for (i = 0; i < STORES_COUNT; ++i) {
    pthread_mutex_destroy(&stores[i].mutex);
  }

  if (write(log_file_d, message, strlen(message)) <= 0) {
    perror("Can't write to log file");
    exit(1);
  }
  close(log_file_d);

  printf("All done!\n");

  pthread_exit(NULL);
}

void WaitForEnd(void)
{
  pthread_join(end_up_thread, NULL);

  pthread_mutex_unlock(&loader_mutex);
}

void OpenLogFile(void)
{
  if ((log_file_d = open("log_file.txt",
                         O_CREAT | O_TRUNC | O_WRONLY, 0666)) == -1) {
    perror("Can't create log file");
    exit(1);
  }
}

void *WWait(void *n)
{
  pthread_mutex_lock(&end_up_mutex);
  pthread_mutex_lock(&end_up_mutex);

  pthread_exit(0);
}



int main(void)
{ 
  OpenLogFile();
  InitializeStores();
  InitializeBuyers();

  pthread_mutex_init(&end_up_mutex, NULL);
  pthread_create(&end_up_thread, NULL, WWait, (void *) NULL);

  LaunchLoader();
  OpenStores();

  WaitForEnd();

  CleanUp();

  return 0;
}
