/**
* Name: Eyal Cohen.
**/

#include <stdio.h>
#include <string.h>

#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>

#define BG "&"
#define AMPERSENT "&"
#define BUFF_SIZED 1500
#define SPACE " "
#define NUM_ELEMENTS 512
#define NUM_COMMANDS 3
#define NUM_JOBS 512
#define TRUE 1

typedef struct {
    int id;
    char string_command[NUM_ELEMENTS];
} Job;

/**
 * parse the input to after parsed array.
 * @param input to parse
 * @param after_parsed save the parsed with space.
 */
void stringParser(char *input, char **after_parsed, int *size) {
    char delimeter[2] = SPACE;
    int local_size = 0;
    /* get the first token */
    char *token = strtok(input, delimeter);
    while (token != NULL) {
        after_parsed[local_size] = (char *) malloc((sizeof(char) * strlen(token)) + 1);
        strcpy(after_parsed[local_size], token);
        local_size++;
        token = strtok(NULL, delimeter);
    }
    after_parsed[local_size - 1] = strtok(after_parsed[local_size - 1], "\n");
    /* remove the last space */
    *size = local_size;
}

/**
 * get jobs array and print all the jobs that still running.
 * @param jobs array of jobs.
 * @param capacity of the array.
 */
void printJobs(Job *jobs, int capacity) {
    for (int i = 0; i < capacity; ++i) {
        printf("%d %s\n", jobs[i].id, jobs[i].string_command);
    }
}

/**
 * init a new job by parameters and add it to the jobs array.
 * @param pid of the job.
 * @param string_command string of the job;
 * @param jobs the array of the jobs.
 * @param capacity
 */
void addJob(int pid, char *string_command, Job *jobs, int *capacity) {
    int current_capacity = *capacity;
    Job job;
    job.id = pid;
    // clean the "&" at the end.
    char *current_command = strtok(string_command, AMPERSENT);
    strcpy(job.string_command, current_command);
    jobs[current_capacity] = job;
    ++current_capacity;
    *capacity = current_capacity;
}

/**
 * delete all jobs that are done.
 * @param jobs array of jobs.
 * @param capacity size of capacity.
 */
void deleteDeadJobs(Job *jobs, int *capacity) {
    int terminated_pid;
    int status;
    int current_capacity = *capacity;
    //find element and switch it with the last, equal to delete it.
    for (int i = 0; i < current_capacity; ++i) {
        //find the id in the array.
        terminated_pid = waitpid(jobs[i].id, &status, WNOHANG);
        if (jobs[i].id == terminated_pid) {
            //edge case when we need to delete last element
            if (i != current_capacity - 1) {
                jobs[i] = jobs[current_capacity - 1];
            }
            --current_capacity;
            *capacity = current_capacity;
        } else if (terminated_pid == -1) {
            printf("error in reading child process status");
        }
    }
}

void executeJobCommand(Job *jobs, int *capacity) {
    deleteDeadJobs(jobs, capacity);
    printJobs(jobs, *capacity);
}

/**
 * Get parsered array and delete the aloccated strings in it.
 * @param parse words.
 * @param size size of array.
 */
void freeParsered(char **parse, int size) {
    for (int i = 0; i < size; ++i) {
        free(parse[i]);
        parse[i] = NULL;
    }
}

/**
 * get the input command and save.
 * @param buffer to get input to.
 * @return 1 if succeed
 */
int getInput(char *buffer) {
    printf("> ");
    fgets(buffer, BUFF_SIZED, stdin);
    if (buffer[0] == '\n') getInput(buffer);
}

/**
 * execute the parsered command with execvp
 * @param parsed input, first arg is the name of the command, others are the parameters.
 */
void executeRegularCommand(char *buffer, char **parsed, int *size, Job *jobs, int *job_capacity) {
    // current size.
    int local_size = (*size);
    int back_ground = 0;
    if (strcmp(parsed[local_size - 1], BG) == 0) {
        parsed[local_size - 1] = NULL;
        back_ground = 1;
    }
    // Forking a new child.
    pid_t pid = fork();
    // check if fork failed.
    if (pid == -1) {
        printf("Failed to forked the child\n");
        return;
    } else if (pid == 0) {
        // only the child will be here.
        if (execvp(parsed[0], parsed) < 0) {
            fprintf(stderr, "Error in system call\n");
        }
        freeParsered(parsed, *size);
        exit(0);
    } else {
        // father waiting for child to terminate
        printf("%d\n", pid);
        if (back_ground) {
            addJob(pid, buffer, jobs, job_capacity);
        } else {
            wait(NULL);
        }
        return;
    }
}

/**
 * init the command array.
 * @param command_array to init.
 */
void initCommandArray(char **command_array) {
    command_array[0] = "cd";
    command_array[1] = "exit";
    command_array[2] = "jobs";
}

/**
 * check if the command is a special command. return 1 if it does.
 * @param command_array to check the command from.
 * @param parsed parsered line.
 * @return 1 if special 0 otherwise.
 */
int isSpecialCommand(char **command_array, char **parsed) {
    char *command_name = parsed[0];
    for (int i = 0; i < NUM_COMMANDS; ++i) {
        if (strcmp(command_name, command_array[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * execute command, spacial or regular.
 * @param parsed line parsered.
 * @param size of the parsered line.
 * @param Job jobs array.
 * @param jobs_capacity pointer to the capacity.
 */
void executeSpecialCommand(char **parsed, int *size, Job *jobs, int *jobs_capacity, char **command_array) {
    // choose of the spacial command if it is.
    int choose = 0;
    // spacial command array.
    // name of command
    char *command_name = parsed[0];
    for (int i = 0; i < NUM_COMMANDS; ++i) {
        if (strcmp(command_name, command_array[i]) == 0) {
            choose = i + 1;
            break;
        }
    }
    switch (choose) {
        case 1:
            //edge cases for different cases.
            if (parsed[1] == NULL) {
                chdir(getenv("HOME"));
            } else if (strcmp(parsed[1], "~") == 0) {
                chdir(getenv("HOME"));
            } else if (chdir(parsed[1]) == -1) {
                fprintf(stderr, "Error in system call\n");
            }
            printf("%d\n", getpid());
            break;
        case 2:
            freeParsered(parsed, *size);
            printf("%d\n", getpid());
            int jobs_size = *jobs_capacity;
            // kill the children running processes
            for (int i = 0; i < jobs_size; ++i) {
                int terminated_pid = waitpid(jobs[i].id, NULL, WNOHANG);
                if (terminated_pid == 0) {
                    kill(jobs[i].id, SIGKILL);
                }
            }
            exit(0);
        case 3:
            executeJobCommand(jobs, jobs_capacity);
        default:
//            executeRegularCommand(parsed, size);
            break;
    }
}

/**
 * print all the parsered words.
 * @param parse lines.
 * @param size of the parsered array.
 */
void printParsered(char **parse, int size) {
    for (int i = 0; i < size; ++i) {
        printf("%s\n", parse[i]);
    }
}

int main() {
    Job jobs[NUM_JOBS];
    int jobs_capacity = 0;
    char *command_array[NUM_COMMANDS];
    initCommandArray(command_array);
    while (TRUE) {
        int size = 0;
        char *parsed[NUM_ELEMENTS];
        char buffer[NUM_JOBS];
        getInput(buffer);
        char *buffer_copy = (char *) malloc(sizeof(char) * strlen(buffer) + 1);
        strcpy(buffer_copy, buffer);
        stringParser(buffer, parsed, &size);
        // check whether is special command or regular and execute the kind of command.
        if (isSpecialCommand(command_array, parsed)) {
            executeSpecialCommand(parsed, &size, jobs, &jobs_capacity, command_array);
        } else {
            // do regular command.
            executeRegularCommand(buffer_copy, parsed, &size, jobs, &jobs_capacity);
        }
        freeParsered(parsed, size);
        free(buffer_copy);
        buffer_copy = NULL;
    }
}