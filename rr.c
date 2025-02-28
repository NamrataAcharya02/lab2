#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 burst_left;
  /*u32 waiting_time;*/
  u32 response_time;
  bool responded;
  bool added;

  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  u32 current_time = data[0].arrival_time;
  struct process * current_proc; 

  /* Set current time to first arrival time*/
  for (u32 i = 0; i < size; i++)
  {
    current_proc = &data[i];
    if (current_proc->arrival_time < current_time)
    {
      current_time = current_proc->arrival_time;
    }
    
    current_proc->burst_left = current_proc->burst_time;
    current_proc->responded = false;
    current_proc->added = false;

  }

  bool done = false;
  u32 quant = 1;
  u32 size_left = size;
  /*printf("starting loop. %d", current_time);
*/
  while (size_left > 0) {
    struct process * p;
    

    /* check if current time = arrival time of any process*/
    for (u32 i = 0; i < size; i++)
    {
      struct process * proc = &data[i];
      if (!proc->added && proc->arrival_time == current_time)
      {
        TAILQ_INSERT_TAIL(&list, proc, pointers);
        proc->added = true;
      }
    }
    /*printf("Time: %d, q: %d, \n", current_time, quant);

    TAILQ_FOREACH(p, &list, pointers) {
      printf(" Queue is pid %d, left burst %d\n", p->pid, p->burst_left);
      printf("-----------------------------\n");
    }*/

    /* check if quant <= quantum length*/
    if (quant <= quantum_length)
    {
      /* check if left burst > 0*/
      if (TAILQ_FIRST(&list)->burst_left > 0)
      {
        /* check if response time = 0*/
        if (!TAILQ_FIRST(&list)->responded)
        {
          TAILQ_FIRST(&list)->response_time = current_time - TAILQ_FIRST(&list)->arrival_time;
           TAILQ_FIRST(&list)->responded = true;
          
          /*printf("response time for %d: %d",  TAILQ_FIRST(&list)->pid, TAILQ_FIRST(&list)->response_time);
        */
        }
        TAILQ_FIRST(&list)->burst_left--;
        quant++;
      } else {
        /* remove first process and reset quant*/
        total_waiting_time += current_time - TAILQ_FIRST(&list)->arrival_time - TAILQ_FIRST(&list)->burst_time;
        TAILQ_REMOVE(&list, TAILQ_FIRST(&list), pointers);
        size_left--;
        current_time--;
        
        quant = 1;
      }
    } else {
      /* remove first process and reset quant*/
      if (TAILQ_FIRST(&list)->burst_left == 0)
      {
        
        total_waiting_time += current_time - TAILQ_FIRST(&list)->arrival_time - TAILQ_FIRST(&list)->burst_time;
        current_time--;
        TAILQ_REMOVE(&list, TAILQ_FIRST(&list), pointers);
        quant = 1;
        size_left--;
        
      } else {
        struct process* temp = TAILQ_FIRST(&list);
        TAILQ_REMOVE(&list, TAILQ_FIRST(&list), pointers);
        TAILQ_INSERT_TAIL(&list, temp, pointers);
        current_time --;
        quant = 1;
      }
      
    }
    
    current_time++;
  }

  for (u32 i = 0; i < size; i++) {
    struct process * p = &data[i];
    total_response_time += p->response_time;
  }

  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
