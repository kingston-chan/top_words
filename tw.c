// COMP2521 21T2 Assignment 1
// tw.c ... compute top N most frequent words in file F
// Usage: ./tw [Nwords] File

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Dict.h"
#include "stemmer.h"
#include "WFreq.h"

#define MAXLINE 1000
#define MAXWORD 100

#define isWordChar(c) (isalnum(c) || (c) == '\'' || (c) == '-')

static void string_tolower(char *str);

int main(int argc, char *argv[]) {
	int   nWords;    // number of top frequency words to show
	char *fileName;  // name of file containing book text

	// process command-line args
	switch (argc) {
		case 2:
			nWords = 10;
			fileName = argv[1];
			break;
		case 3:
			nWords = atoi(argv[1]);
			if (nWords < 10) nWords = 10;
			fileName = argv[2];
			break;
		default:
			fprintf(stderr,"Usage: %s [Nwords] File\n", argv[0]);
			exit(EXIT_FAILURE);
	}
	
	FILE *sw, *fp;
	sw = fopen("stopwords", "r");
	// Cannot open stopwords file
	if (sw == NULL) {
		fprintf(stderr, "Can't open stopwords\n");
		return 0;
	}
	fp = fopen(fileName, "r");
	// Cannot open given file
	if (fp == NULL) {
		fprintf(stderr, "Can't open %s\n", fileName);
		fclose(sw);
		return 0;
	}
	// Set a buffer to read in lines from file
	char sw_line[MAXWORD+1];
	// Create a dictionary for the stopwords
	Dict d_sw = DictNew();
	while (fgets(sw_line, MAXWORD+1, sw) != NULL) {
		// Remove newline from stopword list
		sw_line[strcspn(sw_line, "\n")] = '\0';
		// Insert word into stopword dictionary
		DictInsert(d_sw, sw_line);
	}
	// Create a dictionary for the book
	Dict d_book = DictNew();
	// Set a buffer to read in lines from file
	char line[MAXLINE+1];
	// Set delimiters for tokenising words
	char delimiter[] = "!\"#$%&()*+,./:;<=>?@[\\]^_`{|}~\n\r ";
	int is_pg_book = 0;
	int end = 1;
	while (fgets(line, MAXLINE+1, fp) != NULL && end) {
		// Check if Project Gutenberg book
		if (strstr(line, "*** START OF")) {
			is_pg_book = 1;
			fgets(line, MAXLINE+1, fp);
		}
		if (strstr(line, "*** END OF")) end = 0;
		// Check if read in line has valid characters
		if (is_pg_book && end) {
			// Convert line to all lowercase
			string_tolower(line);
			// Tokenize line with delimiter
			char *token = strtok(line, delimiter);
			while (token != NULL) {
				// If not a stopword or a single character 
				// Insert into dictionary
				if (!DictFind(d_sw, token) && token[1] != '\0') {
					stem(token, 0, strlen(token)-1);
					DictInsert(d_book, token);
				}
				token = strtok(NULL, delimiter);
			}
		}
		
	}
	// Free stopwords dictionary
	DictFree(d_sw);
	// Close both stopword and given book file
	fclose(sw);
	fclose(fp);
	// Given file is not a Project Gutenberg book
	if (is_pg_book == 0 || end) {
		DictFree(d_book);
		fprintf(stderr, "Not a Project Gutenberg book\n");
		return 0;
	}
	
	// Create an array of WFreq structs
	WFreq *wfs = malloc(nWords*sizeof(*wfs));
	if (wfs == NULL) {
		DictFree(d_book);
		fprintf(stderr, "Memory allocation failed for top N array\n");
		return 0;
	}
	// Put all most frequent words into wfs and
	// get how many words was place into it
	int k = DictFindTopN(d_book, wfs, nWords);
	// Print out the word and its frequency
	for (int j = 0; j < k; j++) {
		printf("%d %s\n", wfs[j].freq, wfs[j].word);
	}
	free(wfs);
	DictFree(d_book);	
}

// Lowercase the string
static void string_tolower(char *str) {
	for (int i = 0; str[i] != '\0'; i++) {
		str[i] = tolower(str[i]);
	}
}