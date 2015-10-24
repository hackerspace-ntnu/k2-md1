#ifndef FILEIO_HEADER
#define FILEIO_HEADER

#include <cstdio>
#include <malloc.h>

namespace KineBot
{
/*!
 * \brief Read a text file into memory
 * \param file Path to an existing file which is to be read
 * \return A null-terminated string with the file's text contents
 */
static char* read_text_file(const char* file)
{
    FILE* fp = fopen(file,"r");
    fseek(fp,0,SEEK_END);
    size_t sz = ftell(fp);
    rewind(fp);

    char* text = (char*)malloc(sz+1);
    fread(text,sizeof(char),sz,fp);
    text[sz] = 0;

    fclose(fp);
    return text;
}

}

#endif
