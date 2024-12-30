#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h" 

//define SEEK_SET, SEEK_CUR, SEEK_END
#define SEEK_SET 0 
#define SEEK_CUR 1 
#define SEEK_END 2 

int main(int argc, char *argv[]) {

    int offset;
    char *new_string;
    int fp;

    //check number of arguemnt error
    if(argc < 4){
	printf(1, "usage : lseek_test <filename> <offset> <string>\n");
	exit(); 
    }
    
    // open the file in read&write mode
    fp = open(argv[1], O_RDWR);
    if(fp < 0) {
	    //handle file exist error
	    printf(1, "The file dosen't exist");
	    exit();
    }

    //print the original file contents
    printf(1, "Before : ");
    char buf[512];
    int n;
    while ((n = read(fp, buf, sizeof(buf))) > 0) {
        write(1, buf, n);  // Output file contents to stdout
    }
    printf(1, "\n");

    //store offset and string that be wanted to insert
    offset = atoi(argv[2]);
    new_string = argv[3];

    //move file pointer
    if (lseek(fp, offset, SEEK_SET) < 0) {
        exit();
    }
    // write new argv[3] at the offset
    if (write(fp, new_string, strlen(new_string)) < 0) {
        exit();
    }

    lseek(fp, 0, SEEK_SET);
    
    //print the modificated file contents
    printf(1, "After : ");
    while ((n = read(fp, buf, sizeof(buf))) > 0) {
        write(1, buf, n);  // Output file contents to stdout
    }
    printf(1, "\n");

    close(fp);
    exit();

}
