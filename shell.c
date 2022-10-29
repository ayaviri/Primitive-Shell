#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <assert.h>
#include "tokens.h"
#include "vect.h"

int maxLineSize = 256; // in characters
char *previousInputBuffer;
char *currentInputBuffer;

int executeLine(vect_t *tokens);
int executePipes(vect_t *tokens);
int executeRedirection(vect_t *tokens);
int executeAtom(vect_t *tokens);
int executeSource(vect_t *tokens);
int executePrev();
int executeHelp();
int executeChangeDirectory(vect_t *tokens);

int main(int argc, char **argv) {

  printf("Welcome to mini-shell\n");
  int hasTerminated = 0;
  previousInputBuffer = malloc(maxLineSize * sizeof(char));
  currentInputBuffer = malloc(maxLineSize * sizeof(char));

  while (!hasTerminated) {
    printf("shell $ ");
    fgets(currentInputBuffer, maxLineSize, stdin);
    int stringLength = strlen(currentInputBuffer);

    if (stringLength == 0) {
      // dealing with end of file
      printf("\nBye bye.\n");
      hasTerminated = 1;
    } else {
      currentInputBuffer[stringLength - 1] = '\0';
      int prevComparison = strcmp(currentInputBuffer, "prev");

      if (prevComparison == 0) {
        // execute previous command
        printf("%s\n", previousInputBuffer);
        strcpy(currentInputBuffer, previousInputBuffer);
      } else {
        // establish new previous command
        strcpy(previousInputBuffer, currentInputBuffer);
      }

      char **currentInputTokens = get_tokens(currentInputBuffer);

      vect_t *tokensVector = vect_new();
      char **currentToken = currentInputTokens;
      while (*currentToken != NULL) {
        int currentTokenLength = strlen(*currentToken);
        char *currentTokenCopy = malloc(currentTokenLength * sizeof(char) + 1);
        currentTokenCopy = strcpy(currentTokenCopy, *currentToken);
        vect_add(tokensVector, currentTokenCopy);
        free(currentTokenCopy);
        ++currentToken;
      }

      int vectorLength = vect_size(tokensVector);
      for (int index = 0; index < vectorLength; index++) {
        free(currentInputTokens[index]);
      }
      free(currentInputTokens);

      int executionStatus = executeLine(tokensVector);
      vect_delete(tokensVector);
      if (executionStatus == 1) {
        hasTerminated = 1;
      }
    }
  }

  return 0;
}

int executeLine(vect_t *tokens) {
  vect_t *currentFragment = vect_new();
  int vectorLength = vect_size(tokens);

  for (int index = 0; index < vectorLength; index++) {
    char *currentToken = vect_get_copy(tokens, index);
    int semicolonComparison = strcmp(currentToken, ";");
    if (semicolonComparison == 0) {
      int executionStatus = executePipes(currentFragment);

      if (executionStatus == 1) {
        return 1;
      }

      vect_delete(currentFragment);
      currentFragment = vect_new();
    } else {
      vect_add(currentFragment, currentToken);
    }
    free(currentToken);
  }

  // execution of tail fragment if it remains
  int tailFragmentSize = vect_size(currentFragment);
  if (tailFragmentSize > 0) {
    int executionStatus = executePipes(currentFragment);

    if (executionStatus == 1) {
      vect_delete(currentFragment);
      return 1;
    } 
  }

  vect_delete(currentFragment);
  return 0;
}

int executePipes(vect_t* tokens) {
  vect_t *currentFragment = vect_new();
  int vectorLength = vect_size(tokens);

  for (int index = 0; index < vectorLength; index++) {
    char *currentToken = vect_get_copy(tokens, index);
    int pipeComparison = strcmp(currentToken, "|");

    if (pipeComparison == 0) {
      free(currentToken);
      int childProcessId = fork();
      int childExecutionStatus;

      if (childProcessId == 0) {
        int pipeFileDescriptors[2];
        assert(pipe(pipeFileDescriptors) == 0);
        int readEnd = pipeFileDescriptors[0];
        int writeEnd = pipeFileDescriptors[1];
        int grandchildExecutionStatus;
        int grandchildProcessId = fork();

        if (grandchildProcessId == 0) {
          close(readEnd);
          close(1);
          assert(dup(writeEnd) == 1);
          int executionStatus = executeRedirection(currentFragment);
          exit(executionStatus);
        }

        wait(&grandchildExecutionStatus);

        if (grandchildExecutionStatus == 1) {
          vect_delete(currentFragment);
          return 1;
        }

        close(writeEnd);
        close(0);
        assert(dup(readEnd) == 0);

        // execute remaining command here, though
        int remainingTokens = vectorLength - (index + 1);
        if (remainingTokens > 0) {
          vect_t *fragmentRemainder = vect_new();
          for (int remainderIndex = index + 1; remainderIndex < vectorLength; remainderIndex++) {
            char *currentToken = vect_get_copy(tokens, remainderIndex);
            vect_add(fragmentRemainder, currentToken);
            free(currentToken);
          }
          int executionStatus = executeLine(fragmentRemainder);
          // free(fragmentRemainder);
          exit(executionStatus);
        } else {
          printf("Broken pipe\n");
          exit(0);
        }
      }

      wait(&childExecutionStatus);

      if (childExecutionStatus == 1) {
        vect_delete(currentFragment);
        return 1;
      }

      vect_delete(currentFragment);
      currentFragment = vect_new();
    } else {
      vect_add(currentFragment, currentToken);
      free(currentToken);
    }
  }

  // in the case that there are no pipes
  int fragmentSize = vect_size(currentFragment);
  if (fragmentSize == vectorLength && fragmentSize > 0) {
    int executionStatus = executeRedirection(currentFragment);

    if (executionStatus == 1) {
      vect_delete(currentFragment);
      return 1;
    }
  }

  vect_delete(currentFragment);

  return 0;
}

int executeRedirection(vect_t *tokens) {
  vect_t *currentAtom = vect_new();
  int vectorLength = vect_size(tokens);

  for (int index = 0; index < vectorLength; index++) {
    char *currentToken = vect_get_copy(tokens, index);
    int inputComparison = strcmp(currentToken, "<");
    int outputComparison = strcmp(currentToken, ">");

    if (inputComparison == 0) {
      int childExecutionStatus;
      int childProcessId = fork();

      if (childProcessId == 0) {
        // assume there is a token after the redirection operator and that that token is the input file
        close(0);
        // advance to the input file and also skip it on the next iteration
        char *inputFile = vect_get_copy(tokens, index + 1); // skip the input file in the next iteration
        int fileDescriptor = open(inputFile, O_RDONLY);
        assert(fileDescriptor == 0);
        int executionStatus = executeAtom(currentAtom);
        exit(executionStatus);
      }

      wait(&childExecutionStatus);

      if (childExecutionStatus == 1) {
        vect_delete(currentAtom);
        return 1;
      }

      index += 1; // adjust index to account for the advancement to the input file upon success
      vect_delete(currentAtom);
      currentAtom = vect_new();
    } else if (outputComparison == 0) {
      int childExecutionStatus;
      int childProcessId = fork();

      if (childProcessId == 0) {
        // assume there is a token after the redirection operator and that that token is the output file
        close(1);
        char *outputFile = vect_get_copy(tokens, index + 1); // skip the output in the next iteration
        int fileDescriptor = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        assert(fileDescriptor == 1);
        int executionStatus = executeAtom(currentAtom);
        exit(executionStatus);
      }

      wait(&childExecutionStatus);

      if (childExecutionStatus == 1) {
        vect_delete(currentAtom);
        return 1;
      }

      index += 1; // adjust index to account for the advancement to the input file upon success
      vect_delete(currentAtom);
      currentAtom = vect_new();
    } else {
      vect_add(currentAtom, currentToken);
    }
    free(currentToken);
  }

  // in the case that there are no redirection operators
  int atomSize = vect_size(currentAtom);
  if (atomSize == vectorLength && atomSize > 0) {
    int executionStatus = executeAtom(currentAtom);
    
    if (executionStatus == 1) {
      vect_delete(currentAtom);
      return 1;
    }
  }

  vect_delete(currentAtom);

  return 0;
}

int executeAtom(vect_t *tokens) {
  char* firstToken = vect_get_copy(tokens, 0);
  int vectorLength = vect_size(tokens);
  assert(vectorLength != 0);

  int sourceComparison = strcmp(firstToken, "source");
  int exitComparison = strcmp(firstToken, "exit");
  int prevComparison = strcmp(firstToken, "prev");
  int helpComparison = strcmp(firstToken, "help");
  int cdComparison = strcmp(firstToken, "cd");

  if (sourceComparison == 0) {
    int executionStatus = executeSource(tokens);

    if (executionStatus == 1) {
      free(firstToken);
      return 1;
    }
  } else if (exitComparison == 0) {
    printf("Bye bye.\n");
    free(firstToken);
    return 1;
  } else if (prevComparison == 0) {
    // will not be executed in a sequence
    return 0;
  } else if (helpComparison == 0) {
    executeHelp();
  } else if (cdComparison == 0) {
    int executionStatus = executeChangeDirectory(tokens);

    if (executionStatus == 1) {
      free(firstToken);
      return 1;
    }
  } else {
    // conversion of vector to array for execution
    char **tokenArray = malloc((vectorLength + 1) * sizeof(char*));
    for (int index = 0; index < vectorLength; index++) {
      char *currentToken = vect_get_copy(tokens, index);
      tokenArray[index] = currentToken;
      // free(currentToken); // TODO: free each element of the token array
    }
    tokenArray[vectorLength] = NULL;

    int childExecutionStatus;
    int processId = fork();

    if (processId == 0) {
      int executionStatus = execvp(firstToken, tokenArray);
      exit(executionStatus);
    }

    wait(&childExecutionStatus);

    if (childExecutionStatus) {
      printf("[%s]: command not found\n", firstToken);
    }

    for (int index = 0; index < vectorLength; index++) {
      free(tokenArray[index]);
    }
    free(tokenArray);
  }

  free(firstToken);
  return 0;
}

int executeSource(vect_t *tokens) {
  int vectorLength = vect_size(tokens);

  if (vectorLength > 1) {
    // SOURCE WILL IGNORE ALL ARGUMENTS AFTER THE FIRST TEXT FILE
    char *sourceFile = vect_get_copy(tokens, 1);
    int endOfFile = 0;
    char currentInputBuffer[maxLineSize];
    FILE *fileDescriptor = fopen(sourceFile, "r");

    while (!endOfFile) {

      if (fgets(currentInputBuffer, maxLineSize, fileDescriptor) == NULL) {
        endOfFile = 1;
      } else {
        int stringLength = strlen(currentInputBuffer);
        currentInputBuffer[stringLength - 1] = '\0';
        int prevComparison = strcmp(currentInputBuffer, "prev");

        if (prevComparison == 0) {
          // execute previous command
          strcpy(currentInputBuffer, previousInputBuffer);
        } else {
          // establish new previous command
          strcpy(previousInputBuffer, currentInputBuffer);
        }

        char **currentInputTokens = get_tokens(currentInputBuffer);

        vect_t *tokensVector = vect_new();
        char **currentToken = currentInputTokens;
        while (*currentToken != NULL) {
          int currentTokenLength = strlen(*currentToken);
          char *currentTokenCopy = malloc(currentTokenLength * sizeof(char) + 1);
          strcpy(currentTokenCopy, *currentToken);
          vect_add(tokensVector, currentTokenCopy);
          free(currentTokenCopy);
          ++currentToken;
        }

        int vectorLength = vect_size(tokensVector);
        for (int index = 0; index < vectorLength; index++) {
          free(currentInputTokens[index]);
        }
        free(currentInputTokens);

        int executionStatus = executeLine(tokensVector);
        vect_delete(tokensVector);
        if (executionStatus == 1) {
          return 1;
        }
      }
    }

  } else {
    printf("source: not enough arguments\n");
  }

  return 0;
}

int executePrev() {
  printf("prev: cannot be executed in a sequence\n");
  return 0;
}

int executeHelp() {
  printf("available commands:\n");
  printf(" * cd [/file/path/] - changes the current working directory\n");
  printf(" * source [file.txt] - executes a script\n");
  printf(" * prev - prints the previous command line and executes it again. cannot be executed in a sequence\n");
  printf(" * help - explains all of the built-in commands\n");
  return 0;
}

int executeChangeDirectory(vect_t *tokens) {
  int vectorLength = vect_size(tokens);
  char *homeDirectory = malloc(strlen(getenv("HOME")) * sizeof(char) + 1);
  char *currentWorkingDirectory = malloc(maxLineSize * sizeof(char));
  char *newWorkingDirectory = malloc(maxLineSize * sizeof(char));
  getcwd(currentWorkingDirectory, maxLineSize);
  strcpy(homeDirectory, getenv("HOME"));


  int status;

  if (vectorLength > 1) {
    // CD WILL IGNORE ALL ARGUMENTS AFTER THE FIRST FILE PATH
    char *filePath = vect_get_copy(tokens, 1);

    if (strcmp(filePath, "~") == 0) { // *****
      // home
      strcpy(newWorkingDirectory, homeDirectory);
      status = chdir(newWorkingDirectory);
    } else if (filePath[0] == '/') { // *****
      // absolute path
      strcpy(newWorkingDirectory, filePath);
      status = chdir(newWorkingDirectory);
    } else if (filePath[0] == '~') {
      // path relative to home - remove ~
      memmove(filePath, filePath + 1, strlen(filePath));
      strcpy(newWorkingDirectory, homeDirectory);
      strcat(newWorkingDirectory, filePath);
      status = chdir(newWorkingDirectory);
    } else if (filePath[0] == '.' && filePath[1] == '/') {
      // relative path - remove .
      memmove(filePath, filePath + 1, strlen(filePath));
      strcpy(newWorkingDirectory, currentWorkingDirectory);
      strcat(newWorkingDirectory, filePath);
      status = chdir(newWorkingDirectory);
    } else {
      // relative path - insert slash
      strcpy(newWorkingDirectory, currentWorkingDirectory);
      strcat(newWorkingDirectory, "/");
      strcat(newWorkingDirectory, filePath);
      status = chdir(newWorkingDirectory);
    }

    if (status != 0) {
      printf("cd: no such file or directory: %s\n", filePath);
    }

    free(filePath);
  } else {
    // home
    newWorkingDirectory = malloc(strlen(homeDirectory) * sizeof(char));
    strcpy(newWorkingDirectory, homeDirectory);
    chdir(newWorkingDirectory);
    status = 0;
  }

  if (status != 0) {
    chdir(currentWorkingDirectory);
  }

  free(currentWorkingDirectory);
  free(homeDirectory);
  free(newWorkingDirectory);

  return 0;
}

