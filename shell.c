#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define MAX 1000

int lookup_iter;

struct lookup{

	int index[1000];
	char* command[1000];
};

typedef struct lookup* lookup_table;

lookup_table Table;

int found_in_PATH(char* str,char* input)
{
	    int len;
        struct dirent *pDirent;
        DIR *pDir;

        pDir = opendir (input);
        if (pDir == NULL) {
            printf ("Error In finding in Path");
            return 1;
        }

        while ((pDirent = readdir(pDir)) != NULL) {
            //printf ("%s\n", pDirent->d_name);
            if(strcmp(pDirent->d_name,str)==0)
            	return 1;
        }
        return 0;
        closedir (pDir);

}

int parse_with_Spaces(char* str, char** parsed) 
{ 
	int i; 

	for (i = 0; i < MAX; i++) { 
		parsed[i] = strsep(&str, " "); 

		if (parsed[i] == NULL) 
			{
				break; 
			}
		if (strlen(parsed[i]) == 0) 
			i--; 
	} 
	return i;
} 


void execute(char* input[], char* path,int size)
{
	// First We need to parse the input

	char* arg[size+1];

	

	if(strcmp(input[size-1],"&")==0)
	{
		int j;
		for(j=0;j<size-1;j++)
		{
			arg[j] = input[j];
		}
		arg[j] = NULL;
		strcat(path,"/");
		strcat(path,arg[0]);
	}
	else
	{
		int i;
		for(i=0;i<size;i++)
		{
			arg[i] = input[i];
			//printf("%s\n",arg[i]);
		}
			arg[i] = NULL;
			strcat(path,"/");
			strcat(path,arg[0]);
	//printf(" The Path is %s\n",path);
    }

	pid_t p = fork();

	if(p==0)
	{
		// Creating New Process Group for the command
		// Background Process
		if(strcmp(input[size-1],"&")==0)
		{
			setpgid(0,0);
			return;
		}

		else 
		{
			setpgid(0,getpid());
	    }

		sleep(1);
		execv(path,arg);

		exit(0);
	}
	else
	{
		//setpgid(0,0);
		if(strcmp(input[size-1],"&")==0)
		{
			// Send The Process In Background
			printf("Sending The Process in Background\n");

		}
		else
		{
			tcsetpgrp(STDIN_FILENO,p);
			tcsetpgrp(STDOUT_FILENO,p);
			tcsetpgrp(STDERR_FILENO,p);
			printf("Sending the Process in foreground\n");
			wait(NULL);

			// Put the shell Back in foreground
			tcsetpgrp(STDIN_FILENO,getpid());
			tcsetpgrp(STDOUT_FILENO,getpid());
			tcsetpgrp(STDERR_FILENO,getpid());
		}

	}

}

void cd_handler(char* input[])
{
		chdir(input[1]);
		printf("\n");
}

void redirection_handler(char* input[],char* path,char* red_op,int index)
{

	if(strcmp(red_op,"<")==0)
	{	

  		int fd, sz; 
  		char *c = (char *) calloc(100, sizeof(char)); 
  
  		fd = open(input[index+1],O_RDONLY); 

  		if (fd < 0) { 

  		 printf("Error in Opening File\n");
  		 exit(1); 

  		}
  
  		sz = read(fd, c, 100); 

  		c[sz] = '\0'; 

  		char* arg[MAX];

  		printf("%s\n",c);


  		for(int j=0;j<index;j++)
  		{
  			arg[j] = malloc(sizeof(char)*MAX);
  			strcpy(arg[j],input[j]);
  		}

  		char* parse[MAX];
  		int len = parse_with_Spaces(c,parse);
  		int i;
  		int k = 0;
  		for(i=index;i<index+len;i++)
  		{
  			arg[i] = malloc(sizeof(char)*MAX);
  			strcpy(arg[i],parse[k++]);
  		}

  		execute(arg,path,index+len);
 

	}
	else if(strcmp(red_op,">")==0 || strcmp(red_op,">>")==0)
	{
		int saved;
		saved = dup(1);

		int file_desc;

		if(strcmp(red_op,">")==0)
		file_desc = open(input[index+1],O_WRONLY);

		else if(strcmp(red_op,">>")==0)
		{
			file_desc = open(input[index+1],O_WRONLY | O_APPEND);
		}

		if(file_desc==-1)
		{
			printf("Error In reading the file\n");
			exit(0);
		}

		dup2(file_desc,1);

		char* arg[MAX];

		for(int j=0;j<index;j++)
		{
			arg[j] = (char*)malloc(sizeof(char)*MAX);
		}
		int i;
		for(i=0;i<index;i++)
		{
			strcpy(arg[i],input[i]);
		}

		execute(arg,path,index);

		// Restoring the output to stdout

		dup2(saved,1);
		close(saved);
	}
}


char* read_line(char *buffer)
{
  int bufsize = BUFSIZE;
  int position = 0;
  buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "error\n");
    exit(0);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') 
    {
      buffer[position] = '\0';
      return buffer;
    } 
    else 
    {
      buffer[position] = c;
    }

    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "error\n");
        exit(0);
      }
    }
  }
}

void printAllShortcuts(lookup_table t,int size)
{
	printf("Shortcut Commands supported :\n");
	for(int i=0;i<size;i++)
	{
		if(t->index[i]!=-1)
		printf("%s\n",t->command[i]);
	}
}

void short_cut_execution(int number)
{
	char* string;
	string = (char*)malloc(sizeof(char)*MAX);

	char* temp_string;
	temp_string = (char*)malloc(sizeof(char)*MAX);

	

	int i;
	for( i=0;i<MAX;i++)
	{
		if(Table->index[i]==number)
		{
			strcpy(string,Table->command[i]);
			break;
		}
	}

	strcpy(temp_string,string);

	char* s = getenv("PATH");

	char* str_path = (char*)malloc(sizeof(char)*MAX);
	strcpy(str_path,s);

	char* temp_arg[MAX];
	char* parsed[MAX];

	int size = parse_with_Spaces(temp_string,temp_arg);

	int j;

	for(int k=0;k<size-3;k++)
	{
		parsed[k] = (char*)malloc(sizeof(char)*MAX);
		strcpy(parsed[k],temp_arg[k+3]);
	}

	//
	if(strcmp(parsed[0],"cd")==0)
	{
		cd_handler(parsed);
		char cwd[MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) 
		{
       		printf("%s/\n", cwd);
   		}
   		return;
	}
	//
	char* temp;

	size = size-3;

	while((temp=strsep(&str_path,":"))!=NULL)
	{
		//printf("%s\n",temp);
		if(found_in_PATH(parsed[0],temp))
			{

				char* read_op = "No_Redirection";

				int i;
				for(i=0;i<size;i++)
				{
					if(strcmp(parsed[i],"<")==0)
					{
						read_op = "<";
						break;
					}
					else if(strcmp(parsed[i],">")==0)
					{
						read_op = ">";
						break;
					}
					else if(strcmp(parsed[i],">>")==0)
					{
						read_op = ">>";
						break;
					}

				}

				if(strcmp(read_op,"No_Redirection")==0)
					execute(parsed,temp,size);
				else
					redirection_handler(parsed,temp,read_op,i);
				
				printf("\n");
				break;
			}
	}


}

void handle_sigint(int sig)
{
	int number;
	printf("Enter The Number\n");
	scanf("%d",&number);
	
	short_cut_execution(number);

	exit(0);

}

int parsePipe(char* str, char** strpiped) 
{ 
	int i; 
	for (i = 0; i < MAX; i++) { 
		strpiped[i] = strsep(&str, "|"); 
		if (strpiped[i] == NULL) 
			break; 
	} 

	if (strpiped[1] == NULL) 
		return 0; // returns zero if no pipe is found. 
	else { 
		return i; 
	} 
} 
void pipe_handler(char** pipeParsed,int index)
{
	char* s= getenv("PATH");

				char* parsed[MAX];

				char* str = (char*)malloc(sizeof(char)*MAX);

				strcpy(str,pipeParsed[index]);

				int size = parse_with_Spaces(str,parsed);

				// Find The Path

				char* str_path = (char*)malloc(sizeof(char)*MAX);

				strcpy(str_path,s);

				char* temp;

  while((temp=strsep(&str_path,":"))!=NULL)
	{
		//printf("%s\n",temp);
		if(found_in_PATH(parsed[0],temp))
			{
				// for(int i=0;i<size;i++)
				// 	printf("%s ",parsed[i]);

				char* read_op = "No_Redirection";

				int i;
				for(i=0;i<size;i++)
				{
					if(strcmp(parsed[i],"<")==0)
					{
						read_op = "<";
						break;
					}
					else if(strcmp(parsed[i],">")==0)
					{
						read_op = ">";
						break;
					}
					else if(strcmp(parsed[i],">>")==0)
					{
						read_op = ">>";
						break;
					}

				}

				if(strcmp(read_op,"No_Redirection")==0)
					execute(parsed,temp,size);
				else
					redirection_handler(parsed,temp,read_op,i);
				
				break;
			}
	}

}

void execute_with_pipes(int pipes,char** pipeParsed)
{
	int total = pipes;

	pipes = pipes-1;

	int pip[pipes][2];

	char* s= getenv("PATH");

	for(int j=0;j<pipes;j++)
	{
		if(pipe(pip[j]) < 0)
		{
			printf("Error in creating pipe\n");
			continue;
		}
	}

	int i;

	for(i=0;i<total;i++)
	{
		pid_t p = fork();

		if(p==0)
		{
			// Children

			if(i==0)
			{
				close(pip[i][0]);
				dup2(pip[i][1],STDOUT_FILENO);

				close(pip[i][1]);

				// Execute
				
				pipe_handler(pipeParsed,i);


				exit(0);
			}

			else if(i==total-1)
			{
				close(pip[i][1]);
				dup2(pip[i][0],STDIN_FILENO);

				close(pip[i][0]);

				//
				// Execute

				pipe_handler(pipeParsed,i);

				exit(0);
			}

			else
			{
				close(pip[i-1][1]);
				dup2(pip[i-1][0],STDIN_FILENO);

				close(pip[i-1][0]);

				//

				close(pip[i][0]);
				dup2(pip[i][1],STDOUT_FILENO);

				close(pip[i][1]);

				//

				pipe_handler(pipeParsed,i);

				exit(0);
				// Execute
			}

			break;
		}
		else
		{
			// Parent


			continue;
		}
	}


	if(i==total)
	{
		// Parent
		for(int k=0;k<total;k++)
		{
			wait(NULL);
		}
	}
	// Work Here

}
void execute_pipe_handler(char** parsed,int size)
{
	char* s = getenv("PATH");

	char* str_path = (char*)malloc(sizeof(char)*MAX);

	strcpy(str_path,s);

	char* temp;
	while((temp=strsep(&str_path,":"))!=NULL)
	{
		//printf("%s\n",temp);
		if(found_in_PATH(parsed[0],temp))
			{
				// for(int i=0;i<size;i++)
				// 	printf("%s ",parsed[i]);

				char* read_op = "No_Redirection";

				int i;
				for(i=0;i<size;i++)
				{
					if(strcmp(parsed[i],"<")==0)
					{
						read_op = "<";
						break;
					}
					else if(strcmp(parsed[i],">")==0)
					{
						read_op = ">";
						break;
					}
					else if(strcmp(parsed[i],">>")==0)
					{
						read_op = ">>";
						break;
					}

				}

				if(strcmp(read_op,"No_Redirection")==0)
					execute(parsed,temp,size);
				else
					redirection_handler(parsed,temp,read_op,i);
				
				break;
			}
	}



}
void execute_double(char* string,int index)
{
	// Function to execute commands with double pipes
	char* str1;
	char* str2;
	char* str3;

	str1 = (char*)malloc(sizeof(char)*MAX);
    str2 = (char*)malloc(sizeof(char)*MAX);
    str3 = (char*)malloc(sizeof(char)*MAX);

    int n = strlen(string);

    int i;
    for(i=0;i<index;i++)
    {
    	str1[i] = string[i];
    }

    str1[i] = '\0';

    int j,c=0;
    for(j=index+2;j<n;j++)
    {
    	if(string[j]==',')
    	{
    		break;
    	}

    	str2[c++] = string[j]; 
    }
    str2[c] = '\0';

    int k;
    c=0;
    for(k=j+1;k<n;k++){
    	str3[c++]=string[k];
    }
    str3[c]='\0';


    //printf("double %s %s %s\n", str1,str2,str3);

    char* parsed1[MAX];

    char* parsed2[MAX];

    char* parsed3[MAX];

    int size1 = parse_with_Spaces(str1,parsed1);
    int size2 = parse_with_Spaces(str2,parsed2);
    int size3 = parse_with_Spaces(str3,parsed3);

    int pip[2];

   	if(pipe(pip)<0)
   		printf("Error in Pipe Creation\n");

   

   	int t;
 	for(t = 0;t<3;t++)
 	{
 	    pid_t p = fork();

 	    if(p==0)
 	    {
 	    	// Child
 	    	if(t==0)
 	    	{
 	    		close(pip[0]);
 	    		dup2(pip[1],STDOUT_FILENO);
 	    		close(pip[1]);

 	    		// Execute Parse1
 	    		
 	    		execute_pipe_handler(parsed1,size1);


 	    		exit(0);
 	    	}

 	    	else
 	    	{
 	    		close(pip[1]);
 	    		dup2(pip[0],STDIN_FILENO);
 	    		close(pip[0]);

 	    		if(t==1)
 	    		{
 	    			// Execute Parse2
 	    			execute_pipe_handler(parsed2,size2);
 	    			exit(0);
 	    		}
 	    		else if(t==2)
 	    		{
 	    			// Execute Parse3
 	    			execute_pipe_handler(parsed3,size3);
 	    			exit(0);
 	    		}
 	    	}
 	    	exit(0);
 	    }
 	    else
 	    {
 	    	continue;
 	    }

 	}

 	 	wait(NULL);
 	    wait(NULL);
 	    wait(NULL);

}

void execute_triple(char* string,int index)
{
	// Function to execute commands with triple pipes
	char* str1;
	char* str2;
	char* str3;
	char* str4;

	str1 = (char*)malloc(sizeof(char)*MAX);
    str2 = (char*)malloc(sizeof(char)*MAX);
    str3 = (char*)malloc(sizeof(char)*MAX);
    str4 = (char*)malloc(sizeof(char)*MAX);


    int n = strlen(string);

    int i;
    for(i=0;i<index;i++)
    {
    	str1[i] = string[i];
    }

    str1[i] = '\0';

    int j,c=0;
    for(j=index+3;j<n;j++)
    {
    	if(string[j]==',')
    	{
    		break;
    	}

    	str2[c++] = string[j]; 
    }
    str2[c] = '\0';

    int k;
    c=0;
    for(k=j+1;k<n;k++){
    	if(string[k]==','){
    		break;
    	}
    	str3[c++]=string[k];
    }
    str3[c]='\0';

    c=0;
    for(j=k+1;j<n;j++){
    	str4[c++]=string[j];
    }
    str4[c]='\0';

    char* parsed1[MAX];

    char* parsed2[MAX];

    char* parsed3[MAX];

    char* parsed4[MAX];

    int size1 = parse_with_Spaces(str1,parsed1);
    int size2 = parse_with_Spaces(str2,parsed2);
    int size3 = parse_with_Spaces(str3,parsed3);
    int size4 = parse_with_Spaces(str4,parsed4);

     int pip[2];

   	if(pipe(pip)<0)
   		printf("Error in Pipe Creation\n");

   	  	int t;
 	for(t = 0;t<4;t++)
 	{
 	    pid_t p = fork();

 	    if(p==0)
 	    {
 	    	// Child
 	    	if(t==0)
 	    	{
 	    		close(pip[0]);
 	    		dup2(pip[1],STDOUT_FILENO);
 	    		close(pip[1]);

 	    		// Execute Parse1
 	    		
 	    		execute_pipe_handler(parsed1,size1);


 	    		exit(0);
 	    	}

 	    	else
 	    	{
 	    		close(pip[1]);
 	    		dup2(pip[0],STDIN_FILENO);
 	    		close(pip[0]);

 	    		if(t==1)
 	    		{
 	    			// Execute Parse2
 	    			execute_pipe_handler(parsed2,size2);
 	    			exit(0);
 	    		}
 	    		else if(t==2)
 	    		{
 	    			// Execute Parse3
 	    			execute_pipe_handler(parsed3,size3);
 	    			exit(0);
 	    		}
 	    		else if(t==3)
 	    		{
 	    			execute_pipe_handler(parsed4,size4);
 	    			exit(0);
 	    		}
 	    	}
 	    	exit(0);
 	    }
 	    else
 	    {
 	    	continue;
 	    }

 	}

 	 	wait(NULL);
 	    wait(NULL);
 	    wait(NULL);
 	    wait(NULL);
    //printf("%s %s %s %s\n", str1,str2,str3,str4);


}

int isDouble(char* string)
{
	int n = strlen(string);

	for(int i=0;i<n-2;i++)
	{
		if(string[i]=='|' && string[i+1]=='|' && string[i+2]!='|')
			return i;
	}

	return 0;
}

int isTriple(char* string)
{

	int n = strlen(string);

	for(int i=0;i<n-2;i++)
	{
		if(string[i]=='|' && string[i+1]=='|' && string[i+2]=='|')
			return i;
	}

	return 0;
}



int main(int argc, char *argv[])
{

	lookup_iter = 0;

	char* string;

	char* parsed[MAX];

	char* pipeParsed[MAX];

	char* s = getenv("PATH");

	lookup_table lookup;

	lookup = (lookup_table)malloc(sizeof(struct lookup));

	Table = lookup;

	while(1)
	{

		signal(SIGINT, handle_sigint);
		char cwd[MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) 
		{
       		printf("%s/\n", cwd);
   		}

   		printAllShortcuts(lookup,lookup_iter);

   	printf("Process ID - %d\n",getpid());

   	printf("Enter Your Command Here: ");

   	int redirection_bit = 0;

	string = read_line(string);
	// Taking The Input

	if(strcmp(string,"exit")==0)
	{
		exit(0);
	}

	int id = isDouble(string);
	int it = isTriple(string);

	if(it>0)
	{
		// Execute with triple pipes
		execute_triple(string,it);
		continue;
	}

	if(id>0)
	{
		// Execute with double pipes case

		execute_double(string,id);
		continue;
	}

	



	char* temp_string = (char*)malloc(sizeof(char)*MAX);
	strcpy(temp_string,string);

	char* t_string = (char*)malloc(sizeof(char)*MAX);
	strcpy(t_string,string);

	int pipes = parsePipe(t_string,pipeParsed);

	if(pipes)
	{
		printf("No. of Pipes = %d\n",pipes-1);

		execute_with_pipes(pipes,pipeParsed);

		continue;
		//exit(0);
	}
	int size = parse_with_Spaces(temp_string,parsed);

	// Handling The ShortCut Commands
    if(strcmp(parsed[0],"sc")==0 && strcmp(parsed[1],"-i")==0)
    {
    	// Short Cut Command to be added
    	lookup->index[lookup_iter] = atoi(parsed[2]);

    	lookup->command[lookup_iter] = (char*)malloc(sizeof(char)*1000);

    	strcpy(lookup->command[lookup_iter],string);

    	printf("Added\n");
    	lookup_iter++;
    	continue;
    }

    else if(strcmp(parsed[0],"sc")==0 && strcmp(parsed[1],"-d")==0)
    {
    	// Short Cut Command to be deleted

    	int ind = atoi(parsed[2]);

    	for(int i=0;i<MAX;i++)
    	{
    		if(lookup->index[i]==ind)
    		{
    			lookup->index[i] = -1;
    			lookup->command[i] = "NULL";
    			break;
    		}
    	}

    	continue;
    }


	char* str_path = (char*)malloc(sizeof(char)*MAX);
	strcpy(str_path,s);

	if(strcmp(parsed[0],"cd")==0)
	{
		cd_handler(parsed);
		continue;
	}

	char* temp;

	while((temp=strsep(&str_path,":"))!=NULL)
	{
		//printf("%s\n",temp);
		if(found_in_PATH(parsed[0],temp))
			{
				// for(int i=0;i<size;i++)
				// 	printf("%s ",parsed[i]);

				char* read_op = "No_Redirection";

				int i;
				for(i=0;i<size;i++)
				{
					if(strcmp(parsed[i],"<")==0)
					{
						read_op = "<";
						break;
					}
					else if(strcmp(parsed[i],">")==0)
					{
						read_op = ">";
						break;
					}
					else if(strcmp(parsed[i],">>")==0)
					{
						read_op = ">>";
						break;
					}

				}

				if(strcmp(read_op,"No_Redirection")==0)
					execute(parsed,temp,size);
				else
					redirection_handler(parsed,temp,read_op,i);
				
				break;
			}
	}

	printf("\n");



	}

	return 0;
}