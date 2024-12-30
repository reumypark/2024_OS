#include "types.h"
#include "stat.h"
#include "user.h"
#include "types.h"

int atoi_(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';
    return res;
} 

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf(1, "Usage: scheduler_test q_level cpu_burst cpu_wait io_wait end_time\n");
        exit();
    }

    int q_level = atoi_(argv[1]);
    int cpu_burst = atoi_(argv[2]);
    int cpu_wait = atoi_(argv[3]);
    int io_wait = atoi_(argv[4]);
    int end_time = atoi_(argv[5]);
    int p_num = atoi_(argv[6]);

    // Print start message
    printf(1, "start scheduler_test\n");

    // Get process parameters set by test1-1, test1-2, or test1-3
    // Create up to 3 child processes
    
    for(int i = 0; i < p_num; i++) {
	int pid = fork();
	if(pid == 0) {	
	 set_proc_info(q_level, cpu_burst, cpu_wait, io_wait, end_time);
          while (1);
	  exit();
	}
	else if(pid < 0) {
	   printf(1, "Fork failed!\n");
           
	} else if(pid > 0){
	  //It is parent process.  
	}
    }


    for(int i = 0; i < p_num; i++) wait();

    printf(1, "end of scheduler_test\n");  
    exit();
}
