#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

int forkCount = 0;
pid_t holder[512];
int parent;
int recent_process = -10;
int * exitMethod;

void getInput();
void handleInput(char *);
void shellExit();
void shellCD(char *);
int expansion(char*);
void shellStatus();
int findAnd(char*);
void execute(char *, int);
void redirection(char *, int);

int main()
{
	memset(holder,0,sizeof(holder[0]));
	parent = getpid();
	while(1)
	{

		if(parent == getpid())
		{
			waitpid(recent_process,&exitMethod,0);
			forkCount--;
			printf(": ");
			fflush(stdout);
			getInput();
		}
		else
		{
			break;
		}
	}
}

void getInput()
{
	char * input = NULL;
	size_t size = 2048;
	getline(&input,&size,stdin);
	handleInput(input);
}

void handleInput(char * input)
{
	char inputCopy [2048];
	char inputCopy2 [2048];
	pid_t id = -100;
	int check =1;
	strcpy(inputCopy2,input);
	while(check==1)
	{
		check =expansion(inputCopy2);
		fflush(stdout);
	}
	strcpy(inputCopy,inputCopy2);
	inputCopy[strlen(inputCopy)-1]=0;
	printf("%s\n",inputCopy);
	fflush(stdout);
	free(input);
	input = NULL;
	char * exit1 = "exit";
	char * exit2 = "exit &";
	char * status1 = "status";
	char * status2 = "status &";
	if(inputCopy[0] == '#' || strlen(inputCopy) == 0)
	{
		return;
	}
	else if((strcmp(inputCopy,exit1)==0)||strcmp(inputCopy,exit2)==0)
	{
		shellExit();
	}
	else if((inputCopy[0] == 'c' && inputCopy[1] == 'd') &&(inputCopy[2] == ' ' || inputCopy[2] == '\0'))
	{
		shellCD(inputCopy);
	}
	else if((strcmp(inputCopy,status1)==0)||strcmp(inputCopy,status2)==0)
	{
		shellStatus();
	}
	else 
	{
		int priority = findAnd(inputCopy); 	
		if(forkCount < 4)
		{
			forkCount++;
			id = fork();	
			int current = getpid();
			if(priority == 1 && parent == current)
			{
				recent_process = id;
			}
			int i = 0;
			if( priority == 0 && parent == current)
			{
				while(holder[i] != 0)
				{
					i++;
				}
				holder[i] = current;
			}
		}
		else
		{
			printf("Too many forks");
			fflush(stdout);
		}
		if(getpid() != parent)
		{
			execute(inputCopy,priority);	
		}
	}
}

void shellExit()
{
	int i = 0;
	while(holder[i]!=0)
	{
		printf("%d",holder[i]);
		fflush(stdout);
		kill(holder[i],SIGKILL);
		i++;
		printf("Killed a Child");
		fflush(stdout);
	}
	exit(0);
}

void shellCD(char input [2048])
{
	char path[2048];
	strcpy(path,input+3);
	if(path[strlen(path)-2] == ' ' && path[strlen(path)-1]=='&')
	{
		path[strlen(path)-2] = '\0';
	}
	char * home_path = getenv("HOME");
	int i;
	if(input[0] == 'c' && input[1] == 'd' && input[2] == '\0')
	{
		i=chdir(home_path);
	}
	else if(strncmp(path,home_path,strlen(home_path))==0)
	{
		i=chdir(path);
	}
	else
	{
		char home[2048];
		strcpy(home,home_path);
		home[strlen(home_path)]= '/';
		strcpy(home + strlen(home_path)+1,path);
		i = chdir(home);
	}
	if(i == 0)
	{
		char cwd[1000];
		getcwd(cwd,sizeof(cwd));
		printf("current dir: %s\n",cwd);
		fflush(stdout);
	}
	else if (i == -1)
	{
		printf("Invalid File Path\n");
		fflush(stdout);
	}
}
int expansion(char input [2048])
{
	int i=0;
	int j=1;
	int q;
	int t= 0;
	if(input[strlen(input-1)] == '\n')
	{
		input[strlen(input)-1]=0;
	}
	while(input[j]!=0)
	{
		if(input[i] == '$' && input[j] == '$')
		{
			char temp[2048];
			memset(temp,0,100);
			sprintf(temp,"%d",parent);
			char copyArray[2048];
			memset(copyArray,0,100);
			strcpy(copyArray,input);
			for(q=i; q<strlen(temp)+i; q++)
			{
				input[q]=temp[t];
				t++;
			}
			t=0;
			strcpy(input+strlen(temp)+i,copyArray+i+2);
			return 1;
		}
		i++;
		j++;
	}
	return 0;

}

void shellStatus()
{
	if(recent_process == -10)
	{
		printf("no foreground 0\n");
		fflush(stdout);
	}
	else
	{
		int i;
		if(i=WIFEXITED(exitMethod)!=0)
		{
			printf("exit value %d\n",i);
			fflush(stdout);
		} 
		else
		{
			i = WIFSIGNALED(exitMethod);
			printf("terminated by signal %d\n",i);
			fflush(stdout);
		}
	}
}

int findAnd(char input [2048])
{
	if(input[strlen(input)-1] == '&' && input[strlen(input)-2]==' ')
	{
		input[strlen(input)-1] ='\0';
		return 0;
	}
	else
	{
		return 1;
	}
}

void execute(char input[2048],int priority)
{
	char splitInput[512] [2048];
	int i = 0;
	int j= 0;
	int q = 0;
	char temp [2048];
	redirection(input,priority);
	while(i <  strlen(input)+1)
	{
		if(input[i] == ' ' || input[i]=='\0')
		{
			temp[j]='\0';
			j = 0;
			strcpy(splitInput[q],temp);
			q++;
		}		
		else
		{
			temp[j] = input[i];
			j++;
		}
		i++;
	}
	char * temp1[q];
	for(j = 0; j<q; j++)
	{
		temp1[j] = splitInput[j];
	//		printf("%s\n",splitInput[j]);
	//			fflush(stdout);
		//	printf("%d %s\n",j,temp1[j]);
	}
	temp1[q] = NULL;
	int t = execvp(temp1[0],temp1);
}

void redirection(char input[2048],int priority)
{
	int i;
	int input_num = -1;
	int output = -1;
	char input_file [2048];
	char output_file [2048];
	char holder [2048];
	char holder2 [2048];
	for(i = 0; i < strlen(input); i++)
	{
		if(input[i] == '<')
		{
			input_num = i;
		}
		if(input[i] == '>')
		{
			output = i;
		}
	}
	int q = 0;
	if(input_num != -1)
	{
		for(i = input_num+2; i < strlen(input); i++)
		{
			if(input[i] != ' ')
			{
				holder[q] = input[i];
				q++;
			}	
			else
			{
				break;
			}
		}
		holder[q]='\0';
	}
	strcpy(input_file,holder);
	q = 0;
	if(output != -1)
	{
		for(i = output+2; i < strlen(input); i++)
		{
			if(input[i] != ' ')
			{
				holder2[q] = input[i];
				q++;
			}	
			else
			{
				break;
			}
		}
		holder2[q]='\0';
	}
	strcpy(output_file,holder2);
	char temp[2048];
	strcpy(temp,input+input_num+2+strlen(input_file));	
	int r = 0;
	for( i = input_num -1; i < strlen(input)-2-strlen(input_file); i++)
	{
		input[i] = temp[r];
		r++;
	}
	for(i = 0; i < strlen(input); i++)
	{
		if(input[i] == '>')
		{
			output = i;
		}
	}
	char temp2[2048];
	strcpy(temp2,input+output+2+strlen(output_file));	
	r = 0;
	//	printf("infiles %s\n",input_file);
	//	fflush(stdout);
	//	printf("outfiles %s\n",output_file);
	//	fflush(stdout);
	if(input[output+strlen(output_file)+1]!='\0')
	{
		for( i = output -1; i < strlen(input)-2-strlen(output_file); i++)
		{
			input[i] = temp2[r];
			r++;
		}
	}
	printf("input testing");
	fflush(stdout);
	printf("input last %s\n",input);
	fflush(stdout);
	printf("outfiles:%s\n",output_file);
	fflush(stdout);
	printf("size:%d\n",strlen(output_file));
	fflush(stdout);
	int fd;
	if(output_file != '\0')	
	{
		fd = open(output_file,O_CREAT|O_TRUNC);
	}
	close(fd);
	//chmod(output_file,"0777");
	//		dup2(fd,1);



}