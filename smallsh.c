
#define  _POSIX_C_SOURCE 200809L

#include <sys/types.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <string.h> 
#include <signal.h> 
#include <fcntl.h>  
#include <stddef.h>
#include <errno.h>
#include <sys/wait.h>


#define MIN_WORDS 512


//function for replacing substrings with other substrings in a string
char *substr_rep(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);


void check_background_processes(){
    int status;
    pid_t pid;

    do {
        pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (pid > 0) {
            if (WIFEXITED(status)) {
                fprintf(stderr, "Child process %d done. Exit status %d.\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Child process %d done. Signaled %d.\n", pid, WTERMSIG(status));
                fprintf(stderr,"\nexit\n");
            } else if (WIFSTOPPED(status)) {
                kill(pid, SIGCONT);
                fprintf(stderr, "Child process %d stopped. Continuing.\n", pid);
            }
        }
    } while (pid > 0);
}


int main() {


  //need to declare variables outside loop
  char *line = NULL;
  size_t n = 0;
  char  **words=(char**)malloc(sizeof(char*)*MIN_WORDS);
  int num_words;
  //char *words[MIN_WORDS];
  //int num_words =0;
  pid_t pid =getpid();
  char pid_str[16];
  snprintf(pid_str, sizeof(pid_str), "%d", pid);

  int dollarQuest =0;
  //memcpy(dollarQuest,pid_str,16);

  //declare $! variable
  
  int dollarEx = -1;
  
  // set IFS variable 
 char* ifs = getenv("IFS");
  if (ifs == NULL){
    ifs = "\t\n";
  }
  //fprintf(stderr,"%s", ifs);
 // get HOME env variable
 char* home = getenv("HOME");
 //run our infinite loop 
LOOP:for(;;){
    // put variables that need to reset here
    char *token;
    num_words =0;
  //check for un-waited background processes in the same process group ID
  
  //print promt to std err by expanding PS1
  char *ps1 = getenv("PS1");
  if (ps1 == NULL) {
        ps1 = "";
    }

  //check for background processes and print messages

  check_background_processes();
  fprintf(stderr, "%s", ps1);
  // read a line of input from stdin and split the words
  ssize_t line_length = getline(&line, &n, stdin);
  if (line_length == -1){
      clearerr(stdin);
      errno = 0;
      exit(1);
  }
  //-----------------------------------------------------------
  char *lineCopy = strdup(line);
  token = strtok(lineCopy, ifs);
  while (token != NULL){ 
    words[num_words++] = strdup(token);
    token = strtok(NULL,ifs);
    //fprintf(stderr, "%s", words[num_words]); 
  }
  words[num_words] = NULL;
  //fprintf(stderr,"%d",num_words);
  // free memory
  free(lineCopy);
  //--------------------------------------
  for(int i=0; i<num_words; i++){
    //exansion loop
    if (strstr(words[i],"$$") != NULL){
      //pid_t pid = getpid();
      //char pid_str[8] ={0};
      //snprintf(pid_str, sizeof(pid_str), "%d", pid);
      substr_rep(&words[i], "$$", pid_str);  
    } 
 
    if ((strlen(words[i])>1) && words[i][0] == '~' && words[i][1] == '/'){
        substr_rep(&words[i], "~", home);     
    }

    if (strstr(words[i],"$?") !=NULL){
        char dollarQuest_str[16];
        sprintf(dollarQuest_str,"%d",dollarQuest);
        substr_rep(&words[i], "$?", dollarQuest_str);
    }

    if (strstr(words[i],"$!") !=NULL){
      if(dollarEx == -1){
       substr_rep(&words[i], "$!", "");
      }
      else{
        char dollarEx_str[16];
        sprintf(dollarEx_str,"%d",dollarEx);
        substr_rep(&words[i], "$!", dollarEx_str);
      }
    }
  }


  
  int bgflag = 0;
  char *fileIn= NULL;
  char *fileOut= NULL;
  //new parsing for loop modified
  
  
  for(int i=0; i<num_words; i++){
     int ampPos = num_words-1;
      if (strcmp(words[i], "#") == 0){
        words[i] = NULL;
        ampPos = i-1;
        break;
       }
      
      else if (strcmp(words[ampPos],"&") == 0){
        words[ampPos] = NULL;
        bgflag = 1;
        break;
      }
      
      else if (strcmp(words[i],"<")==0 && fileIn == NULL){
          words[i] = NULL;
          if (words[i+1] != NULL){
          fileIn = words[i +1];
          }

          else{
          break;
          }
      }
      else if (strcmp(words[i], ">")==0 && fileOut ==NULL){
          words[i] = NULL;
          if (words[i+1] != NULL){
          fileOut = words[i +1];
          }
          else{
          break;
          }
      }
      
    }

if(words[0]==NULL){
    goto LOOP;

}
 

 if(strcmp(words[0],"exit")==0){
      fprintf(stderr, "\nexit\n");
      if(num_words > 2){
          perror("too many arguments");
      }
      if(num_words == 2){
        int exitCode = atoi(words[1]);
        exit(exitCode);
      }
      else if (num_words ==1){
      exit(dollarQuest);
      }
      break;
    }

 


if(strcmp(words[0],"cd")==0 && num_words <=2){ 
      if(num_words ==1){ 
          chdir(home);
      }
      else{
          chdir(words[1]); 
     }
      continue;
 }


   int childStatus;
   pid_t spawnPid = fork();

  switch(spawnPid){
	case -1:
		perror("fork()\n");
		exit(3);
		//break;
	case 0:
		// In the child process
		// Replace the current program with "/bin/ls"
	  //printwords(words,num_words);
    
    if(fileIn != NULL){
     int fd = open(fileIn,O_RDONLY);

     if(fd==-1){
      printf("Error: could not open file");

      break;
     }

     if(dup2(fd, STDIN_FILENO) == -1){
      close(fd);
      break;
     }
    }


    if(fileOut != NULL){
      
      int fd =open(fileOut, O_WRONLY | O_CREAT | O_TRUNC, 0777);
      
      if(fd==-1){
          printf("Error could not open file");
          break;
        }
      
      if(dup2(fd, STDOUT_FILENO)== -1){
        fprintf(stderr,"could not open");
        close(fd);
        break;
      }

    }

    if(execvp(words[0], words)==-1){
      perror("execvp");
      exit(EXIT_FAILURE);
    };
		break;
	default:
		// In the parent process
		// Wait for child's termination
    
      if(bgflag == 0){
        spawnPid = waitpid(spawnPid, &childStatus, 0);
        if(WIFEXITED(childStatus)){
          dollarQuest = WEXITSTATUS(childStatus);
          break;
      }
    }

    //----IF BGFLAG IS 1 / IT IS A BACKGROUND

    if(bgflag ==1){
        
      dollarEx = spawnPid;
    }
		break;
	}
  //for(int i=0; i< num_words; i++){
  //free(words[i]);
  //}
 }
}


//definition of substring replacement function
char *substr_rep(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub)
{
  char *str = *haystack;
  size_t haystack_len = strlen(str);
  size_t const needle_len = strlen(needle), sub_len = strlen(sub);
  
  for(; (str = strstr(str, needle));){
    ptrdiff_t off = str - *haystack;
    if (sub_len > needle_len){
    str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1 ));
    if (!str) goto exit;
    *haystack = str;
    str = *haystack + off;
    }
    memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
    memcpy(str, sub, sub_len);
    haystack_len = haystack_len + sub_len - needle_len;
    str += sub_len;
  }
  str = *haystack;
  if(sub_len < needle_len){
    str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
    if (!str) goto exit;
    *haystack = str;
  }
exit:
  return str;
}

