// test1-2.c
#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    //Pass the parameter to scheduler_test
    char *args[] = {"scheduler_test", "1", "0", "0", "0", "500", "1"};
    exec("scheduler_test", args);

    printf(1, "Exec fail\n");
    exit();
}

