#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

int bflag, eflag, nflag, sflag, tflag, vflag;
int rval;
const char *filename;

static void scanfiles(char *argv[], int count);
static void format_line(FILE *);
static void printfile(int);

int main(int argc, char *argv[]) {
  while (1) {
    int ch;
    int index = 0;
    static struct option options[] = {
        {"number-nonblank", 0, NULL, 'b'},
        {"number", 0, NULL, 'n'},
        {"squeeze-blank", 0, NULL, 's'},
        {NULL, 0, NULL, 0},
    };
    ch = getopt_long(argc, argv, "+bEenstv", options, &index);
    if (ch == -1) break;
    switch (ch) {
      case 'b':
        bflag = nflag = 1;
        break;
      case 'e':
        eflag = vflag = 1;
        break;
      case 'E':
        eflag = 1;
        break;
      case 'n':
        nflag = 1;
        break;
      case 's':
        sflag = 1;
        break;
      case 't':
        tflag = vflag = 1;
        break;
      case 'v':
        vflag = 1;
        break;
      default:
        fprintf(stderr, "usage: cat [-bEenstv] [file ...]\n");
        break;
    }
  }
  argv += optind;

  if (bflag || eflag || nflag || sflag || tflag || vflag)
    scanfiles(argv, 1);

  else
    scanfiles(argv, 0);

  if (fclose(stdout)) err(1, "stdout");

  return rval;
}

static void scanfiles(char *argv[], int count) {
  int i = 0;
  char *path;
  FILE *fp;

  while ((path = argv[i]) != NULL || i == 0) {
    int fd;

    if (path == NULL || strcmp(path, "-") == 0) {
      filename = "stdin";
      fd = STDIN_FILENO;
    }

    else {
      filename = path;
      fd = open(path, O_RDONLY);
    }

    if (fd < 0) {
      warn("%s", path);
      rval = 1;
    }

    else if (count) {
      if (fd == STDIN_FILENO)
        format_line(stdin);

      else {
        fp = fdopen(fd, "r");
        format_line(fp);
        fclose(fp);
      }
    }

    else
      printfile(fd);

    if (path == NULL) {
      printf("NO");
      break;
    }
    ++i;
  }
}

static void format_line(FILE *fp) {
  int ch, get_flag, line, prev;

  if (fp == stdin && feof(stdin)) clearerr(stdin);

  line = get_flag = 0;

  for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
    if (prev == '\n') {
      if (sflag) {
        if (ch == '\n') {
          if (get_flag) continue;
          get_flag = 1;
        }

        else
          get_flag = 0;
      }
      if (nflag && (!bflag || ch != '\n')) {
        (void)fprintf(stdout, "%6d\t", ++line);

        if (ferror(stdout)) break;
      }
    }

    if (ch == '\n') {
      if (eflag && putchar('$') == EOF) break;
    }

    else if (ch == '\t') {
      if (tflag) {
        if (putchar('^') == EOF || putchar('I') == EOF) break;
        continue;
      }
    }

    else if (vflag) {
      if (!isascii(ch) && !isprint(ch)) {
        if (putchar('M') == EOF || putchar('-') == EOF) break;
        ch = toascii(ch);
      }

      if (iscntrl(ch)) {
        if (putchar('^') == EOF ||
            putchar(ch == '\177' ? '?' : ch | 0100) == EOF)

          break;

        continue;
      }
    }

    if (putchar(ch) == EOF) break;
  }

  if (ferror(fp)) {
    warn("%s", filename);
    rval = 1;
    clearerr(fp);
  }

  if (ferror(stdout)) err(1, "stdout");
}

static void printfile(int rfd) {
  int off, wfd;
  ssize_t nr, nw;
  static size_t bsize;
  static char *buf = NULL;
  struct stat sbuf;

  wfd = fileno(stdout);

  if (buf == NULL) {
    if (fstat(wfd, &sbuf)) err(1, "%s", filename);

    bsize = MAX(sbuf.st_blksize, 1024);

    if ((buf = malloc(bsize)) == NULL) err(1, "buffer");
  }

  while ((nr = read(rfd, buf, bsize)) > 0)

    for (off = 0; nr; nr -= nw, off += nw)

      if ((nw = write(wfd, buf + off, (size_t)nr)) < 0) err(1, "stdout");

  if (nr < 0) {
    warn("%s", filename);
    rval = 1;
  }
}
