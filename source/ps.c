#ifdef CS333_P2

#define MAX_ENTRIES 16

#include "types.h"
#include "user.h"
#include "uproc.h"

int
main(int argc, char *argv[])
{
  struct uproc* value;

  value = malloc(sizeof(struct uproc) * MAX_ENTRIES);

  if (getprocs(MAX_ENTRIES, value) < 0) {
    printf(2,"Error: getprocs call failed. %s at line %d\n",
        __FILE__, __LINE__);
    exit();
  }

  printf(1, "\nPID\tName\t\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\n");

  for (int i = 0; i < MAX_ENTRIES; i++){
    if (value[i].pid == NULL)
      break;
    printf(1, "%d\t%s\t\t%d\t%d\t%d\t", 
      value[i].pid,
      value[i].name,
      value[i].uid,
      value[i].gid,
      value[i].ppid
    );

    // Special case for floating point
    // Elapsed Ticks
    if (value[i].elapsed_ticks < 10){
      printf(1, "0.00%d\t", value[i].elapsed_ticks);
    } else if (value[i].elapsed_ticks < 100){
      printf(1, "0.0%d\t", value[i].elapsed_ticks);
    } else if (value[i].elapsed_ticks < 1000){
      printf(1, "0.%d\t", value[i].elapsed_ticks);
    }else{
      printf(1, "%d.%d\t", value[i].elapsed_ticks/1000, value[i].elapsed_ticks%1000);
    }

    // CPU Total Ticks
    if (value[i].CPU_total_ticks < 10){
      printf(1, "0.00%d", value[i].CPU_total_ticks);
    } else if (value[i].CPU_total_ticks < 100){
      printf(1, "0.0%d", value[i].CPU_total_ticks);
    } else if (value[i].CPU_total_ticks < 1000){
      printf(1, "0.%d", value[i].CPU_total_ticks);
    }else{
      printf(1, "%d.%d", value[i].CPU_total_ticks/1000, value[i].CPU_total_ticks%1000);
    }

    printf(1, "\t%s\t%d\n",
      value[i].state,
      value[i].size
    );
  }

  printf(1, "\n");
  exit();
}

#endif // CS333_P2