#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/wait.h>


#define true 1
#define false 0
#define bool int

typedef int error_code;

#define ERROR (-1)
#define HAS_ERROR(code) ((code) < 0)
#define NULL_TERMINATOR '\0'

#define MAX_CMD 10
#define MAX_CMD_LENGTH 100
enum op {
    BIDON, NONE, OR, AND, ALSO    //BIDON is just to make NONE=1, BIDON is unused
};

struct command {
    char **call;
    enum op operator;
    struct command *next;
    int count;
    bool also;
    char *line;
};
//hint hint: this looks suspiciously similar to a linked list we saw in the demo. I wonder if I could use the same ideas here??

void freeStringArray(char **arr) {  //todo probably add this to free the "call" parameter inside of command
    if (arr != NULL) {
        for (int i = 0; i < MAX_CMD; i++) {
            free(arr[i]);
        }
    }
    free(arr);
}

error_code readline(char **out) {   //todo this is pretty barebones, you must complete it
    size_t size = 1024;                       // size of the char array
    char *line = malloc(sizeof(char) * size);       // initialize a ten-char line
    int ch;
    if (line == NULL) return ERROR;   // if we can't, terminate because of a memory issue

    for (int at = 0; size; at++) { //todo 10 is clearly too small, make this bigger
        ch = getchar();
        if (ch == '\n') {        // if we get a newline
            line[at] = NULL_TERMINATOR;    // finish the line with return 0
            break;
        }
        line[at] = ch; // sets ch at the current index and increments the index
    }

    out[0] = line;
    return 0;
}

/* To check if a command has rN , if so take out the last ")" */
error_code repeatchecker(char *buf) {
    int penDown = 0;
    int yes = 0;
    for (int i = 0; buf[i] != '\0'; i++) {
        if (penDown == 0 && buf[i] == 'r' && (buf[i + 1])>='0' && (buf[i + 1])<='9') {
            yes = 1;
        } else if (buf[i] != ' ') {
            penDown = 1;
        }
        if (buf[i] == ')') {
            if (yes == 1) {
                buf[i] = '\0';
            }
        }
    }
    return yes;
}

/** To parse a line in to a single command form AKA "call" */
error_code parsecommand(char **call, char *buf) {
    int count = 1;
    int k = 0;
    char *countSTR = calloc(1,MAX_CMD_LENGTH);
    int rn = repeatchecker(buf);
    if (rn == 1) {
        while (*buf != '(') {
            if ((*buf)>='0' && (*buf)<='9') {
                countSTR[k] = *buf;
                k++;
            }
            buf++;
        }
        *buf = '\0';
        buf++;
        if(countSTR != NULL){
        count = atoi(countSTR);}
    }

    if (buf == NULL) {
        call = NULL;
        return 1;
    }
    int i = 0;
    int j = 0;
    int penDown = 0;

    while (*buf != '\0') {
        if (*buf != ' ') {
            penDown = 1;
            call[i][j] = *buf;
            j++;
        } else if (penDown == 1 && *buf == ' ') {
            penDown = 0;
            call[i][j] = '\0';
            i++;
            j = 0;
        }
        buf++;
    }

    call[i][j] = '\0';
    for (i = 0; i < MAX_CMD; i++) {
        if (strcmp(call[i],"\0")==0) {
            free(call[i]);
            call[i] = NULL;
            break;
        }
    }
    free(countSTR);
    return count;
}

/** Parsing a line to a struct command */
struct command *parseline(char *line1) {
    if(strcmp(line1," ")==0 || *line1 == '\0' ){
        return NULL;
    }

    struct command *head = calloc(1,sizeof(struct command));
    head->call = malloc(MAX_CMD *sizeof (char *));
    for (int i = 0; i < MAX_CMD; ++i) {
        head->call[i] = calloc(1,MAX_CMD_LENGTH);
    }

    head->operator = BIDON;
    head->also = 0;
    head->count = 0;
    struct command *temp;
    struct command *next ;

    int counter;
    int penDown = 1;
    for (int i = 0; line1[i] != '\0'; i++) {
        if (line1[i + 1] == '\0') {
            if (line1[i] != '&') {
                temp->count = parsecommand(temp->call, temp->line);
            } else {
                line1[i] = '\0';
                counter = parsecommand(temp->call, temp->line);
                temp->count = counter;
                temp->operator = ALSO;
                head->also = 1;
            }
        }
        if (penDown == 1 && (line1[i] != '&' || line1[i] != '|')) {

            temp = head;
            temp->line = line1 + i;
            penDown = 0;

        } else if (line1[i] == '&' && line1[i + 1] == '&') {
            line1[i] = '\0';
            temp->count = parsecommand(temp->call, temp->line);
            temp->operator = AND;
            if (line1[i + 2] != '\0') {
                next = calloc(1,sizeof(struct command));
                next->call = malloc(MAX_CMD *sizeof (char *));
                for (int i = 0; i < MAX_CMD; i++) {
                    next->call[i] = calloc(1,MAX_CMD_LENGTH);
                }
                next->operator = BIDON;
                next->also = 0;
                next->count = 0;
                temp->next = next;
                temp = next;
                temp->line = line1 + i + 2;
            }
            i++;

        } else if (line1[i] == '|' && line1[i + 1] == '|') {

            line1[i] = '\0';
            temp->count = parsecommand(temp->call, temp->line);
            temp->operator = OR;
            if (line1[i + 2] != '\0') {
                next = calloc(1,sizeof(struct command));
                next->call = malloc(MAX_CMD *sizeof (char *));
                for (int i = 0; i < MAX_CMD; i++) {
                    next->call[i] = calloc(1,MAX_CMD_LENGTH);
                }
                next->operator = BIDON;
                next->also = 0;
                next->count = 0;
                temp->next = next;
                temp = next;
                temp->line = line1 + i + 2;
            }
            i++;
        } else if (line1[i] == '&' && line1[i + 1] != '&') {
            line1[i] = '\0';
            temp->count = parsecommand(temp->call, temp->line);
            temp->operator = ALSO;
            head->also = 1;
            if (line1[i + 1] != '\0') {
                next = calloc(1,sizeof(struct command));
                next->call = malloc(MAX_CMD *sizeof (char *));
                for (int i = 0; i < MAX_CMD; i++) {
                    next->call[i] = calloc(1,MAX_CMD_LENGTH);
                }
                next->operator = BIDON;
                next->also = 0;
                next->count = 0;
                temp->next = next;
                temp = next;
                temp->line = line1 + i + 2;
            }
        }
    }
    if (temp->operator == AND || temp->operator == OR) {
        if (temp->next == NULL) {
            printf("Invalid command!\n");
            return NULL;
        }
    }
    next = NULL;
    return head;
}

int executeSingleCmd(char **argv) {
    pid_t pid;
    pid = fork();
    int status;
    if (pid == 0) {
        /* child*/
        execvp(argv[0], argv);
        printf("%s: command not found\n", argv[0]);
        exit(1);
    }
    if (pid == -1) {
        perror("fork error ! \n");
        exit(1);
    }
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

int executeMultipleCommand(struct command *lineCmd) {
    struct command *tmp = lineCmd;
    while (tmp != NULL) {
        int code;
        for (int i = 0; i < tmp->count; i++) {
            code = executeSingleCmd(tmp->call);
        }
        if (tmp->operator == AND) {
            if (code == 0) {
                struct command *node = tmp->next;
                tmp = node;
            } else {
                while (tmp->next != NULL) {
                    if (tmp->next->operator == AND) {
                        struct command *node = tmp->next;
                        tmp = node;
                    } else if (tmp->next->operator == OR) {
                        struct command *node = tmp->next->next;
                        tmp = node;
                        break;
                    } else if (tmp->next->operator == BIDON || tmp->next->operator == ALSO) {
                        struct command *node = tmp->next->next;
                        tmp = node;
                        break;
                    }
                }
            }
        } else if (tmp->operator == OR) {
            if (code != 0) {
                struct command *node = tmp->next;
                tmp = node;
            } else {
                while (tmp->next != NULL) {
                    if (tmp->next->operator == OR) {
                        struct command *node = tmp->next;
                        tmp = node;
                    } else if (tmp->next->operator == AND) {
                        struct command *node = tmp->next->next;
                        tmp = node;
                        break;
                    } else if (tmp->next->operator == BIDON || tmp->next->operator == ALSO) {
                        struct command *node = tmp->next->next;
                        tmp = node;
                        break;
                    }
                }
            }
        } else if (tmp->operator == BIDON || tmp->operator == ALSO) {
            return 0;
        }
    }
    return 0;
}

int execute(struct command *cmdLine) {
    if (cmdLine == NULL) {
        return ERROR;
    }
    if (cmdLine->also == 1) {
        pid_t pid;
        pid = fork();
        int status;
        if (pid == 0) {
            executeMultipleCommand(cmdLine);
            exit(0);
        } else {
            char *line;
            readline(&line);
            if (strcmp(line, "exit") == 0) {
                free(line);
                exit(0);
            }
            struct command *line_cmd = parseline(line);
            execute(line_cmd); //Little recursion
            free(line);
            waitpid(pid, &status, 0);
        }
    } else {
        executeMultipleCommand(cmdLine);
    }
    struct command *node1;
    while (cmdLine != NULL) {
        node1 = cmdLine;
        cmdLine = cmdLine->next;
        freeStringArray(node1->call);
        free(node1);
    }
    return 0;
}

int main(void) {
    struct command *line_cmd;
    while (1) {
        char *line;
        readline(&line);    //todo what about error_code?
        if (strcmp(line, "exit") == 0) {
            free(line);
            exit(0);
        }
        line_cmd = parseline(line);
        execute(line_cmd);
        free(line);
    }
}