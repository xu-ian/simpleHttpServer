#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_LINE 4096

int remove_char_from_string(char c, char *str)
{
    int i=0;
    int len = strlen(str)+1;
    int count = 0;
    for(i=0; i<len; i++)
    {
        if(str[i] == c)
        {
            count++;
            // Move all the char following the char "c" by one to the left.
            strncpy(&str[i],&str[i+1],len-i);
        }
    }
    return count;
}

void get_current_time(char *headers){
  time_t current_time;
  struct tm* ptime;
  char *date = malloc(sizeof(char)*100);
  time(&current_time);
  ptime = gmtime(&current_time);
  strcpy(date, "Date: ");
  strcat(date, asctime(ptime));
  strcat(headers, date);
  free(date);
}

void set_content_length(char *headers, long int length){
  char *content_length = malloc(sizeof(char)*100);
  sprintf(content_length, "Content-Length: %ld\r\n", length);
  strcat(headers, content_length);
  free(content_length);
}

int get_content_length(char *headers){
  int body_read_length = 0;
  char *status_line_remainder;
  char *header_remainder;
  char *token = NULL;

  //Removes the status line
  token = strtok_r(headers, "\n", &header_remainder);
  
  token = strtok_r(NULL, "\n", &header_remainder);
  while(strcmp(token, "\r") != 0){
    remove_char_from_string('\r', token);
    token = strtok_r(token, ": ", &status_line_remainder);
    if(strcmp(token, "Content-Length") == 0){
      body_read_length = atoi(status_line_remainder);
    }
    token = strtok_r(NULL, "\n", &header_remainder);
  }
  return body_read_length;
}

void read_file_contents(char *name, char *totalFileContents){
  char *fileContents = malloc(sizeof(char)*MAX_LINE);
  FILE *fptr;

  //Open the requested file
  fptr = fopen(name, "rb");
  strcpy(totalFileContents, "\0");

  // Read the contents of the file and store it in totalFileContents  
  while(!feof(fptr)){
    fgets(fileContents, MAX_LINE, fptr);
    remove_char_from_string('\r', fileContents);
    strcat(totalFileContents, fileContents);
  }
  //printf("File length by file: %ld, by length of string: %ld\n", ftell(fptr), strlen(totalFileContents));
  remove_char_from_string('\r', totalFileContents);
  strcat(totalFileContents, "\n");
  free(fileContents);
  fclose(fptr);
}

void read_file_data(char *name, char *totalFileContents){
  FILE *fptr;
  fptr = fopen(name, "rb");
  while(!feof(fptr)){
    fread(totalFileContents, MAX_LINE, 1, fptr);
    printf("%s\n", totalFileContents);
  }
  //printf("File length by file: %ld, by length of string: %ld\n", ftell(fptr), strlen(totalFileContents));
  strcat(totalFileContents, "\n");
  fclose(fptr);
}

int get_file_length(char *name){

  FILE *fptr;
  fptr = fopen(name, "r");
  if(fptr == NULL){
    return 0;
  }
  //Gets the length of the file
  fseek(fptr, 0L, SEEK_END);
  int file_length = ftell(fptr);
  fseek(fptr, 0L, SEEK_SET);
  fclose(fptr);
  return file_length;
}

int add_entry(char *entry){
  FILE *fptr;
  fptr = fopen("db.txt", "a");
  if(get_file_length("db.txt") > 0){
    fwrite("\n", 1, 1, fptr);
  }
  fwrite(entry, 1, strlen(entry), fptr);
  fclose(fptr);
  return 0;
}

int remove_entry(char *entry){
  FILE *fptr;
  fptr = fopen("db.txt", "r+");
  char *totalFileContents = malloc(sizeof(char)*MAX_LINE);
  char *fileContents = malloc(sizeof(char)*MAX_LINE);
  totalFileContents[0]='\0';

  while(!feof(fptr)){
    fgets(fileContents, MAX_LINE, fptr);
    remove_char_from_string('\n', fileContents);
    if(strcmp(fileContents, entry) != 0)
    {
      strcat(totalFileContents, fileContents);
      strcat(totalFileContents, "\n");
    }
  }
  fclose(fptr);
  fptr = fopen("db.txt", "w");
  totalFileContents[strlen(totalFileContents)-1] = '\0';
  fwrite(totalFileContents, 1, strlen(totalFileContents), fptr);
  fclose(fptr);
  free(totalFileContents);
  free(fileContents);
  return 0;
}

int replace_entry(char *entry, char *replacement){
  FILE *fptr;
  fptr = fopen("db.txt", "r+");
  char *totalFileContents = malloc(sizeof(char)*MAX_LINE);
  char *fileContents = malloc(sizeof(char)*MAX_LINE);
  totalFileContents[0]='\0';

  bool first_line = true;
  while(!feof(fptr)){
    fgets(fileContents, MAX_LINE, fptr);
    if(!first_line){
      strcat(totalFileContents, "\n");
    } else {
      first_line = false;
    }
    remove_char_from_string('\n', fileContents);
    if(strcmp(fileContents, entry) != 0)
    {
      strcat(totalFileContents, fileContents);
    } else {
      strcat(totalFileContents, replacement);
    }
  }
  fclose(fptr);
  fptr = fopen("db.txt", "w");
  fwrite(totalFileContents, 1, strlen(totalFileContents), fptr);
  fclose(fptr);
  free(totalFileContents);
  free(fileContents);
  return 0;
}

void get_entries(char *totalFileContents){
  FILE *fptr;
  fptr = fopen("db.txt", "r");
  char *fileContents = malloc(sizeof(char)*MAX_LINE);
    strcpy(totalFileContents, "{\"entries\":[");
    fgets(fileContents, MAX_LINE, fptr);
    remove_char_from_string('\n', fileContents);
    strcat(totalFileContents, "\"");
    strcat(totalFileContents, fileContents);
    while(!feof(fptr)){
    //printf("%s\n", totalFileContents);
    strcat(totalFileContents, "\",\"");
    fgets(fileContents, MAX_LINE, fptr);
    remove_char_from_string('\n', fileContents);
    strcat(totalFileContents, fileContents);
  }
  strcat(totalFileContents, "\"]}");
  printf("%s\n", totalFileContents);
  fclose(fptr);
  free(fileContents);
}