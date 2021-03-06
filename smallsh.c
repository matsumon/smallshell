#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int forkCount = 0;
pid_t holder[512];
int parent;
int priority;
pid_t id = -100;
int recent_process = -10;
int * exitMethod;
volatile sig_atomic_t background_toggle=0;

void getInput();
void handleInput(char *);
void shellExit();
void shellCD(char *);
int expansion(char*);
void shellStatus();
void childStatus(int *);
int findAnd(char*);
void execute(char *, int);
void redirection(char *, int);
void signalStop(int);
void signalTerm(int);
int checkForBlank(char *);

int main()
{
	struct sigaction sigStpHandler ={0};
	sigStpHandler.sa_handler=signalStop;
	sigemptyset(&sigStpHandler.sa_mask);
	sigStpHandler.sa_flags=0;
	sigaction(SIGTSTP,&sigStpHandler,NULL);
	struct sigaction sigIntHandler={0};
	sigIntHandler.sa_handler=SIG_IGN;
	sigemptyset(&sigIntHandler.sa_mask);
	sigaddset(&sigIntHandler.sa_mask,SIGINT);
	sigaction(SIGINT,&sigIntHandler,NULL);
	memset(holder,0,sizeof(holder[0]));
	parent = getpid();
	while(1)
	{

	//	printf("Main Parent: %d\nCurrent: %d\n",parent,getpid());
	//	fflush(stdout);
		int i = 0;
	//	printf("pid background: %d\n",holder[1]);
	//	fflush(stdout);
	/*	while(holder[i] != 0)
		{
			printf("pid background: %d\n",holder[i]);
			fflush(stdout);
			i++;
		}*/
		if(parent == getpid())
		{
			/*if(priority == 1)
			{
				waitpid(recent_process,&exitMethod,0);
			}
			if(WIFSIGNALED(exitMethod)!=0 && priority == 1)
			{
				int z;
				printf("terminated by signal %d\n",WTERMSIG(exitMethod));
				fflush(stdout);
			}*/

			int * childExitMethod=0;
			int childpid = -1;
			childpid=waitpid(-1,&childExitMethod,WNOHANG);
			if(childpid > 0)
			{
				printf("Child PID: %d has finished. \n ",childpid);
				fflush(stdout);
				childStatus(childExitMethod);
			}	
			clearerr(stdin);
			fflush(stdout);
			printf(": ");
			fflush(stdout);
			clearerr(stdin);
			getInput();
			if(id > 0 && priority == 0)
			{
				printf("Child PID: %d was created. \n",id);
				fflush(stdout);
			}
		}
		else
		{
			break;
		}
	}
	return 0;
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
	if(input==NULL)
	{
		return;
	}
	char inputCopy [2048];
	char inputCopy2 [2048];
	//	pid_t id = -100;
	int check =1;
	strcpy(inputCopy2,input);
	if(checkForBlank(inputCopy2) == 0)
	{
		return;
	}

	while(check==1)
	{
		check =expansion(inputCopy2);
		fflush(stdout);
	}
	strcpy(inputCopy,inputCopy2);
	inputCopy[strlen(inputCopy)-1]=0;
	//printf("%s\n",inputCopy);
	//fflush(stdout);
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
		if(parent == getpid())
		{
			priority = findAnd(inputCopy); 	
			if(background_toggle == 1)
			{
				priority = 1;
			}
			if(forkCount < 100)
			{
				forkCount++;
				id = fork();	
				int current = getpid();
				if(priority == 1 && id )
				{
					recent_process = id;
					//	wait(id);
					//	printf("THINGS\n");
					//	fflush(stdout);
					if(priority == 1)
					{
						waitpid(id ,&exitMethod, 0);
						if(WIFSIGNALED(exitMethod) !=0 )
						{
							int z;
							printf("terminated by signal %d\n",WTERMSIG(exitMethod));
							fflush(stdout);
						}
					}

				}
				int i = 0;
				if( priority == 0 && parent == current)
				{
					while(holder[i] != 0)
					{
						i++;
					}
					holder[i] = id;
				}
			}
			else
			{
				printf("Too many forks");
				fflush(stdout);
				return;
			}
			if(getpid() != parent)
			{
				execute(inputCopy,priority);	
				printf("Couldnt find command\n");
				fflush(stdout);
				exit(1);
				return;
			}
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
		return;
	}

	if(errno != 10)
	{
		printf("a exit value 1\n");
		fflush(stdout);
		return;
	}
	else
	{
		int i;
		if(i=WIFEXITED(exitMethod)!=0)
		{
			printf("exit value: %d\n",WEXITSTATUS(exitMethod));
			fflush(stdout);
		} 
		else
		{
			i = WIFSIGNALED(exitMethod);
			printf("terminated by signal %d\n",WTERMSIG(exitMethod));
			fflush(stdout);
		}
		return;
	}
}

void childStatus(int * childExitMethod)
{
	int i;
	if(i=WIFEXITED(childExitMethod)!=0)
	{
		printf("child exit value %d\n",i);
		fflush(stdout);
	} 
	else
	{
		i = WIFSIGNALED(childExitMethod);
		printf("child terminated by signal %d\n",WTERMSIG(childExitMethod));
		fflush(stdout);
	}



}

int findAnd(char input [2048])
{
	if(input[strlen(input)-1] == '&' && input[strlen(input)-2]==' ')
	{
		input[strlen(input)-2] ='\0';
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
	if(priority ==1)
	{
		struct sigaction sigIntHandler={0};
		sigIntHandler.sa_handler=SIG_DFL;
		sigemptyset(&sigIntHandler.sa_mask);
		sigaddset(&sigIntHandler.sa_mask,SIGINT);
		sigaction(SIGINT,&sigIntHandler,NULL);

	}
	int t = execvp(temp1[0],temp1);
}

void redirection(char input[2048],int priority)
{
	int i;
	int input_num = -1;
	int inputFlip = -1;
	int outputFlip = -1;
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
			inputFlip=1;
		}
		if(input[i] == '>')
		{
			output = i;
			outputFlip=1;
		}
	}
	int q = 0;
	if(input_num != -1)
	{
		for(i = input_num+2; i < strlen(input); i++)
		{
			if(input[i] != ' ' && input[i] != '\0' )
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
	//	printf("inputfile %s\n",input_file);	
	//	fflush(stdout);
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
	//	printf("outputfile %s\n",output_file);	
	//	fflush(stdout);
	//	printf("input output flipeed %d %d\n",inputFlip,outputFlip);	
	//	fflush(stdout);
	//	printf("input output nums %d %d\n",input_num,output);	
	//	fflush(stdout);
	char endInput [2048];
	if(outputFlip == 1)
	{
		strcpy(endInput,input + output + 2);	
		if(strlen(endInput) != strlen(output_file))
		{
			strcpy(input+output,endInput+strlen(output_file)+1);
		}	
		else
		{
			input[output -1] = '\0';
		}
	}
	char endInput2 [2048];
	if(inputFlip == 1)
	{
		strcpy(endInput2,input + input_num + 2);	
		if(strlen(endInput2) != strlen(input_file))
		{
			strcpy(input+input_num,endInput2+strlen(input_file)+1);
		}	
		else
		{
			input[input_num -1] = '\0';
		}
	}
	//	printf("priority: %d %d\n", priority, outputFlip);	
	//	fflush(stdout);
	if(inputFlip == -1 && priority == 0)
	{
		//		printf("BackGround Process Taking Input From A Blackhole\n");
		//		fflush(stdout);
		int fd = open("/dev/null",O_WRONLY | O_CREAT,0020);
		dup2(fd,0);
	}

	if(outputFlip == -1 && priority == 0)
	{
		//		printf("BackGround Process Screaming Into Blackhole\n");
		//		fflush(stdout);
		int fd = open("/dev/null",O_WRONLY | O_CREAT,0020);
		dup2(fd,1);
	}

	if(outputFlip == 1)
	{
		int fd = open(output_file,O_WRONLY | O_CREAT,0600);
		//printf("fd = %d\n",fd);
		if(fd < 0)
		{
			printf("Couldn't Open File");
			fflush(stdout);
			return;
		}
		dup2(fd,1);
	}
	/*	printf("Input:%s\n", input);
		fflush(stdout);
		printf("NUMS:%d %d\n ",inputFlip,outputFlip);
		fflush(stdout);
		printf("inputfile:%s\noutputfile:%s\n", input_file,output_file);
		fflush(stdout);*/
	if(inputFlip == 1)
	{
		int fd = open(input_file,O_RDONLY,0001);
		if(fd < 0)
		{
			printf("Couldn't Open File");
			fflush(stdout);
			return;
		}
		//		printf("fd = %d\n",fd);
		//		fflush(stdout);
		dup2(fd,0);
	}

}


void signalStop(int signal)
{
	if(background_toggle == 0)
	{
		write(1,"Entering Foreground-only mode (& is now ignored)\n",50);
		background_toggle= 1;
	}
	else if(background_toggle == 1)
	{
		write(1,"Exiting Foreground-only mode",29);
		background_toggle= 0;
	}

	return;
}
void signalTerm(int signum)
{
	return;
}

int checkForBlank(char input[2048])
{
	int i = 0;
	for(i=0;i<strlen(input);i++)
	{
		if(input[i] != ' ' && input[i] !='\n')
		{
			return 1;
		}

	}
	return 0;
}
