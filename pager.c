#include <errno.h>
#include <stdio.h>

#include "types.h"

#include "page.h"
#include "pager.h"

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

  long pos = ftell(pp->fp);
  if (pos == -1L) {
    fprintf(stderr, "ftell failed");
    return 1;
  }

  pp->page_count = pos / PAGE_SIZE;

  if (pos != pp->page_count * PAGE_SIZE) {
    fprintf(stderr, "File may be corrupted");
    return 1;
  }

  return 0;
}

int pager_read_page(Pager *pp, u32 page, void *buf) {
  if (page >= pp->page_count) {
    return 1;
  }

  if (fseek(pp->fp, page * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed");
    return 1;
  }

  usize n = fread(buf, 1, PAGE_SIZE, pp->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "fread failed");
    return 1;
  }

  return 0;
}

int pager_write_page(Pager *pp, u32 page, const void *buf) {
  if (page > pp->page_count) {
    return 1;
  }

  if (fseek(pp->fp, page * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek failed");
    return 1;
  }

  usize n = fwrite(buf, sizeof(u8), PAGE_SIZE, pp->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "error when writing into file");
    return 1;
  }

  if (fflush(pp->fp) == EOF) {
    fprintf(stderr, "error when flushing");
    return 1;
  }

  if (page == pp->page_count) {
    pp->page_count++;
  }

  return 0;
}

int pager_create_page(Pager *pp, u32 *page_id) {
  *page_id = pp->page_count;
  pp->page_count++;

  return 0;
}

void pager_close(Pager *pp) {
  if (!pp->fp) {
    return;
  }

  fclose(pp->fp);
}
