char* get_line(const char* str, int line_num);
void get_request(int old_sec, int *req_line, int *req_sec, int total_lines, int lines_per_sec, int max_sec);
char *get_sector(char *source,int sector,int lines_per_sector,int max_line);
int get_max_line(char *file);
int get_lines(char *file);