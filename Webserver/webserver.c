#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include "helpers.h"

#define MAX_PENDING 5
#define MAX_LINE 4096

unsigned int data_length;

pthread_mutex_t lock;

bool keep_posting = false;

struct http_response{
  char status_line[100];
  char headers[MAX_LINE];
  unsigned char *body;
};

struct pipeline_args{
  char *buffer;
  int socket;
  pthread_mutex_t enter_lock;
  pthread_mutex_t exit_lock;
  bool *keep_alive;
};

typedef struct http_response http_response_t;

typedef struct pipeline_args pipeline_args_t;

const char *status_continue = "HTTP/1.1 100 Continue \r\n";
const char *status_ok = "HTTP/1.1 200 OK \r\n";
const char *status_400 = "HTTP/1.1 400 Forbidden \r\n";
const char *status_missing = "HTTP/1.1 404 Page Not Found \r\n";
const char *server_error = "HTTP/1.1 500 Server Error \r\n";
const char *enable_cors = "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\nAccess-Control-Allow-Methods: *\r\n";

void combine(http_response_t *response, char *outputmessage){
  outputmessage[0] = '\0';
  strcat(outputmessage, response->status_line);
  strcat(outputmessage, response->headers);
  strcat(outputmessage, "\r\n");
  strcat(outputmessage, response->body);
  strcat(outputmessage, "\0");
  printf("Length: %ld\n", strlen(outputmessage));
}

void *handlePath(void *aux){
  pipeline_args_t *args = (pipeline_args_t *)aux;
  int new_s = args->socket;
  char retval[MAX_LINE];
  char buf[MAX_LINE];
  char *token = NULL;
  char *bodytoken = NULL;
  char *request = NULL;
  char *request_remainder;
  char *status_line_remainder;
  bool keep_alive = false;
  FILE *fptr;
  http_response_t *response = malloc(sizeof(http_response_t));
  response->headers[0] = '\0';
  buf[0] = '\0';  
  
  /* Gets the status line from the request */
  strcpy(retval, args->buffer);
  token = strtok_r(retval, "\n", &request_remainder);
  remove_char_from_string('\r', token);
  request = token;

  /* Gets the method from the status line*/
  token = strtok_r(request, " ", &status_line_remainder);
  remove_char_from_string('\r', token);
  if(strcmp(token, "GET") == 0){
    printf("Get Request\n");
    /* Gets the path from the status line */
    token = strtok_r(NULL, " ", &status_line_remainder);

    if(strcmp(token, "/") == 0){//Get the root pagex
      printf("Get Main Page\n");
      //Set status_message;
      strcpy(response->status_line, status_ok);
    
      //Set the headers of the response
      get_current_time(response->headers);
      strcat(response->headers, "Content-Type: text/html; charset=UTF-8\r\n");
      //Set the body of the response
      response->body = malloc(sizeof(unsigned char)*MAX_LINE);
      read_file_contents("index.html", response->body);
      set_content_length(response->headers, strlen(response->body));
      //Method to read through headers
      while(token != NULL){
        remove_char_from_string('\r', token);
        if(strcmp(token, "Connection: keep-alive") == 0){
          strcat(response->headers, "Connection: keep-alive\r\n");
          keep_alive = true;
          printf("Keeping Alive: %d\n", keep_alive);
        }
        token = strtok_r(NULL, "\n", &request_remainder);
      }
      //Builds the response
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      printf("Sending main page\n");
      send(new_s, buf, strlen(buf), 0);
      printf("Keeping Alive: %d\n", keep_alive);
    } else if(strcmp(token, "/favicon.ico") == 0){
      printf("Getting a favicon\n");
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-type: image/x-icon\r\n");
      response->body = malloc(sizeof(unsigned char)*MAX_LINE);
      set_content_length(response->headers, get_file_length("favicon.ico"));
      
      while(token != NULL){
        remove_char_from_string('\r', token);
        if(strcmp(token, "Connection: keep-alive") == 0){
          strcat(response->headers, "Connection: keep-alive\r\n");
          keep_alive = true;
        }
        token = strtok_r(NULL, "\n", &request_remainder);
      }
      get_current_time(response->headers);
      response->body[0] = '\0';
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
      fptr = fopen("favicon.ico", "rb");
      unsigned long int prior = 0;
      while(!feof(fptr)){
        fgets(buf, MAX_LINE, fptr);
        send(new_s, buf, ftell(fptr) - prior, 0);
        prior = ftell(fptr);
      }
      fclose(fptr);
    } else if(strcmp(token, "/testpic.png") == 0){
      printf("Getting an image\n");

      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-type: image/png\r\n");
            remove_char_from_string('/', token);
      set_content_length(response->headers, get_file_length("testpic.png"));

      while(token != NULL){
        remove_char_from_string('\r', token);
        if(strcmp(token, "Connection: keep-alive") == 0){
          strcat(response->headers, "Connection: keep-alive\r\n");
          keep_alive = true;
        }
        token = strtok_r(NULL, "\n", &request_remainder);
      }

      get_current_time(response->headers);
      response->body = malloc(sizeof(char)*MAX_LINE);
      response->body[0] = '\0';
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
      fptr = fopen("testpic.png", "rb");
      unsigned long int prior = 0;
      while(!feof(fptr)){
        fgets(buf, MAX_LINE, fptr);
        send(new_s, buf, ftell(fptr) - prior, 0);
        prior = ftell(fptr);
      }
      fclose(fptr);

    } else {//Get a file from a path
      printf("Getting: %s\n", token);
      remove_char_from_string('/', token);
      //printf("Comparison: %d\n", strcmp(token, "favicon.ico"));
      if((fptr = fopen(token, "r")) > 0){
        fclose(fptr);
        strcpy(response->status_line, status_ok);
        if(strcmp(token, "main.css") == 0){
          strcat(response->headers, "Content-type: text/css; charset=UTF-8\r\n");
          response->body = malloc(sizeof(unsigned char)*MAX_LINE);
          read_file_contents(token, response->body);
        } else if(strcmp(token, "script.js") == 0){
          strcat(response->headers, "Content-type: text/javascript; charset=UTF-8\r\n");
          response->body = malloc(sizeof(unsigned char)*MAX_LINE);
          read_file_contents(token, response->body);
        }
        
        //Method to read through headers
        set_content_length(response->headers, strlen(response->body)); 

        while(token != NULL){
          remove_char_from_string('\r', token);
          if(strcmp(token, "Connection: keep-alive") == 0){
            strcat(response->headers, "Connection: keep-alive\r\n");
            keep_alive = true;
          }
          token = strtok_r(NULL, "\n", &request_remainder);
        }
        get_current_time(response->headers);
        combine(response, buf);
        while(pthread_mutex_lock(&args->enter_lock) < 0){
          sleep(1);
        }
        printf("Sending css page\n");
        send(new_s, buf, strlen(buf), 0);
      } else {
      }
    }
  } else if(strcmp(token, "POST") == 0){
    printf("Post Request\n");
    token = strtok_r(NULL, " ", &status_line_remainder);
    if(strcmp(token, "/item") == 0){
      printf("Post item\n");
      strcpy(response->status_line, status_ok);
      response->body = malloc(sizeof(char)*MAX_LINE);
      //Set status_message;
      response->body[0] = '\0';
      int body_read_length = 0;
      //Set the headers of the response
      get_current_time(response->headers);
        while(strcmp(token, "\r") != 0){
          remove_char_from_string('\r', token);
          if(strcmp(token, "Connection: keep-alive") == 0){
            strcat(response->headers, "Connection: keep-alive\r\n");
            keep_alive = true;
          }
          token = strtok_r(token, ": ", &status_line_remainder);
          if(strcmp(token, "Content-Length") == 0){
            body_read_length = atoi(status_line_remainder);
          }
          token = strtok_r(NULL, "\n", &request_remainder);
        }
      remove_char_from_string('\r', token);
      token = strtok_r(NULL, "\n", &request_remainder);
      token[body_read_length] = '\0';
      add_entry(token);
      get_entries(response->body);      
      strcat(response->headers, "Content-Type: application/json\r\n");
      set_content_length(response->headers, strlen(response->body));
      //Builds the response
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
    } else if(strcmp(token, "/ping") == 0){
      strcpy(response->status_line, status_ok);
      response->body = malloc(sizeof(char)*MAX_LINE);
      strcat(response->headers, "Content-type: application/json\r\n");
      strcat(response->headers, enable_cors);
      strcpy(response->body, "{\"Status\":\"OK\"}\r\n");
      set_content_length(response->headers, strlen(response->body));
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
    }else {
      printf("Path does not exist\n");
      strcmp(response->status_line, status_missing);
      get_current_time(response->headers);
      response->body = malloc(sizeof(char)*MAX_LINE);
      response->body[0] = '\0';
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
      keep_alive = false;
    }
  } else if(strcmp(token, "OPTIONS") == 0){
    strcpy(response->status_line, status_ok);
    strcat(response->headers, enable_cors);
    response->body = malloc(sizeof(char)*MAX_LINE);
    response->body[0] = '\0';
    while(token != NULL){
      remove_char_from_string('\r', token);
      if(strcmp(token, "Connection: keep-alive") == 0){
        strcat(response->headers, "Connection: keep-alive\r\n");
        keep_alive = true;
      }
      token = strtok_r(NULL, "\n", &request_remainder);
    }
    response->body = malloc(sizeof(char)*MAX_LINE);
    response->body[0] = '\0';
    combine(response, buf);
    while(pthread_mutex_lock(&args->enter_lock) < 0){
      sleep(1);
    }
    send(new_s, buf, strlen(buf), 0);
    //TODO: Run a simple second server and request a cors header to figure out what needs to be returned.
  } else if(strcmp(token, "DELETE") == 0){
    printf("Delete request\n");
    strcpy(response->status_line, status_ok);
    response->body = malloc(sizeof(unsigned char)*MAX_LINE);
      //Set status_message;
      int body_read_length = 0;
      //Set the headers of the response
      strcpy(response->headers, "\0");
      get_current_time(response->headers);
        while(strcmp(token, "\r") != 0){
          remove_char_from_string('\r', token);
          if(strcmp(token, "Connection: keep-alive") == 0){
            strcat(response->headers, "Connection: keep-alive\r\n");
            keep_alive = true;
          }
          token = strtok_r(token, ": ", &status_line_remainder);
          if(strcmp(token, "Content-Length") == 0){
            body_read_length = atoi(status_line_remainder);
          }
          token = strtok_r(NULL, "\n", &request_remainder);
        }
      remove_char_from_string('\r', token);
      token = strtok_r(NULL, "\n", &request_remainder);
      token[body_read_length] = '\0';
      remove_entry(token);
      get_entries(response->body);
      strcat(response->headers, "Content-Type: application/json\r\n");
      set_content_length(response->headers, strlen(response->body));
      
      //Builds the response
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
  } else if(strcmp(token, "PATCH") == 0){
    printf("Patch request\n");
    strcpy(response->status_line, status_ok);
    response->body = malloc(sizeof(unsigned char)*MAX_LINE);
      //Set status_message;
      int body_read_length = 0;
      //Set the headers of the response
      strcpy(response->headers, "\0");
      get_current_time(response->headers);
        while(strcmp(token, "\r") != 0){
          remove_char_from_string('\r', token);
          if(strcmp(token, "Connection: keep-alive") == 0){
            strcat(response->headers, "Connection: keep-alive\r\n");
            keep_alive = true;
          }
          token = strtok_r(token, ": ", &status_line_remainder);
          if(strcmp(token, "Content-Length")==0){
            body_read_length = atoi(status_line_remainder);
          }
          token = strtok_r(NULL, "\n", &request_remainder);
        }
      remove_char_from_string('\r', token);

      token = strtok_r(NULL, "\n", &request_remainder);
      token[body_read_length] = '\0';
      bodytoken = strtok_r(token, ",", &status_line_remainder);
      replace_entry(token, status_line_remainder);
      get_entries(response->body);
      strcat(response->headers, "Content-Type: application/json\r\n");
      set_content_length(response->headers, strlen(response->body));
      
      //Builds the response
      combine(response, buf);
      while(pthread_mutex_lock(&args->enter_lock) < 0){
        sleep(1);
      }
      send(new_s, buf, strlen(buf), 0);
  } else if(strcmp(token, "HEAD") == 0){
    token = strtok_r(NULL, " ", &status_line_remainder);
    if(strcmp(token, "/") == 0){
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-Type: text/html; charset=UTF-8\r\n");
    } else if(strcmp(token, "/favicon.ico") == 0){
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-Type: image/x-icon\r\n");
    } else if(strcmp(token, "/testpic.png") == 0){
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-Type: image/png\r\n");
    } else if(strcmp(token, "/main.css") == 0){
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-type: text/css; charset=UTF-8\r\n");
    } else if(strcmp(token, "/script.js") == 0){
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-type: text/javascript; charset=UTF-8\r\n");
    } else if(strcmp(token, "/item") == 0){
      strcpy(response->status_line, status_ok);
      strcat(response->headers, "Content-Type: application/json\r\n");
    } else {
      strcpy(response->status_line, status_missing);
    }
    strcat(response->headers, "Connection: keep-alive\r\n");
    keep_alive = true;
    response->body = malloc(1);
    response->body[0] = '\0';
    combine(response, buf);
    while(pthread_mutex_lock(&args->enter_lock) < 0){
      sleep(1);
    }
    send(new_s, buf, strlen(buf), 0);
  }
  //printf("Keeping Alive: %d\n", keep_alive);
  free(response->body);
  free(response);
  if(!keep_alive){
    *(args->keep_alive) = false;
    printf("Killed!\n");
  }
  free(args);
  pthread_mutex_unlock(&args->exit_lock);

}

void *handleConnection(void *aux){
  char buf[MAX_LINE];
  char tempbuf[MAX_LINE];
  char bufline[MAX_LINE];
  char *token = NULL;
  char *request_remainder;//Gets the remainder of a request
  int processed;
  int len;//Tracks the length read from a buffer
  int total_len = 0; //Tracks the total continuous length of a request
  int new_s = *(int *)aux;
  bool keep_alive = true;
  clock_t last_time, current_time;
  last_time = clock();
  pthread_mutex_t enter;
  pthread_mutex_t exit; 
  pthread_mutex_init(&enter, NULL);
  pthread_mutex_init(&exit, NULL);

  //TODO: keep_alive should be false if client sends HTTP 1.0
  while(keep_alive) {
    total_len = 0;
    processed = 0;
    /* Puts the contents sent to the client to the buffer */
    len = recv(new_s, buf, sizeof(buf), 0);
    buf[len] = '\0';
    
    current_time = clock();

    if(len > 0){

      total_len += len;
      //Sets the pipeline instance
      pipeline_loop:

      //printf("Len: %d, %ld\n %s\n", len, strlen(buf), buf);
      bufline[0] = '\0';
      tempbuf[0] = '\0';
      strcpy(tempbuf , buf);

      pipeline_args_t *pipeline = malloc(sizeof(pipeline_args_t));
      pipeline->buffer = malloc(sizeof(unsigned char)*MAX_LINE);      
      //printf("tempbuf(%ld): %s\n", strlen(tempbuf), tempbuf);
      //Get the header of a request
      token = strtok_r(tempbuf, "\n", &request_remainder);
      while(strlen(token) > 1){
        //printf("Token length: %ld\n", strlen(token));
        remove_char_from_string('\r', token);
        strcat(bufline, token);
        strcat(bufline, "\r\n");
        token = strtok_r(NULL, "\n", &request_remainder);
      }
      strcat(bufline, "\r\n");
      //Copies the request status and header into the pipeline buffer
      strcpy(pipeline->buffer, bufline);
      //Sets the length of the header
      processed += strlen(bufline);

      //printf("Bufline:\n%s\n", bufline);
      //Gets the size of the body, will be 0 if there is no body
      int body_length = get_content_length(bufline);
      
      //Copies the body into the pipeline buffer
      //printf("PipelineLen(%ld), request_remainder(%ld)\n", strlen(pipeline->buffer), strlen(request_remainder));
      if(body_length > 0){
        if(total_len - processed >= body_length){
          strncat(pipeline->buffer, request_remainder, body_length);
        } else {
          strcat(pipeline->buffer, request_remainder);
        }
      }
      //printf("PipelineLen(%ld), request_remainder(%ld)\n", strlen(pipeline->buffer), strlen(request_remainder));
      processed += body_length;
      pipeline->socket = new_s;
      pipeline->enter_lock = enter;
      pipeline->exit_lock = exit;
      pipeline->keep_alive = &keep_alive; // Reference to connection's keep alive variable

      //Pre-lock the thread exit lock, the lock will unlock when the thread finishes.
      pthread_mutex_lock(&pipeline->exit_lock);

      //Changes the locks
      enter = exit;
      pthread_mutex_t temp;
      pthread_mutex_init(&temp, NULL);
      exit = temp;

      printf("Processed: %d, Total length: %d\n", processed, total_len);

      if(processed < total_len){
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handlePath, pipeline);

        //Puts the remainder of the buffer in extrabuffer
        char extrabuffer[MAX_LINE];
        strcpy(extrabuffer, &buf[strlen(pipeline->buffer)]);
        //Reads another line from the buffer
        //Prevents reading from buffer if there is nothing more to read
        if(len == MAX_LINE){
          len = recv(new_s, buf, MAX_LINE - strlen(extrabuffer), 0);
          buf[len] = '\0';
          total_len += len;
          strcat(extrabuffer, buf);
        }
        
        //Concatenates the contents of extrabuffer and buf and stores it in buf
        strcpy(buf, extrabuffer);
        //Go back to the start of the pipeline
        goto pipeline_loop;
      } else {//Will choose not to pipeline if it many use too much memory
        handlePath(pipeline);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  if(argc != 2){
    printf("\nInvalid number of arguments\nUsage: 'webserver.exe PORT_NUMBER'\n\n");
    exit(1);
  }
  int port = atoi(argv[1]);
  struct sockaddr_in sin;
  int len;
  int s, new_s;
  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  
  if(pthread_mutex_init(&lock, NULL) < 0){
    perror("mutex lock: failed");
    exit(1);
  }

  /* setup passive open */
  if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("simplex-talk: socket");
    exit(1);
  }  
  if((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("simplex-talk: bind");
    exit(1);
  } else {
    printf("Socket listening on port %d\n", port);
  }


  listen(s, MAX_PENDING);


  /* wait for connection, then receive and print text */
  while(1) {
    const int one = 1;
    len = sizeof(sin);
    if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
      perror("simplex-talk: accept");
      exit(1);
    }
    if(setsockopt(new_s, SOL_SOCKET, SO_REUSEADDR, &one, (socklen_t)sizeof(one))< 0){
      perror("simplex-talk: reuseaddr");
      exit(1);
    }
    if(setsockopt(new_s, SOL_SOCKET, SO_KEEPALIVE, &one, (socklen_t)sizeof(one)) < 0){
      perror("simplex-talk: keepalive");
      exit(1);
    }
    pthread_t thread_id;    
    pthread_create(&thread_id, NULL, handleConnection, &new_s);
    
  }
}