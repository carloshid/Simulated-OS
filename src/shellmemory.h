#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* size, int* pg0, int* pg1, char* filename, char* fileNameInBackingStore);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
int getPageIndex(int pgN);
void free_page(int pgN);
void loadProgramPage(int programPage, int pageNumberInMemory, char* filename);
int getFirstAvailablePage();
void printPg2();
int evict_LRU();
void updatePageTime(int pg);
#endif