#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "types.h"

#define PAGE_SIZE 4096
#define PAGE_HEADER_SIZE 128

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

  if (fseek(pp->fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "fseek failed");
    return 1;
  }
  pp->page_count = ftell(pp->fp)/PAGE_SIZE;

  return 0;
}

int pager_read_page(Pager *pp, u32 page, void *buf) {
  if (page >= pp->page_count) {
    return 1;
  }

  if (fseek(pp->fp, page*PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed");
  }

  usize n = fread(buf, 1, PAGE_SIZE, pp->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "fread failed");
    return 1;
  }

  return 0;
}

void pager_create_page(Pager *pp) {
  if (fseek(pp->fp, pp->page_count * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed");
  }
  
  // write the page_id
  u32 buf[1] = { pp->page_count };
  usize n = fwrite(buf, sizeof(u32), 1, pp->fp);

  if (n != 1) {
    fprintf(stderr, "error when writing into file");
  }

  // pretend the rest of the header is being written
  u8 buff[PAGE_HEADER_SIZE - sizeof(u32)] = { 0 };
  usize n1 = fwrite(buff, sizeof(u8), PAGE_HEADER_SIZE - sizeof(u32), pp->fp); 

  if (n1 != PAGE_HEADER_SIZE - sizeof(u32)) {
    fprintf(stderr, "error when writing into file");
  }

  // fill with 0 the rest of the page
  u8 buff1[PAGE_SIZE - PAGE_HEADER_SIZE] = { 0 };
  usize n2 = fwrite(buff1, sizeof(u8), PAGE_SIZE - PAGE_HEADER_SIZE, pp->fp);

  if (n2 != PAGE_SIZE - PAGE_HEADER_SIZE) {
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

int main(void) {
  //create_file();
  //page_read();
  Pager pg1;
  pager_open(&pg1, "test.db");
  printf("Number of pages: %u\n", pg1.page_count);
  
  u8 buf[PAGE_SIZE];
  pager_create_page(&pg1);
  printf("Number of pages: %u\n", pg1.page_count);
  pager_read_page(&pg1, pg1.page_count-1, buf);

  for (u32 i = 0; i < 8; i++) {
    printf("%u", buf[i]);
  }

  pager_close(&pg1);
  return 0;
}
