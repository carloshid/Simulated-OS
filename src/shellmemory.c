#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include "ready_queue.h"

//#define SHELL_MEM_LENGTH 1000

void mem_free_lines_between(int start, int end);
int evict_LRU();
char* mem_get_value_at_line(int index);
void printShellMemory();

struct memory_struct{
	char *var;
	char *value;
};

struct memory_struct varmemory[VAR_STORE_SIZE];
struct memory_struct pagememory[FRAME_STORE_SIZE];
 
struct page {
	int pageNumber;
	int startIndex;
	int size;
	bool available;
	int lastUsed;
};

struct page pages[FRAME_STORE_SIZE / 3];

int getPageIndex(int pgN) {
	return pages[pgN].startIndex;
}

void printPg2() {
	int i = pages[2].pageNumber;
	int j = pages[2].startIndex;
	int k = pages[2].size;
	printf("Page 2 - number: %d, start index: %d, size: %d\n", i, j, k);
}


// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}

void createPage(int index, int number) {
	pages[number].size = 3;
	pages[number].startIndex = index;
	pages[number].pageNumber = number;
	pages[number].available = true;
	pages[number].lastUsed = 0;
}

int time_counter = 1;

int getNextTimeUnit() {
	return time_counter++;
}

// Shell memory functions

void mem_init(){
	int i;
	for (i=0; i < FRAME_STORE_SIZE; i++){		
		pagememory[i].var = "none";
		pagememory[i].value = "none";
	}
	for (i = 0; i < VAR_STORE_SIZE; i++) {
		varmemory[i].var = "none";
		varmemory[i].value = "none";
	}

	// Create pages for frame store
	for (int i = 0; i < FRAME_STORE_SIZE; i = i + 3) {
		createPage(i, i / 3);
	}
}

int getFirstAvailablePage() {
	for (int i = 0; i < FRAME_STORE_SIZE / 3; i++) {
		if (pages[i].available) {
			return i;
		}
	}
	return -1;
}

void updatePageTime(int pg) {
	pages[pg].lastUsed = getNextTimeUnit();
}

void loadProgramPage(int programPage, int pageNumberInMemory, char* filename) {
	
	int startLine = programPage * 3;
	int startMemoryIndex = pages[pageNumberInMemory].startIndex;
	pages[pageNumberInMemory].available = false;
	pages[pageNumberInMemory].lastUsed = getNextTimeUnit();

	char* var = calloc(1, 100);
	strcpy(var, filename);
	strcat(var, "@");
	char numStr[5];
	sprintf(numStr, "%d", programPage);
	strcat(var, numStr);

	FILE* file = fopen(filename, "rt");
	char *line;
	// Skip lines until startLine
	for (int i = 0; i < startLine; i++) {

		line = calloc(1, 1000);
		fgets(line, 999, file);
		free(line);
	}

	// Load the 3 lines into the corresponding page in memory
	for (int i = 0; i < 3; i++) {
		if (feof(file)) {
			break;
		} 
		else {
			line = calloc(1, 1000);
			fgets(line, 999, file);
			pagememory[i + startMemoryIndex].var = strdup(var);
			pagememory[i + startMemoryIndex].value = strndup(line, strlen(line));
			free(line);
		}
	}
	free(var);
	fclose(file);
}

// Free the page in memory
void free_page(int pgN) {
	int start = pages[pgN].startIndex;
	int end;
	if (strcmp(pagememory[start + 2].var, "none") != 0) {
		end = start + 2;
	}
	else if (strcmp(pagememory[start + 1].var, "none") != 0) {
		end = start + 1;
	}
	else {
		end = start;
	}
	mem_free_lines_between(start, end);
	pages[pgN].available = true;
}

// Evict the least recently used page
int evict_LRU() {
	int lru = 0;
	int value = pages[0].lastUsed;
	// Find the least recently used
	for (int i = 1; i < FRAME_STORE_SIZE / 3; i++) {
		if (pages[i].lastUsed < value) {
			value = pages[i].lastUsed;
			lru = i;
		}
	}

	// Evict it
	char* line;
	int start = pages[lru].startIndex;
	char* var = pagememory[start].var;
	printf("Page fault! Victim page contents:\n\n");
	line = mem_get_value_at_line(start);
	printf("%s", line);
	if (strcmp(pagememory[start + 1].var, "none") != 0) {
		line = mem_get_value_at_line(start + 1);
		printf("%s", line);
	}
	if (strcmp(pagememory[start + 2].var, "none") != 0) {
		line = mem_get_value_at_line(start + 2);
		printf("%s", line);
	}
	printf("\nEnd of victim page contents.\n");

	char* program;
	char* programPage; 

	program = strtok(var, "@");
	programPage = strtok(NULL, "@");

	int programPageInt = atoi(programPage);

	free_page(lru);
	return lru;
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(varmemory[i].var, var_in) == 0){
			varmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i< VAR_STORE_SIZE; i++){
		if (strcmp(varmemory[i].var, "none") == 0){
			varmemory[i].var = strdup(var_in);
			varmemory[i].value = strdup(value_in);
			return;
		} 
	}

	return;

}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=0; i< VAR_STORE_SIZE; i++){
		if (strcmp(varmemory[i].var, var_in) == 0){
			return strdup(varmemory[i].value);
		} 
	}
	return NULL;

}


void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < FRAME_STORE_SIZE; i++){
		if(strcmp(pagememory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, pagememory[i].var, pagememory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", FRAME_STORE_SIZE, FRAME_STORE_SIZE-count_empty, count_empty);
}



/*
 * Function:  addFileToMem 
 * 	Added in A2
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		Note that the first 100 lines are for set command, the rests are for run and exec command
 *
 *  pStart: This function will store the first line of the loaded file 
 * 			in shell memory in here
 *	pEnd: This function will store the last line of the loaded file 
 			in shell memory in here
 *  fileID: Input that need to provide when calling the function, 
 			stores the ID of the file
 * 
 * returns: error code, 21: no space left
 */
int load_file(FILE* fp, int* size, int* pg0, int* pg1, char* filename, char* fileNameInBackingStore)
{
	// Create the file in the backing store
	int counter = 0;
	char* newFilename = calloc(1, 100);
	char* nameWithFolder = calloc(1, 100);
	FILE* fileExists;
	char* command = calloc(1, 100);
	while (true) {
		strcpy(newFilename, filename);
		char numStr[5];
		sprintf(numStr, "%d", counter);
		strcat(newFilename, numStr);
		strcpy(nameWithFolder, "backingstore/");
		strcat(nameWithFolder, newFilename);

		if ((fileExists = fopen(nameWithFolder, "r"))) {
			// file exists, add 1 to counter
			fclose(fileExists);
			counter += 1;
		}
		else {
			// file does not exist, create it	
			strcpy(command, "touch ");
			strcat(command, nameWithFolder);
			system(command);
			break;
		}
	}
	free(newFilename);
	free(command);

	// Copy the contents of the file to the newly created file
	FILE* newFile = fopen(nameWithFolder, "wt");
	int c;
	while ((c = fgetc(fp)) != EOF) {
		if (c == ';') {
			fputc('\n', newFile);
		}
		else
		{
			fputc(c, newFile);
		}
	}
	fclose(newFile);
	rewind(fp);
	strcpy(fileNameInBackingStore, nameWithFolder);

	// Open the new file to read from it
	FILE* newFileRead = fopen(nameWithFolder, "rt");
	free(nameWithFolder);

	// Count the number of lines in the file
	int lines = 0;
	char* line;
	int error_code = 0;
	while (true) {
		if (feof(newFileRead)) {
			break;
		}
		lines += 1;
		line = calloc(1, 1000);
		fgets(line, 999, newFileRead);
		free(line);
	}
	fclose(newFileRead);

	*size = lines;
	*pg0 = getFirstAvailablePage();
	// At least 1 page available
	if (*pg0 != -1) {
		loadProgramPage(0, *pg0, fileNameInBackingStore);
	}
	// Program has more than 1 page
	if (lines > 3) {
		*pg1 = getFirstAvailablePage();
		// At least 1 page available
		if (*pg1 != -1) {
			loadProgramPage(1, *pg1, fileNameInBackingStore);
		}
	}

    return error_code;
}



char* mem_get_value_at_line(int index){
	if(index<0 || index > FRAME_STORE_SIZE) return NULL; 
	return pagememory[index].value;
}

void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<FRAME_STORE_SIZE; i++){
		if (strcmp(pagememory[i].var, "none") != 0) {
			if (pagememory[i].var != NULL) {
				free(pagememory[i].var);
			}
			if (pagememory[i].value != NULL) {
				free(pagememory[i].value);
			}
			pagememory[i].var = "none";
			pagememory[i].value = "none";
		}
	}
}