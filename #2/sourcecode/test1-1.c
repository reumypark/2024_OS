// test1-1.c
#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    //Pass the parameters to scheduler_test
    char *args[] = {"scheduler_test", "0", "0", "0", "0", "500", "1"};
    exec("scheduler_test", args);

    printf(1, "Exec fail\n");
    exit();
}
