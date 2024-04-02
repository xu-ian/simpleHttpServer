#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include "list.h"

struct request {
  char *method;
  char *path;
};

void remove_char_from_string(char, char *);

void get_current_time(char *);

void set_content_length(char *, int);

int get_content_length(char *);

void read_file_contents(char *, char *);

void read_file_data(char *, char *);

int get_file_length(char *);

int add_entry(char *);

int remove_entry(char *);

int replace_entry(char *, char *);

void get_entries(char *);
