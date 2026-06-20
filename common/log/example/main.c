/*
 * main.c
 *
 *  Created on: Apr 3, 2023
 *      Author: mannk
 */


#include <stdio.h> // for FILE
//#include <syscall.h>
#include "fcntl.h"
#include <stdlib.h>
#include "../src/log.h"

#define PERM 660
void filecp(FILE* infile, FILE* outfile);
void fatalCb(log_Event* env);

// simplified cat linux
int main(int argc, char* argv[]) {
	FILE* fp;
	FILE* logfile;

	if (argc == 1){
		printf("No file to cat\n");
		return 1;
	}

	// open file save log
	logfile = fopen("log.txt", "a");

	// add callback write to file, loglevel >= LOG_TRACE
	log_add_fp(logfile, LOG_TRACE);

	// add custom callback funtion when meet LOG_FATAL
	log_add_callback(fatalCb, stderr, LOG_FATAL);

	const char* TAG = "TESTING";
	while(--argc){
		if((fp = fopen(*++argv, "r")) == NULL) {
			printf("ERR open file");
			return 1;
		}
		log_info(TAG, "Address of file in RAM: %p", fp);
		log_trace(TAG, "Opening file, printf content file: %s", *argv);
		filecp(fp, stdout);
		fclose(fp);

		log_fatal(TAG, "This is a fatal test");

		log_info(TAG, "Address of file in RAM after close file: %p");
		log_info(TAG, "line:%d, file:%s\n", __LINE__, __FILE__);
	}
	fclose(logfile);
	return 0;
}

void filecp(FILE* infile, FILE* outfile){
	char c;
	while((c=getc(infile)) != EOF){
		putc(c, outfile);
	}
}

void fatalCb(log_Event* env){
	printf("This is a fatal action\n");
}
