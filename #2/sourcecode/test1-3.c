// test1-3.c
#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    //Pass the parameters to scheduler_test
    char *args[] = {"scheduler_test", "2", "0", "0", "0", "300", "3"};
    exec("scheduler_test", args);

    printf(1, "Exec fail\n");
    exit();
}

