#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

#define MAX_PENDING 5
#define MAX_LINE 256

#define FIND 1
#define ADD 2
#define LIST 3
#define DELETE 4
#define HELP 5

const char *databaseFileName = "mydb.txt";
const char *tempDatabaseFileName="tempdb.txt";
char *prependedmsg="";

void trim(char *input)
{
    char *dst = input, *src = input;
    char *end;
    int noOfValidCharacters = 0;
    int i = 0;
    char *emptyString = "";
    while (input[i] != '\0')
    {
        if (input[i] != ' ')
            noOfValidCharacters++;
        i++;
    }

    if (noOfValidCharacters == 0)
    {
        input = emptyString;
    }
    else
    {
        // Skip whitespace at front...
        while (isspace((unsigned char)*src))
        {
            ++src;
        }

        // Trim at end...
        end = src + strlen(src) - 1;
        while (end > src && isspace((unsigned char)*end))
        {
            *end-- = 0;
        }

        // Move if needed.
        if (src != dst)
        {
            while ((*dst++ = *src++));
        }
    }
}

void convertStringToUpperCase(char *str)
{
    int i = 0;
    while (str[i])
    {
        str[i] = toupper(str[i]);
        i++;
    }
}

struct Course
{
    char *courseId;
    char *courseName;
    int noOfRegisteredStudents;
    int noOfVacanciesLeft;
    char *timeOffered;
};

int getNumOfTokens(char *str)
{
    int i = 0, numOfTokens = 0;
    while (str[i] != '\0')
    {
        if (str[i] == '|')
            numOfTokens++;
        i++;
    }
    return numOfTokens + 1;
}

char **tokenize(char *str)
{
    char **tokens = calloc(1, sizeof(char*));
    int numOfTokens = 1;
    int i = 0;
    int start = -1;
    int tokens_used = 0;

    int k = 0, j = 0;

    while (str[i] != '\0')
    {
        if (str[i] == '|')
            numOfTokens++;
        i++;
    }
    tokens = realloc(tokens, numOfTokens * sizeof(char*));

    i = 0;
    while (str[i] != '\0')
    {
        if (str[i] == '|')
        {
            char *to = (char*)malloc(i - start + 1);
            j = 0;
            for (k = start + 1; k < i; k++)
            {
                to[j] = str[k];
                j++;
            }
            to[j] = '\0';
            tokens[tokens_used++] = to;
            start = i;
        }
        i++;
    }
    if (str[i - 1] != '|')
    {
        char *to = (char*)malloc(i - start + 1);
        j = 0;
        for (k = start + 1; k < i; k++)
        {
            to[j] = str[k];
            j++;
        }
        to[j] = '\0';
        tokens[tokens_used++] = to;
    }
    return tokens;
}

int getCommand(char *str)
{
    if (strcmp(str, "FIND") == 0 || strcmp(str, "SEARCH") == 0)
        return FIND;
    else if (strcmp(str, "ADD") == 0)
        return ADD;
    else if (strcmp(str, "LIST") == 0 || strcmp(str, "LIST ALL") == 0)
        return LIST;
    else if (strcmp(str, "DELETE") == 0)
        return DELETE;
    else if (strcmp(str, "HELP") == 0)
        return HELP;
    else
        return 0;
}

void emptyString(char *str)
{
    int i=0;
    for (i=0;i<strlen(str);i++)
        str[i]='\0';
}

char * convertPositiveIntegerToString(int num)
{
    int length = snprintf( NULL, 0, "%d", num );
    char* str = malloc( length + 1 );
    snprintf( str, length + 1, "%d", num );
    return str;
}

void displayAndSendMessage(int connection, char *msg)
{
    int len=0;
    int last;
    char *length=NULL;
    char *t2;
    char *t3;
    char * t1;

    if (strlen(prependedmsg)==0)
    {
        t1 = malloc(strlen(msg) + strlen("\nDB$->") + 1);
        while (t1 == NULL)
            t1 = malloc(strlen(msg) + strlen("\nDB$->") + 1);
        emptyString(t1);
        memset(t1, 0, sizeof(t1));
        strcat(t1, msg);
        strcat(t1, "\nDB$->");
    }
    else
    {
        t1 = malloc(strlen(prependedmsg)+strlen(msg) + strlen("\nDB$->") + 1);
        while (t1 == NULL)
            t1 = malloc(strlen(prependedmsg)+strlen(msg) + strlen("\nDB$->") + 1);
        emptyString(t1);
        memset(t1, 0, sizeof(t1));
        strcat(t1, prependedmsg);
        strcat(t1, msg);
        strcat(t1, "\nDB$->");
    }

    len=strlen(t1);
    length=convertPositiveIntegerToString(len);

    len=strlen(t1)+strlen(length)+1;
    length=convertPositiveIntegerToString(len);

    t2 = malloc(strlen(t1) + strlen(length)+1);
    while (t2==NULL)
        t2 = malloc(strlen(t1) + strlen(length)+1);
    emptyString(t2);
    memset(t2 ,0 , sizeof(t2));
    strcat(t2,length);
    strcat(t2,"|");
    strcat(t2,t1);

    printf("%s", t2);
    send(connection, t2, strlen(t2), 0);
}

void appendCourseToDatabase(struct Course course, int connection)
{
    FILE * fp;
    char * line = NULL;
    int found=0;
    size_t len = 0;
    ssize_t read;

    FILE *fptr;
    fptr = fopen(databaseFileName, "a+");

    fp = fopen(databaseFileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1)
    {
        char **tokens = tokenize(line);

        if (strncmp(tokens[0], course.courseId, strlen(tokens[0])) == 0)
        {
            found=1;
        }
    }
    fclose(fp);

    if (found==1)
    {
        char *msg = "Duplicate Course ID!\nCourse ID already Exists!";
        displayAndSendMessage(connection, msg);
    }
    else
    {
        if (fptr != NULL)
        {
            char *msg = "New Course Added!";
            fprintf(fptr, course.courseId);
            fprintf(fptr, "|");
            fprintf(fptr, course.courseName);
            fprintf(fptr, "|");
            fprintf(fptr, "%d", course.noOfRegisteredStudents);
            fprintf(fptr, "|");
            fprintf(fptr, "%d", course.noOfVacanciesLeft);
            fprintf(fptr, "|");
            fprintf(fptr, course.timeOffered);
            fprintf(fptr, "\n");
            fclose(fptr);
            displayAndSendMessage(connection, msg);
        }
    }
}

void listAllRecords(int connection)
{
    FILE * fp;
    char * line = NULL;
    char *prevmsg="Course ID|Course Name|No of Students Registered|No of Vacancies Left|Time Offered\n";
    size_t len = 0;
    ssize_t read;

    fp = fopen(databaseFileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1)
    {
        char * t1 = malloc(strlen(prevmsg) + strlen(line)+1);
        while (t1==NULL)
            t1 = malloc(strlen(prevmsg) + strlen(line));
        emptyString(t1);
        memset(t1 ,0 , sizeof(t1));
        strcat(t1, prevmsg);
        strcat(t1, line);
        emptyString(line);
        prevmsg=t1;

    }
    displayAndSendMessage(connection, prevmsg);
    fclose(fp);
    if (line)
        free(line);
}

void search(int connection,char *searchString)
{
    FILE * fp;
    char * line = NULL;
    int found=0;
    char *prevmsg="Course ID|Course Name|No of Students Registered|No of Vacancies Left|Time Offered\n";
    size_t len = 0;
    ssize_t read;

    fp = fopen(databaseFileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1)
    {
        char **tokens=tokenize(line);

        if (strncmp(tokens[0],searchString, strlen(tokens[0]))==0 || strncmp(tokens[1],searchString, strlen(tokens[1]))==0)
        {
            char *t1 = malloc(strlen("\nCourse ID:")+strlen(tokens[0]) +
                                      strlen("\nCourse Name:") + strlen(tokens[1]) +
                                      strlen("\nNo of Registered Students:") + strlen(tokens[2]) +
                                      strlen("\nNo of Vacancies Left:") + strlen(tokens[3])+
                                      strlen("\nTime Offered:") + strlen(tokens[4])+
                                      1);
            while (t1 == NULL)
                t1 = malloc(strlen(prevmsg) + strlen(line));
            emptyString(t1);
            memset(t1, 0, sizeof(t1));
            strcat(t1, "\nCourse ID:");
            strcat(t1, tokens[0]);
            strcat(t1, "\nCourse Name:");
            strcat(t1, tokens[1]);
            strcat(t1, "\nNo of Registered Students:");
            strcat(t1, tokens[2]);
            strcat(t1, "\nNo of Vacancies Left:");
            strcat(t1, tokens[3]);
            strcat(t1, "\nTime Offered:");
            strcat(t1, tokens[4]);
            emptyString(line);
            prevmsg = t1;
            found=1;
            break;
        }
        else if (strncmp(tokens[4],searchString, strlen(searchString))==0)
        {
            char *t1 = malloc(strlen(prevmsg) + strlen(line) + 1);
            while (t1 == NULL)
                t1 = malloc(strlen(prevmsg) + strlen(line));
            emptyString(t1);
            memset(t1, 0, sizeof(t1));
            strcat(t1, prevmsg);
            strcat(t1, line);
            emptyString(line);
            prevmsg = t1;
            found=1;
        }
    }
    if (found==0)
    {
        char *t1 = malloc(strlen("'")+strlen(searchString)+strlen("' Not Found!") + 1);
        while (t1 == NULL)
            t1 = malloc(strlen("'")+strlen(searchString)+strlen("' Not Found!") + 1);
        emptyString(t1);
        memset(t1, 0, sizeof(t1));
        strcat(t1, "'");
        strcat(t1, searchString);
        strcat(t1, "' Not Found!");
        emptyString(line);
        prevmsg = t1;
        found=1;
    }
    displayAndSendMessage(connection, prevmsg);
    fclose(fp);
    if (line)
        free(line);
}

void deleteCourse(int connection, char *courseName)
{
    FILE *fptr;
    FILE * fp;
    char * line = NULL;
    int found=0;
    char *prevmsg=malloc(strlen("'")+strlen(courseName)+strlen("' Deleted!")+1);
    emptyString(prevmsg);
    memset(prevmsg, 0, sizeof(prevmsg));
    size_t len = 0;
    ssize_t read;

    fptr = fopen(tempDatabaseFileName, "w");
    fp = fopen(databaseFileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        char **tokens=tokenize(line);

        if (strncmp(tokens[1],courseName, strlen(tokens[1]))!=0)
        {
            fprintf(fptr,line);
        }
        else
        {
            found=1;
        }
    }
    if (found==0)
    {
        prevmsg=malloc(strlen("'")+strlen(courseName)+strlen("' Not Found!")+1);
        emptyString(prevmsg);
        memset(prevmsg, 0, sizeof(prevmsg));
        strcat(prevmsg, "'");
        strcat(prevmsg, courseName);
        strcat(prevmsg, "' Not Found!");
    }
    else
    {
        strcat(prevmsg, "'");
        strcat(prevmsg, courseName);
        strcat(prevmsg, "' Deleted!");
    }
    fclose(fp);
    fclose(fptr);

    remove(databaseFileName);
    rename(tempDatabaseFileName,databaseFileName);

    displayAndSendMessage(connection, prevmsg);

    if (line)
        free(line);
}

int canStringBeConvertedToInteger(char *str)
{
    int i=0;
    while(str[i]!='\0')
    {
        if (str[i]=='0' ||
                str[i]=='1' ||
                str[i]=='2' ||
                str[i]=='3' ||
                str[i]=='4' ||
                str[i]=='5' ||
                str[i]=='6' ||
                str[i]=='7' ||
                str[i]=='8' ||
                str[i]=='9')
        {
            i++;
            continue;
        }
        else
            return 0;
    }
    return 1;
}

int main(int argc, char * argv[])
{
    struct sockaddr_in sin;
    char buf[MAX_LINE];//="  list |        |       |    |      |    ";
    int len;
    int s, new_s;
    int option = 1;
    int SERVER_PORT;

    if (argc==2)
    {
        SERVER_PORT = atoi(argv[1]);
    }
    else
    {
        fprintf(stderr, "usage: server <port>\n");
        exit(1);
    }

    /*build address data structure*/
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    //For work around Address Already in Used Error
    setsockopt(PF_INET, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /*setup passive open*/
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR: socket()");
        exit(1);
    }

    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("ERROR: bind()");
        exit(1);
    }

    fprintf(stderr, "INFO: Server started at port %d\n", SERVER_PORT);

    listen(s, MAX_PENDING);

    /*wait for connection, then receive and print text*/
    while (1)
    {
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0)
        {
            perror("ERROR: accept()");
            exit(1);
        }
        else
        {
            char *msg = "Course DB.\nUsage:<Command>[|<arguments>]\nHELP for Help\nEXIT to Exit";
            displayAndSendMessage(new_s, msg);
        }

        memset(buf ,0 , sizeof(buf));
        emptyString(buf);

        while (len = recv(new_s, buf, sizeof(buf), 0))
        {
            char **tokens;
            int numOfTokens;
            int i = 0;
            int command;
            char *msg;

            fputs(buf, stdout);
            convertStringToUpperCase(buf);
            numOfTokens = getNumOfTokens(buf);

            if (numOfTokens > 1)
                tokens = tokenize(buf);

            if (numOfTokens > 1)
            {
                for (i = 0; i < numOfTokens; i++)
                    trim(tokens[i]);
                command = getCommand(tokens[0]);
            }
            else
            {
                trim(buf);
                char *newBuf;
                int i=0;
                newBuf=malloc(strlen(buf)+1);

                while(buf[i]!='\0')
                {
                    newBuf[i]=buf[i];
                    i++;
                }
                newBuf[i]='\0';
                command = getCommand(newBuf);
            }

            prependedmsg=malloc(2);
            memset(prependedmsg ,0 , sizeof(prependedmsg));
            emptyString(prependedmsg);

            switch (command)
            {
                case LIST:
                    printf("LIST command issued\n");
                    if (numOfTokens > 1)
                    {
                        prependedmsg=malloc(strlen("Warning: Excess Arguments Ignored\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Warning: Excess Arguments Ignored\n");
                    }
                    listAllRecords(new_s);
                    break;
                case FIND:
                    printf("FIND command issued\n");
                    if (numOfTokens > 2)
                    {
                        prependedmsg=malloc(strlen("Warning: Excess Arguments Ignored\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Warning: Excess Arguments Ignored\n");
                    }
                    else if (numOfTokens<2)
                    {
                        prependedmsg=malloc(strlen("Error: Not Enough Argument(s)\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Error: Not Enough Argument(s)\n");
                        msg="";
                        displayAndSendMessage(new_s, msg);
                    }
                    else
                    {
                        search(new_s, tokens[1]);
                    }
                    break;
                case ADD:
                    printf("ADD command issued\n");
                    struct Course course;
                    if (numOfTokens > 6)
                    {
                        prependedmsg = malloc(strlen("Warning: Excess Arguments Ignored\n") + 1);
                        memset(prependedmsg, 0, sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg, "Warning: Excess Arguments Ignored\n");
                    }
                    else if (numOfTokens<6)
                    {
                        prependedmsg=malloc(strlen("Error: Not Enough Argument(s)\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Error: Not Enough Argument(s)\n");
                        msg="";
                        displayAndSendMessage(new_s, msg);
                    }
                    else
                    {
                        if (!canStringBeConvertedToInteger(tokens[3]) || !canStringBeConvertedToInteger(tokens[4]))
                        {
                            prependedmsg=malloc(strlen("Error: Argument for both 'No of Registered Students' and 'No of Vacancies left' must be Integer\n")+1);
                            memset(prependedmsg ,0 , sizeof(prependedmsg));
                            emptyString(prependedmsg);
                            strcat(prependedmsg,"Error: Argument for both 'No of Registered Students' and 'No of Vacancies left' must be Integer\n");
                            msg="";
                            displayAndSendMessage(new_s, msg);
                        }
                        else
                        {
                            course.courseId = tokens[1];
                            course.courseName = tokens[2];
                            course.noOfRegisteredStudents = atoi(tokens[3]);
                            course.noOfVacanciesLeft = atoi(tokens[4]);
                            course.timeOffered = tokens[5];
                            appendCourseToDatabase(course, new_s);
                        }
                    }
                    break;
                case DELETE:
                    printf("DELETE command issued\n");
                    if (numOfTokens > 2)
                    {
                        prependedmsg=malloc(strlen("Warning: Excess Arguments Ignored\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Warning: Excess Arguments Ignored\n");
                    }
                    else if (numOfTokens<2)
                    {
                        prependedmsg=malloc(strlen("Error: Not Enough Argument(s)\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Error: Not Enough Argument(s)\n");
                        msg="";
                        displayAndSendMessage(new_s, msg);
                    }
                    else
                    {
                        deleteCourse(new_s, tokens[1]);
                    }
                    break;
                case HELP:
                    printf("HELP command issued\n");
                    if (numOfTokens > 1)
                    {
                        prependedmsg=malloc(strlen("Warning: Excess Arguments Ignored\n")+1);
                        memset(prependedmsg ,0 , sizeof(prependedmsg));
                        emptyString(prependedmsg);
                        strcat(prependedmsg,"Warning: Excess Arguments Ignored\n");
                    }
                    msg = "Usage:\n<Command>[|<arguments>]\n\n"
                            "1. ADD | <course id> | <course name> | <No of Registered Students> | <No of vacancies left> | <Time Offered>\n"
                            "Eg. ADD | CS1234 | Data Structures | 31 | 9 | FALL 2017\n\n"
                            "2. SEARCH/FIND | <course id/course name/time offered>\n"
                            "Eg. FIND | CS1234\n\n"
                            "3. LIST/LIST ALL\n\n"
                            "4. DELETE | <course name>\n\n"
                            "Eg. DELETE | Data Structures\n\n"
                            "5. HELP for Help\n\n"
                            "6. EXIT to Exit";
                    displayAndSendMessage(new_s, msg);
                    break;
                default:
                    msg = "Error: Unrecognized Command\nUsage:\n<Command>[|<arguments>]\nHELP for Help\nEXIT to Exit";
                    displayAndSendMessage(new_s, msg);
            }
            memset(buf ,0 , sizeof(buf));
            emptyString(buf);
            memset(prependedmsg ,0 , sizeof(prependedmsg));
            emptyString(prependedmsg);
        }
        close(new_s);
    }
}