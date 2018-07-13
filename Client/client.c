#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#define SERVER_PORT 5438
#define MAX_LINE 256

void emptyString(char *str)
{
    int i=0;
    for (i=0;i<strlen(str);i++)
        str[i]='\0';
}

int getMessageLength(char *msg)
{
    int i=0;
    int index;
    char *msgLength;
    while(msg[i]!='|')
        i++;

    msgLength=malloc(i+1);
    for (index=0;index<i;index++)
        msgLength[index]=msg[index];
    msgLength[index]='\0';

    return atoi(msgLength);
}

void trimToActualMessageOnly(char* msg)
{
    int i=0;
    int j=0;
    int index;
    char *msgtemp;
    while(msg[i]!='|')
        i++;

    msgtemp=malloc(strlen(msg)-i);
    while(msgtemp==NULL)
        msgtemp=malloc(strlen(msg)-i);

    for (index=(i+1);index<strlen(msg);index++)
    {
        msgtemp[j] = msg[index];
        j++;
    }
    msgtemp[j]='\0';

    memset(msg ,0 , sizeof(msg));
    emptyString(msg);

    strcpy(msg,msgtemp);
}

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

void convertStringToUpperCase(char *str)
{
    int i = 0;
    while (str[i])
    {
        str[i] = toupper(str[i]);
        i++;
    }
}

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

int main(int argc, char * argv[])
{
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    int s;
    int len;
    if (argc==2)
    {
        host = argv[1];
    }
    else
    {
        fprintf(stderr, "usage: simplex-talk host\n");
        exit(1);
    }

    /* translate host name into peer�s IP address */
    hp = gethostbyname(host);
    if (!hp)
    {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    /* active open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }
    else
    {
        //get welcome message
        int firsttime=0;
        int msgLength=0;
        memset(buf ,0 , sizeof(buf));
        emptyString(buf);
        int totalreceived=0;
        while (len=recv(s, buf, sizeof(buf), 0))
        {
            if (firsttime==0)
            {
                msgLength=getMessageLength(buf);
                trimToActualMessageOnly(buf);
                firsttime++;
            }
            fputs(buf, stdout);
            memset(buf ,0 , sizeof(buf));
            emptyString(buf);
            totalreceived+=len;
            if (totalreceived==msgLength)
                break;
        }
    }

    /* main loop: get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin))
    {
        char **tokens;
        int numOfTokens;

        buf[MAX_LINE-1] = '\0';
        len = strlen(buf) + 1;

        numOfTokens=getNumOfTokens(buf);
        tokens=tokenize(buf);

        trim(tokens[0]);
        convertStringToUpperCase(tokens[0]);

        if (strncmp(tokens[0],"EXIT",strlen("EXIT"))==0)
        {
            if (numOfTokens>1)
                printf("\nWarning: Excess Arguments Ignored");
            printf("\nExiting...\n");
            break;
        }
        else
        {
            if (send(s, buf, len, 0) > 0)
            {
                int firsttime = 0;
                int msgLength = 0;
                memset(buf, 0, sizeof(buf));
                emptyString(buf);
                int totalreceived = 0;
                while (len = recv(s, buf, sizeof(buf), 0))
                {
                    if (firsttime == 0)
                    {
                        msgLength = getMessageLength(buf);
                        trimToActualMessageOnly(buf);
                        firsttime++;
                    }
                    fputs(buf, stdout);
                    memset(buf, 0, sizeof(buf));
                    emptyString(buf);
                    totalreceived += len;
                    if (totalreceived == msgLength)
                        break;
                }
            } else {
                printf("Error sending request to server\n");
            }
        }
    }
    close(s);
}