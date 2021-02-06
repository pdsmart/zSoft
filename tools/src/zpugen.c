// zpugen.c
//
// Program to turn a binary file into a VHDL lookup table.
// based on original code from Adam Pierce 29-Feb-2008
//
//   Extensively Modified by: Philip Smart, January 2019-20 to work with the ZPU EVO and its byte addressing modes.
//
// This software is free to use by anyone for any purpose.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

typedef unsigned char BYTE;

int writeByteMatrix(int fd, int wordSize, int bytenum, int addr)
{
    BYTE    opcode[8];
    ssize_t s;

    // Set binary input file to beginning.
    if(lseek(fd, 0L, SEEK_SET) != 0)
    {
        perror("Failed to rewind binary file to beginning, os error.");
        return 3;
    }

    while(1)
    {
        // Read 32 bits for byte output 1-4, 64 bits we output 5-8, otherwise full 32/64bit output.
        s = read(fd, opcode, wordSize == 64 ? 8 : 4);
        if(s == -1)
        {
            perror("File read");
            return 3;
        }

        // End of file.
        if(s == 0)
            break;

        // Output to STDOUT.
        if(bytenum == 64)
        {
            printf("        %6d => x\"%02x%02x%02x%02x%02x%02x%02x%02x\",\n", addr++, opcode[0], opcode[1], opcode[2], opcode[3], opcode[4], opcode[5], opcode[6], opcode[7]);
        } else
        if(bytenum == 32)
        {
            printf("        %6d => x\"%02x%02x%02x%02x\",\n", addr++, opcode[0], opcode[1], opcode[2], opcode[3]);
        } else
        {
            printf("        %6d => x\"%02x\",\n", addr++, opcode[bytenum]);
        }
    }
}

int main(int argc, char **argv)
{
    int     fd1;
    int     fd2;
    FILE    *tmplfp;
    int     addr1 = 0;
    int     addr2 = 0;
    int     bytenum;
    int     mode = 0;
    char    line[512];
    ssize_t s;

    // Check the user has given us an input file.
    if(argc < 3)
    {
        printf("Usage: %s <byte mode - 0-3 = LSW 64bit or 32bit word, 4-7 = MSW 64bit, 32 = 32bit word, 64 = 64bit word> <binary_file> [<startaddr>]\n", argv[0]);
        printf("       or\n");
        printf("       %s BA <32 or 64 - word size> <binary_file> <tmplfile> [<startaddr>]\n\n", argv[0]);
        printf("       or\n");
        printf("       %s BC <32 or 64 - word size> <binary_file1> <start addr1> <binary_file2> <start addr2> <tmplfile>\n\n", argv[0]);
        return 1;
    }
  
    // Are we generating a Byte Addressed file?
    if(strcmp(argv[1], "BA") == 0)
    {
        mode = 1;
    } else 
    if(strcmp(argv[1], "BC") == 0)
    {
        mode = 2;
    }

    // If optional address start parameter given, set address to its value.
    //
    if((mode == 0 && argc == 4) || (mode == 1 && argc == 6))
    {
        addr1 = atoi(argv[mode == 0 ? 3 : 5]);
    } else
    if(mode == 2)
    {
        addr1 = atoi(argv[4]);
        addr2 = atoi(argv[6]);
    }

    if(mode == 0)
    {
        bytenum = atoi(argv[1]);
        if((bytenum < 0 || bytenum > 7) && bytenum != 32 && bytenum != 64)
        {
            perror("Illegal byte number");
            return 1;
        }
    } else if(mode == 1 || mode == 2)
    {
        bytenum = atoi(argv[2]);
        if(bytenum != 32 && bytenum != 64)
        {
            perror("Illegal word size");
            return 2;
        }
       
        // Open the template file.
        tmplfp = fopen(argv[mode == 1 ? 4 : 7], "r");
        if(tmplfp == NULL)
        {
            perror("Template File Open");
            return 3;
        }
    } else
    {
        perror("Unknown mode, coding error.");
        return 4;
    }

    // Open the binary file whose data we need to represent in ascii.
    fd1 = open(argv[mode == 0 ? 2 : 3], 0);
    if(fd1 == -1)
    {
        perror("Binary File Open");
        return 5;
    }

    if(mode == 2)
    {
        // Open the application binary file whose data we need to append to the first file in ascii.
        fd2 = open(argv[5], 0);
        if(fd2 == -1)
        {
            perror("Application Binary File Open");
            return 6;
        }
    }

    if(mode == 0)
    {
        writeByteMatrix(fd1, 32, bytenum, addr1);
    } else
    {
        while(fgets(line, 512, tmplfp) != NULL)
        {
            if((strstr(line, "<BYTEARRAY_0>")) != NULL)
            {
                writeByteMatrix(fd1, 32, 0, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 32, 0, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_1>")) != NULL)
            {
                writeByteMatrix(fd1, 32, 1, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 32, 1, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_2>")) != NULL)
            {
                writeByteMatrix(fd1, 32, 2, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 32, 2, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_3>")) != NULL)
            {
                writeByteMatrix(fd1, 32, 3, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 32, 3, addr2); }
            } 
            else if((strstr(line, "<BYTEARRAY_L0>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 0, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 0, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_L1>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 1, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 1, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_L2>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 2, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 2, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_L3>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 3, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 3, addr2); }
            } 
            else if((strstr(line, "<BYTEARRAY_U0>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 4, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 4, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_U1>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 5, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 5, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_U2>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 6, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 6, addr2); }
            }
            else if((strstr(line, "<BYTEARRAY_U3>")) != NULL)
            {
                writeByteMatrix(fd1, 64, 7, addr1);
                if(mode == 2) { writeByteMatrix(fd2, 64, 7, addr2); }
            } 
            else
            {
                printf("%s", line);
            }
        }
	}

    close(fd1);
    if(mode == 1) { fclose(tmplfp); }
    if(mode == 2) { close(fd2); }
    return 0;
}

