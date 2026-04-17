#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "types.h"

#define PAGE_SIZE 4096

typedef struct {
  FILE *fp;
  u32 page_count;
} Pager;

int pager_open(Pager *pp, const char *filename) {
  pp->fp = fopen(filename, "r+b");
  if (!pp->fp) {
    if (errno != ENOENT) {
      fprintf(stderr, "error when opening file: %d\n", errno);
      return 1;
    }
    
    pp->fp = fopen(filename, "w+b");
    if (!pp->fp) {
      fprintf(stderr, "error when opening file: %d\n", errno);
      return 1;
    }
  }

  fseek(pp->fp, 0, SEEK_END);
  pp->page_count = ftell(pp->fp)/PAGE_SIZE;

  return 0;
}

int pager_read_page(Pager *pp, u32 page, void *buf) {
  if (page >= pp->page_count) {
    return 1;
  }

  fseek(pp->fp, page*PAGE_SIZE, SEEK_SET);
  
  u8 c;
  for (u32 i = 0; i < 8; i++) {
    fread(&c, sizeof(u8), 1, pp->fp);
    printf("%u", c);
  }

  return 0;
}

void pager_create_page(Pager *pp) {
  u8 buf[PAGE_SIZE] = { pp->page_count };
  u32 n = fwrite(buf, 1, PAGE_SIZE, pp->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "error when writing into file");
  }
  
  pp->page_count++;
  return;
}

void pager_close(Pager *pp) {
  if (!pp->fp) {
    return;
  }

  fclose(pp->fp);
}

void create_file() {
  FILE *fp;

  fp = fopen("test.db", "w+");

  if (fp == NULL) {
    fprintf(stderr, "failed to open file \n");
    return;
  }

  fclose(fp);
}

void page_read() {
  FILE *fp = fopen("test.db", "rb");

  if (fp == NULL) {
    fprintf(stderr, "failed to open file \n");
    return;
  }

  if (fseek(fp, PAGE_SIZE * 1, SEEK_SET) != 0) {
    fprintf(stderr, "error");
    return;
  }
  
  u8 c;
  for (u32 i = 0; i < 15; i++) {
    fread(&c, sizeof(u8), 1, fp);
    printf("%u\n", c);
  }

  fclose(fp);
}

int main(void) {
  //create_file();
  //page_read();
  Pager pg1;
  pager_open(&pg1, "test.db");
  printf("Number of pages: %u\n", pg1.page_count);
  
  u32 buf[PAGE_SIZE];
  pager_read_page(&pg1, 100, &buf);
  pager_create_page(&pg1);
  printf("Number of pages: %u\n", pg1.page_count);
  pager_read_page(&pg1, pg1.page_count, &buf);
  pager_close(&pg1);
  return 0;
}
