#include<stdio.h>
#include<string.h>
#include<stdlib.h>
//Some helper functions mainly for string manipulation


//Function to get a line requested from a string
char* get_line(const char* str, int line_num) {
  //Find line requested
  int cur_line = 0;
  const char* p = str;
  while (*p != '\0' && cur_line < line_num) {
    if (*p == '\n') {
      cur_line++;
    }
    p++;
  }

  // Return NULL if we reached the end of the string without finding the specified line
  if (*p == '\0') {
    return NULL;
  }

  // Find the end of the line by searching for the next newline character or the end of the string
  const char* line_start = p;
  while (*p != '\0' && *p != '\n') {
    p++;
  }

  // Copy the line into a buffer
  int line_len = p - line_start;
  char* line = malloc(line_len + 1);
  memcpy(line, line_start, line_len);
  line[line_len] = '\0';
  return line;
}

//Function that randomly creates a request with 0.7 probability for old section and 0.3 for new section
void get_request(int old_sec, int *req_line, int *req_sec, int total_lines, int lines_per_sec, int max_sec)
{
    int r;
    if (old_sec == -1)
    {
        // First request made
        r = (rand() % max_sec);
        *req_sec = r;
    }
    else
    {
        //Choose between old and new segment
        r = rand() % 10;
        if (r <= 6)
        {
            *req_sec = old_sec;
        }
        else
        {
            r = (rand() % max_sec) ;
            *req_sec = r;
        }
    }

    if ((*req_sec) == max_sec && (total_lines % lines_per_sec != 0))
    {
        //In case segment selected is last segment and not full
        r = (rand() % (total_lines % lines_per_sec));
        *req_line = r;
    }
    else
    {
        r = (rand() % lines_per_sec);
        *req_line = r;
    }
}


//Function to get sector from a File and return it
char *get_sector(char *source,int sector,int lines_per_sector,int max_line){
    //Open file for reading
    FILE *file=fopen(source,"r+");
    
    int line=0;
    int found=0;    //flag whether or not we found the requested segment
    
    //Line buffer to temporarily read each line
    char *line_buffer=malloc(sizeof(char) * (max_line+1));
    
    //Buffer to return segment
    char *buffer=malloc(sizeof(char)*((max_line)*lines_per_sector+1));
    
    //We put \0 in the first character of each string so that strcat will push the \0 to the end
    buffer[0]='\0';
     
    while(fgets(line_buffer,max_line+1,file)!=NULL){
        //read line by line
        line++;
        if(line >((sector) * lines_per_sector) && line <= ((sector+1) * lines_per_sector)){
            //If line of line_buffer is in range of requested sector then copy it to buffer
            strcat(buffer,line_buffer);
            found=1;
        }
    }
    
    free(line_buffer);
    
    //If we found at least one line of requested segment then return buffer else return NULL
    if(found){
        return buffer;
    }
    else{
        free(buffer);
        return NULL;
    }
}

//Function to return Longest line in a specified File
int get_max_line(char *file)
{
    int max = 0, characters = 0;
    char ch;

    //Open file for reading
    FILE *source = fopen(file, "r+");
    
    //Read each character
    while ((ch = fgetc(source)) != EOF)
    {
        characters++;
        
        //If we find \n character that means new line so start counting from 0 again
        if (ch == '\n' || ch == '\0')
        {
            if (max < characters)
            {
                max = characters;
            }
            characters = 0;
        }
    }

    //Close stream
    fclose(source);
    
    //Return maximum from lines read
    return max;
}

//Function to get number of lines in a File
int get_lines(char *file)
{

    //Open file for read
    FILE *source = fopen(file, "r+");
    
    int lines = 0;
    char ch;
    
    //Read whole file character by character and count \n appearances
    while ((ch = fgetc(source)) != EOF)
    {

        if (ch == '\n' || ch == '\0')
        {
            lines++;
        }
    }

    //Close stream
    fclose(source);
    
    //Return counter
    return lines;
}