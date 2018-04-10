#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLINE 1024
#define MAXARG 20
#define MAXPIPE 6
#define MAXNAME 100
#define MAXOPEN 1024

typedef struct command
{
  char *args[MAXARG+1];
  int infd;
  int outfd;
}COMMAND;

char cmdline[MAXLINE+1];
char argline[MAXLINE+1];
char *linep;
char *argp;
COMMAND cmd[MAXPIPE];
char infile[MAXNAME+1];
char outfile[MAXNAME+1];

int total_cmd;
int append;
int lastpid;

int has(char *str)
{
  while(*linep==' ' || *linep=='\t')
    linep++;

  char *p=linep;
  while(*str!='\0' && *str==*p)
  { str++;p++;}

  if(*str=='\0')
  {
    linep=p;
    return 1;
  }
  return 0;
}

void get_command(int i)
{
  int j = 0;
  int hasword;
  while (*linep != '\0')
  {
    while (*linep == ' ' || *linep == '\t')
      linep++;

    cmd[i].args[j] = argp;

  while (*linep != '\0' && *linep != ' ' && *linep != '\t' && *linep != '>'
      && *linep != '<'  && *linep != '|'  && *linep != '\n')
  {
      *argp++ = *linep++;
      hasword = 1;
  }
  *argp++ = '\0';
  switch (*linep)
  {
    case ' ':
    case '\t': hasword = 0;j++;break;
    case '<':
    case '>':
    case '|':
    case '\n':
      if (hasword == 0)
        cmd[i].args[j] = NULL;
        return;
    default:return;
   }
 }
}

int builtin(void)
{
  int i=0;
  int found=0;

  if(has("cd"))
  {
    found=1;
    get_command(0);
    int fd;
    fd=open(*(cmd[0].args),O_RDONLY);
    fchdir(fd);
    close(fd);
  }
  else if(has("export"))
  {
    found=1;
    get_command(0);
		int i=0;
		while(cmd[0].args[i])
		{
			char name[20];
			char value[128];
			int j;
			char *p=cmd[0].args[i];
			for(j=0;*p!='=';p++,j++)
				name[j]=*p;
			name[j]='\0';
			for(j=0,p++;*p;p++,j++)
				value[j]=*p;
			value[j]='\0';

			setenv(name,value,1);
			i++;
		}
  }
  else if(has("exit"))
  {
    found=1;
    exit(EXIT_SUCCESS);
  }
  return found;
}

void filename(char *name)
{
	while (*linep == ' ' || *linep == '\t')
		linep++;

	while (*linep != '\0'	&& *linep != ' '	&& *linep != '\t'	&& *linep != '>'
      && *linep != '<'	&& *linep != '|'	&& *linep != '\n')
	{
			*name++ = *linep++;
	}
	*name = '\0';
}

void parse(void)
{
  if(has("\n")||builtin())
    return;

    get_command(0);
  	if (has("<"))
  		filename(infile);
  	int i;
  	for (i=1; i<MAXPIPE; i++)
  	{
  		if (has("|"))
  			get_command(i);
  		else
  			break;
  	}
  	if (has(">"))
  	{
  		if (has(">"))
  			append = 1;
  		filename(outfile);
  	}
  	if (has("\n"))
  	{
  		total_cmd = i;
  		return;
  	}
  	else
  	{
  		fprintf(stderr, "unknown command\n");
  		return;
  	}
}

void excute(void)
{
  if (total_cmd == 0)
		return;
	if (infile[0] != '\0')
		cmd[0].infd = open(infile, O_RDONLY);
	if (outfile[0] != '\0')
	{
		if (append)
			cmd[total_cmd-1].outfd = open(outfile, O_WRONLY | O_CREAT
				| O_APPEND, 0666);
		else
			cmd[total_cmd-1].outfd = open(outfile, O_WRONLY | O_CREAT
				| O_TRUNC, 0666);
	}

	int i,fd,fds[2];
	for (i=0; i<total_cmd;i++)
	{
		if (i<total_cmd-1)
		{
			pipe(fds);
			cmd[i].outfd = fds[1];
			cmd[i+1].infd = fds[0];
		}

    pid_t pid;
    pid = fork();
    if (pid > 0)
      lastpid = pid;
    else if (pid == 0)
    {
      if (cmd[i].infd != 0)
        dup2(cmd[i].infd,0);
      if (cmd[i].outfd != 1)
        dup2(cmd[i].outfd,1);
      int j;
      for (j=3; j<MAXOPEN; j++)
        close(j);
      execvp(cmd[i].args[0], cmd[i].args);
      exit(EXIT_FAILURE);
    }

		if ((fd = cmd[i].infd) != 0)
			close(fd);
		if ((fd = cmd[i].outfd) != 1)
			close(fd);
	}
	while (wait(NULL) != lastpid);
	return;
}

void init(void)
{
  memset(cmd,0,sizeof(cmd));
  //default in 0,out 1
  int i;
  for(i=0;i<MAXPIPE;i++)
  {
    cmd[i].infd=0;
    cmd[i].outfd=1;
  }
  memset(cmdline,0,sizeof(cmdline));
  memset(argline,0,sizeof(argline));
  linep=cmdline;
  argp=argline;
  memset(infile,0,sizeof(infile));
  memset(outfile,0,sizeof(outfile));
  total_cmd=append=lastpid=0;

    char pathbuf[64];
    getcwd(pathbuf, sizeof(pathbuf));
    printf("\e[33m%s$\e[0m ", pathbuf);
  fflush(stdout);

  fgets(cmdline,MAXLINE,stdin);
}

int main()
{
  while(1)
  {
    init();
    parse();
    excute();
  }
  return 0;
}
