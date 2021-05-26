#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int current_time = uptime();

  if (argc == 1){
      int leftover = 0;
      int time_now = uptime() - current_time;
      if(time_now > 1000){
        leftover = time_now % 1000;
        time_now /= 1000;
    }
      if(leftover != 0){
        printf(1, "(null) ran in %d.%d seconds\n", time_now, leftover);
    } else{
        printf(1, "(null) ran in 0.%d seconds\n", time_now);
     }
    } else {
        if (fork() == 0){
            exec(argv[1], &argv[1]);
        }else{
            wait();
             int leftover = 0;
             int time_now = uptime() - current_time;
            
            if(time_now > 1000){
            leftover = time_now % 1000;
            time_now /= 1000;
            }
            if(leftover != 0){
            printf(1, "%s ran in %d.%d seconds\n", argv[1], time_now, leftover);
            } else{
            printf(1, "%s ran in 0.%d seconds\n", argv[1], time_now);
            }
        }
    }

  exit();
}