#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

// Remove dot and newline from extension text
void cleanText(char text[])
{
    int i = 0;
    int j = 0;
    char temp[50];

    if (text[0] == '.') {
        i = 1;
    }

    while (text[i] != '\0' && text[i] != '\n' && text[i] != '\r') {
        temp[j] = text[i];
        i++;
        j++;
    }

    temp[j] = '\0';
    strcpy(text, temp);
}

// Check if file extension is in exclude list
int isExcluded(char ext[], char excluded[][50], int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(ext, excluded[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: ./child1 directory exclude_file done_pipe_fd\n");
        return 1;
    }

    char *folderPath = argv[1];
    char *excludeFile = argv[2];
    int doneFd = atoi(argv[3]);

    char excluded[100][50];
    int excludeCount = 0;

    // Open exclude file using system call
    int fd = open(excludeFile, O_RDONLY);

    if (fd < 0) {
        perror("exclude file open failed");
    }
    else {
        char buffer[4096];

        // Read exclude file using read system call
        int bytesRead = read(fd, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            char oneLine[50];
            int index = 0;

            // Separate exclude file line by line
            for (int i = 0; buffer[i] != '\0'; i++) {
                if (buffer[i] == '\n') {
                    oneLine[index] = '\0';
                    cleanText(oneLine);

                    if (strlen(oneLine) > 0) {
                        strcpy(excluded[excludeCount], oneLine);
                        excludeCount++;
                    }

                    index = 0;
                }
                else {
                    if (index < 49) {
                        oneLine[index] = buffer[i];
                        index++;
                    }
                }
            }

            // Handle last line if no newline at end
            if (index > 0) {
                oneLine[index] = '\0';
                cleanText(oneLine);

                if (strlen(oneLine) > 0) {
                    strcpy(excluded[excludeCount], oneLine);
                    excludeCount++;
                }
            }
        }

        close(fd);
    }

    // Open directory
    DIR *dir = opendir(folderPath);

    if (dir == NULL) {
        perror("directory open failed");
        write(doneFd, "1", 1);
        close(doneFd);
        return 1;
    }

    struct dirent *entry;

    // Read directory entries one by one
    while ((entry = readdir(dir)) != NULL) {

        // Skip hidden files, . and ..
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Find file extension
        char *dot = strrchr(entry->d_name, '.');

        if (dot == NULL) {
            continue;
        }

        char ext[50];
        strcpy(ext, dot + 1);

        // Skip excluded file types
        if (isExcluded(ext, excluded, excludeCount)) {
            printf("Child 1 skipped: %s\n", entry->d_name);
            continue;
        }

        char newFolder[300];
        char oldPath[300];
        char newPath[300];

        // Create folder name based on extension
        snprintf(newFolder, sizeof(newFolder), "%s/%s", folderPath, ext);
        mkdir(newFolder, 0777);

        // Build old and new file paths
        snprintf(oldPath, sizeof(oldPath), "%s/%s", folderPath, entry->d_name);
        snprintf(newPath, sizeof(newPath), "%s/%s/%s", folderPath, ext, entry->d_name);

        // Move file into extension folder
        if (rename(oldPath, newPath) == 0) {
            printf("Child 1 moved %s to %s folder\n", entry->d_name, ext);
        }
    }

    closedir(dir);

    printf("Child 1 finished file categorization.\n");

    // Tell child 4 that child 1 is done
    write(doneFd, "1", 1);
    close(doneFd);

    return 0;
}
