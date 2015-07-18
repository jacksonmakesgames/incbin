#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int fline(char **line, size_t *n, FILE *fp) {
    int chr;
    char *pos;
    if (!line || !n || !fp)
        return -1;
    if (!*line)
        if (!(*line = (char *)malloc((*n=64))))
            return -1;
    chr = *n;
    pos = *line;
    for (;;) {
        int c = fgetc(fp);
        if (chr < 2) {
            *n += (*n > 16) ? *n : 64;
            chr = *n + *line - pos;
            if (!(*line = (char *)realloc(*line,*n)))
                return -1;
            pos = *n - chr + *line;
        }
        if (ferror(fp))
            return -1;
        if (c == EOF) {
            if (pos == *line)
                return -1;
            else
                break;
        }
        *pos++ = c;
        chr--;
        if (c == '\n')
            break;
    }
    *pos = '\0';
    return pos - *line;
}

int main(int argc, char **argv) {
    int ret = 0;
    char outfile[FILENAME_MAX] = "data.c";
    int i;
    FILE *out = NULL;

    argc--;
    argv++;

    if (argc == 0) {
usage:
        fprintf(stderr, "%s [-help] | <files> | [-o output]\n", argv[-1]);
        fprintf(stderr, "   -o       - output file [default is \"data.c\"]\n");
        fprintf(stderr, "   -help    - this\n");
        fprintf(stderr, "example:\n");
        fprintf(stderr, "   %s icon.png music.mp3 -o file.c\n", argv[-1]);
        return 1;
    }

    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            if (i + 1 < argc) {
                strcpy(outfile, argv[i + 1]);
                memmove(argv+i, argv+i+2, (argc-i) * sizeof(char*));
                argc -= 2; /* Eliminate "-o <thing>" */
                continue;
            }
        }
        if (!strcmp(argv[i], "-help"))
            goto usage;
    }

    if (!(out = fopen(outfile, "w"))) {
        fprintf(stderr, "failed to open `%s' for output\n", outfile);
        return 1;
    }

    fprintf(out, "/* File automatically generated by incbin */\n");
    fprintf(out, "#include \"incbin.h\"\n\n");
    fprintf(out, "#ifdef __cplusplus\n");
    fprintf(out, "extern \"C\" {\n");
    fprintf(out, "#endif\n\n");

    for (i = 0; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        char *line = NULL;
        size_t size = 0;
        if (!fp) {
            fprintf(stderr, "failed to open `%s' for reading\n", argv[i]);
            fclose(out);
            return 1;
        }
        while (fline(&line, &size, fp) != -1) {
            char *inc, *beg, *sep, *end, *name, *file;
            FILE *f;
            if (!(inc = strstr(line, "INCBIN"))) continue;
            if (!(beg = strchr(inc, '(')))       continue;
            if (!(sep = strchr(beg, ',')))       continue;
            if (!(end = strchr(sep, ')')))       continue;
            name = beg + 1;
            file = sep + 1;
            while (isspace(*name)) name++;
            while (isspace(*file)) file++;
            sep--;
            while (isspace(*sep)) sep--;
            *++sep = '\0';
            end--;
            while (isspace(*end)) end--;
            *++end = '\0';
            fprintf(out, "/* INCBIN(%s, %s); */\n", name, file);
            fprintf(out, "INCBIN_CONST INCBIN_ALIGN unsigned char g%sData[] = {\n    ", name);
            *--end = '\0';
            file++;
            if (!(f = fopen(file, "rb"))) {
                fprintf(stderr, "failed to include data `%s'\n", file);
                goto end;
            } else {
                long size, i;
                unsigned char *data, count;
                fseek(f, 0, SEEK_END);
                size = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (!(data = (unsigned char *)malloc(size))) {
                    fprintf(stderr, "out of memory\n");
                    fclose(f);
                    ret = 1;
                    goto end;
                }
                if (fread(data, size, 1, f) != 1) {
                    fprintf(stderr, "failed reading include data `%s'\n", file);
                    free(data);
                    fclose(f);
                    ret = 1;
                    goto end;
                }
                for (i = 0; i < size; i++) {
                    if (count == 12) {
                        fprintf(out, "\n    ");
                        count = 0;
                    }
                    fprintf(out, i != size - 1 ? "0x%02X, " : "0x%02X", data[i]);
                    count++;
                }
                free(data);
                fclose(f);
            }
            fprintf(out, "\n};\n");
            fprintf(out, "INCBIN_CONST INCBIN_ALIGN unsigned char *g%sEnd = g%sData + sizeof(g%sData);\n", name, name, name);
            fprintf(out, "INCBIN_CONST unsigned int g%sSize = sizeof(g%sData);\n", name, name);
        }
end:
        free(line);
        fclose(fp);
    }

    if (ret == 0) {
        fprintf(out, "\n#ifdef __cplusplus\n");
        fprintf(out, "}\n");
        fprintf(out, "#endif\n");
        fclose(out);
        return 0;
    }

    fclose(out);
    remove(outfile);
    return 1;
}
