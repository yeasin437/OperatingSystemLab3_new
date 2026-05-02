#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[])
{
    // Program needs directory path and exclude file path
    if (argc != 3) {
        printf("Usage: ./parent directory exclude_file\n");
        return 1;
    }

    int numberPipe[2]; // child 2 and child 3 send random numbers here
    int donePipe[2];   // used to tell child 4 when first 3 children are done

    // Create pipe for random numbers
    if (pipe(numberPipe) == -1) {
        perror("number pipe error");
        return 1;
    }

    // Create pipe for completion signal
    if (pipe(donePipe) == -1) {
        perror("done pipe error");
        return 1;
    }

    pid_t child2 = -1;
    pid_t child3 = -1;

    // Create 4 child processes using loop
    for (int i = 1; i <= 4; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork error");
            return 1;
        }

        // Child process
        if (pid == 0) {

            // Child 1: run child1 program using exec
            if (i == 1) {
                close(numberPipe[0]);
                close(numberPipe[1]);
                close(donePipe[0]);

                char doneFdText[20];
                sprintf(doneFdText, "%d", donePipe[1]);

                char *args[] = {"./child1", argv[1], argv[2], doneFdText, NULL};
                execv("./child1", args);

                perror("exec child1 failed");
                exit(1);
            }

            // Child 2: generate random number and write to pipe
            else if (i == 2) {
                close(numberPipe[0]);
                close(donePipe[0]);

                srand(time(NULL) + getpid());

                int message[2];
                message[0] = 2;           // child number
                message[1] = rand() % 11; // random number 0-10

                printf("Child 2 generated number: %d\n", message[1]);

                write(numberPipe[1], message, sizeof(message));
                write(donePipe[1], "2", 1);

                close(numberPipe[1]);
                close(donePipe[1]);

                exit(0);
            }

            // Child 3: generate random number and write to same pipe
            else if (i == 3) {
                close(numberPipe[0]);
                close(donePipe[0]);

                srand(time(NULL) + getpid());

                int message[2];
                message[0] = 3;
                message[1] = rand() % 11;

                printf("Child 3 generated number: %d\n", message[1]);

                write(numberPipe[1], message, sizeof(message));
                write(donePipe[1], "3", 1);

                close(numberPipe[1]);
                close(donePipe[1]);

                exit(0);
            }

            // Child 4: wait until first 3 children finish, then print date
            else if (i == 4) {
                close(numberPipe[0]);
                close(numberPipe[1]);
                close(donePipe[1]);

                char signal;
                int count = 0;

                // Wait for 3 signals: child1, child2, child3
                while (count < 3) {
                    if (read(donePipe[0], &signal, 1) > 0) {
                        count++;
                    }
                }

                close(donePipe[0]);

                // Run date command using exec
                execlp("date", "date", NULL);

                perror("date command failed");
                exit(1);
            }
        }

        // Parent process
        else {
            if (i == 2) {
                child2 = pid;
            }
            else if (i == 3) {
                child3 = pid;
            }
        }
    }

    // Parent only reads from numberPipe
    close(numberPipe[1]);

    // Parent does not use donePipe after children are created
    close(donePipe[0]);
    close(donePipe[1]);

    int num2 = -1;
    int num3 = -1;
    int message[2];

    // Wait for child 2
    waitpid(child2, NULL, 0);

    read(numberPipe[0], message, sizeof(message));

    if (message[0] == 2) {
        num2 = message[1];
    }
    else {
        num3 = message[1];
    }

    // Wait for child 3
    waitpid(child3, NULL, 0);

    read(numberPipe[0], message, sizeof(message));

    if (message[0] == 2) {
        num2 = message[1];
    }
    else {
        num3 = message[1];
    }

    printf("Parent received number from Child 2: %d\n", num2);
    printf("Parent received number from Child 3: %d\n", num3);

    // Compare numbers
    if (num2 > num3) {
        printf("Winner: Child 2\n");
    }
    else if (num3 > num2) {
        printf("Winner: Child 3\n");
    }
    else {
        printf("Result: Tie\n");
    }

    close(numberPipe[0]);

    return 0;
}
