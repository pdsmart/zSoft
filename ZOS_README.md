<br>

### zOS

zOS is a spin-off from the ZPUTA test application which was created to verify ZPU Evo and SoC operations. ZPUTA evolved as different FPGA's were included in the ZPU Evo scope and it became
clear that it had to be more advanced due to limited resources. Once ZPUTA had evolved into something of a level of sophistication akin to an OS, it was split and zOS was born.

Originally the ZPUTA/zOS developments only targetted the ZPU architecture and they had two primary methods of execution:

* as an application booted by IOCP,
* standalone booted as the ZPU Evo startup firmware.

The mode is chosen in the configuration and functionality is identical. When booted as an application by IOCP, zOS is stored on an SD card and loaded at boot time. When 'standalone'
zOS is stored in the onboard Flash (or preconfigure BRAM acting as ROM).

More recently, zOS was ported to the ARM Cortex-M4 architecture for use with the Freescale K64F on the tranZPUterSW project. As the K64F is a fixed architecture CPU / SoC only one
method of execution exists, 'standalone', it is stored in the onboard Flash and started when the CPU powers up.

In zOS, all non-OS functionality is stored as applications on an SD Card. If an SD Card isnt present then it is better to use ZPUTA as it has the ability to cater for limited FPGA BRAM
resources as all functionality of ZPUTA can be enabled/disabled within the primary loaded image.

To operate, zOS requires a serial connection (physical or usb-virtual) with preferably an ANSI/VT100 terminal emulator package connected to it. The requirement for an ANSI/VT100 terminal
emulation package is to allow commands such as the editor or readline to function correctly. A dumb serial terminal will work fine but at reduced application functionality. If no serial 
connection is available or an interactive OS is not needed, just the ability to boot an application on power up, then the IOCP would be the better choice (albeit not yet ported to the K64F).

## Using the OS

Throughout my career I have used or designed systems where you had live interaction with the end product to either monitor, change the logic flow according to market changes or fix problems
real-time. This has normally been through an embedded command processor using the TCP client-server interconnect. The benefits of this approach are enormous and done correctly mitigate
security risks. Companies such as Reuters and TibCO use this technique (ie. their 'Hawk' product) and I have crafted it into my own developments such as the trade execution and pricing module
I wrote at Morgan Stanley and the Options trading system at Chase Manhattan with great effect. 

When working on some of my projects in this repository it has always been at the forefront of my thoughts that I wanted a similar environment, even on an embedded system with limited
resources, where I could monitor, change or fix on the fly. This thought started off with the IOCP which has basic monitoring, uploading and execution functionality and was further
enhanced when I started writing ZPUTA with its enhanced functionality and external applications. Time scheduling, at least a basic level will soon be implemented which will allow an embedded
application driving the hardware to run as required yet still leave processing resource to run the OS and provide services to a connected user/developer. The projects dont have a network
stack as most dev boards I've used dont have ethernet which limites connectivity to the serial line. The advantage of asynchronous serial communications is it is simple and can be connected
to a terminal on nearly every PC, either direct via a USB adapter or via RS-232. I could implement SLIP but keeping it simple at this level is my preferred alternative. So long as you can
connect a serial line between the dev board hosting zOS and a PC you can use it's functionality.

The OS interface has been written keeping in mind that only a serial text based terminal is available. Some components such as the Editor need more advanced capabilities and to this end
the venerable VT100 emulation was chosen as a suitable candidate (which means you need a VT100 emulator sat in front of the serial line in order to use the Editor). Interaction is keyboard
and text based, (no nice GUI interfaces!) with the sections below providing information needed to use the interface.


### Command Line

The first introduction to the OS is a sign-on message which details the CPU, version etc and issues a prompt for command input, ie:

![zOS Startup Screen](../images/zOS_Startup.png)

Interaction with the OS is no different than say MSDOS, you enter a command and get a response. The only difference is that it is within an embedded system not a full blown PC.

The zOS command line is not a shell interpreter just a basic text interface provided for the issuing of built-in commands or commands stored on the SD card. It does include a cutdown readline
capability with history to aid in command entering and retrieval and generally follows the GNU readline functionality, the recognised keys at the command line are:

| Key         | Action                                                       |
| ---         | -------                                                      |
| CTRL-A      | Go to start of line.                                         |
| CTRL-B      | Move cursor one position to the left.                        |
| CTRL-C      | Abort current line and return CTRL-C to calling application. |
| CTRL-D      | Not defined in shell, passed to running application.         |
| CTRL-E      | Go to end of line.                                           |
| CTRL-F      | Move cursor one position to the right.                       |
| CTRL-K      | Clear the line.                                              |
| CTRL-N      | Recall next historized command.                              |
| CTRL-P      | Recall previous historized command.                          |
| HOME        | Go to start of line.                                         |
| END         | Go to end of line.                                           |
| INSERT      | Not yet defined.                                             |
| DEL         | Delete character under cursor.                               |
| BACKSPACE   | Delete character to left of cursor.                          |
| PGUP        | Not defined in shell, passed to running application.         |
| PGDOWN      | Not defined in shell, passed to running application.         |
| ARROW UP    | Recall previous historized command.                          |
| ARROW DOWN  | Recall next historized command.                              |
| ARROW RIGHT | Move cursor one position to the right.                       |
| ARROW LEFT  | Move cursor one position to the left.                        |

The readline mechanism also recognises a few internal commands, namely:

| Command     | Action                                                       |
| -------     | ------                                                       |
| !\<number\> | Recall and execute given historised command identified by \<number\>. |
| hist\[ory\] | List the history buffer.                                     |


### Applications

The applications currently provided by zOS reside on an SD card and are summarised below. For more detailed information
please refer to the Application section (on the left navigation bar) or the Application README file.

  - #### Disk IO Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | -------------------------------------           | ----------------------------------------------- |
    | ddump    | \[<pd#> \<sect>]                                | Dump a sector                                   |
    | dinit*   | \<pd#> \[\<card type>]                          | Initialize disk                                 |
    | dstat*   | \<pd#>                                          | Show disk status                                |
    | dioctl*  | \<pd#>                                          | ioctl(CTRL_SYNC)                                | 

  - #### Disk Buffer Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | -------------------------------------           | ----------------------------------------------- |
    | bdump    | \<ofs>                                          | Dump buffer                                     |
    | bedit    | \<ofs> \[\<data>] ...                           | Edit buffer                                     |
    | bfill    | \<val>                                          | Fill buffer                                     |
    | blen     | \<len>                                          | Set read/write length for fread/fwrite command  |
    | bread    | \<pd#> \<sect> \[\<num>]                        | Read into buffer                                |
    | bwrite   | \<pd#> \<sect> \[\<num>]                        | Write buffer to disk                            |

  - #### Filesystem Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | -------------------------------------           | ----------------------------------------------- |
    | falloc   | \<fsz> \<opt>                                   | Allocate ctg blks to file                       |
    | fattr    | \<atrr> \<mask> \<name>                         | Change object attribute                         |
    | fcat     | \<name>                                         | Output file contents                            |
    | fcd      | \<path>                                         | Change current directory                        |
    | fclose   |                                                 | Close the open file                             |
    | fconcat  | \<src fn1> \<src fn2> \<dst fn>                 | Concatenate 2 files                             |
    | fcp      | \<src file> \<dst file>                         | Copy a file                                     |
    | fdel     | \<obj name>                                     | Delete an object                                |
    | fdir     | \[\<path>]                                      | Show a directory                                |
    | fdrive   | \<path>                                         | Change current drive                            |
    | fdump    | \<name> \[\<width>]                             | Dump a file contents as hex                     |
    | fexec*   | \<name> \<ldAddr> \<xAddr> \<mode>              | Load and execute file                           |
    | finit*   | \<ld#> \[\<mount>]                              | Force init the volume                           |
    | finspect | \<len>                                          | Read part of file and examine                   |
    | flabel   | \<label>                                        | Set volume label                                |
    | fload*   | \<name> \[\<addr>]                              | Load a file into memory                         |
    | fmkdir   | \<dir name>                                     | Create a directory                              |
    | fmkfs    | \<ld#> \<type> \<au>                            | Create FAT volume                               |
    | fopen    | \<mode> \<file>                                 | Open a file                                     |
    | fread    | \<len>                                          | Read part of file into buffer                   |
    | frename  | \<org name> \<new name>                         | Rename an object                                |
    | fsave    | \<name> \<addr> \<len>                          | Save memory range to a file                     |
    | fseek    | \<ofs>                                          | Move fp in normal seek                          |
    | fshowdir |                                                 | Show current directory                          |
    | fstat    | \[\<path>]                                      | Show volume status                              |
    | ftime    | <y> <m> <d> <h> <M> <s> <fn>                    | Change object timestamp                         |
    | ftrunc   |                                                 | Truncate the file at current fp                 |
    | fwrite   | \<len> \<val>                                   | Write part of buffer into file                  |
    | fxtract  | \<src> \<dst> \<start pos> \<len>               | Extract a portion of file                       |

  - #### Memory Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | -------------------------------------           | ----------------------------------------------- |
    | mclear   | \<start\>&nbsp;\<end\>&nbsp;\[\<word\>\]&nbsp;  | Clear memory                                    |
    | mcopy    | \<start\> \<end\> \<dst addr\>                  | Copy memory                                     |
    | mdiff    | \<start\> \<end\> \<cmp addr\>                  | Compare memory                                  |
    | mdump    | \[\<start\> \[\<end\>\] \[\<size\>\]\]          | Dump memory                                     |
    | mperf    | \<start\> \<end\> \[\<width\>\] \[\<size\>\]    | Test memory performance.<br>This gives a value for the CPU \<-\> Memory, not actual memory performance. As the ZPU is stack based, it uses memory to perform a memory action, so it will never realise full memory bandwidth, hence the need for extended instructions such as LDIR.|
    | msrch    | \<start\> \<end\> \<value\>                     | Search memory.                                  |
    | mtest    | \[\<start\> \[\<end\>] \[iter\] \[test mask\]   | Test memory                                     |
    | meb      | \<addr\> \<byte\> \[...\]                       | Edit memory (Bytes)                             |
    | meh      | \<addr\> \<h-word\> \[...\]                     | Edit memory (H-Word)                            |
    | mew      | \<addr\> \<word\> \[...\]                       | Edit memory (Word)                              |

  - #### Hardware Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | -------------------------------------           | ----------------------------------------------- |
    | hr       |                                                 | Display Register Information                    |
    | ht       |                                                 | Test uS Timer                                   |

  - #### Performance Testing Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | ----------                                      | ----------------------------------------------- |
    | dhry     |                                                 | Dhrystone Test v2.1                             |
    | coremark |                                                 | CoreMark Test v1.0                              |

  - #### Program Execution Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | ----------                                      | ----------------------------------------------- |
    | call"    | \<addr>                                         | Call function @ \<addr>                         |
    | jmp"     | \<addr>                                         | Execute code @ \<addr>                          |

  - #### Miscellaneous Commands

    | Command  | Parameters                                      | Description                                     |
    | -------  | ----------                                      | ----------------------------------------------- |
    | restart  |                                                 | Restart application                             |
    | reset    |                                                 | Reset system                                    |
    | help     | \[\<cmd %>\|\<group %>]                         | Show this screen                                |
    | info     |                                                 | Config info                                     |
    | time     | \[\<y> \<m> \<d> \<h> \<M> \<s>]                | Set/Show current time                           |

  - #### Applications

    | Command  | Parameters                                      | Description                                     |
    | -------  | ----------                                      | ----------------------------------------------- |
    | ed       | \<file\>                                        | Basic WYSIWYG VT100 File Editor                 |
    | kilo     | \<file\>                                        | WYSIWYG VT100 File Editor                       |
    | mbasic   | \[\<file\>\]                                    | Interactive Mini-Basic Interpreter.             |
    | tbasic   |                                                 | Interactive tiny Basic Interpreter.             |

All commands with a (*) are built in to zOS.

*autoexec.bat*
--------------

As per MSDOS, if a file of name 'AUTOEXEC.BAT' is created in the root directory (top level) of the SD filesystem the contents will be executed on zOS at boot/reset. Use this file to automate
setup and startup of your intended application. It currently has no in-built language, just issuing of commands.


## Technical Detail

This section aims to provide some of the inner detail of the operating system to date. A lot of this information is the same in ZPUTA but as zOS matures it will start to have significant differences.



### Memory Organisation


##### IOCP Memory Map

zOS is generally used in it's 'standalone' guise, ie the OS boots up as the primary firmware. For backward compatibility or to aid in more rapid development of zOS on the ZPU, IOCP
can be used as the bootloaded. Depending upon the memory model of IOCP used, the currently defined memory maps for IOCP are as follows:-

![IOCP Memory Map](../images/IOCPMemoryMap.png)

zOS is loaded from the SD card by IOCP and resides typically 0x1000 bytes further in memory as opposed to when zOS is the boot firmware. At time of writing, IOCP can only be used
with the ZPU as it has not been ported to the K64F architecture.


##### zOS Memory Map

zOS running on the ZPU has the memory usage map as per the diagram below. zOS resides in lower BRAM as the boot firmware with Stack/Heap at the top of useable BRAM (or RAM/SDRAM). An
application is loaded at the defined load point in the build script which would typically be the start of RAM/SDRAM or BRAM if the FPGA is sufficiently large.

![zOS Memory Map for ZPU](../images/zOS_Memory_Map_for_ZPU.png)

The memory usage map for the K64F processor is similar but Flash RAM and RAM are well defined not variable as per the ZPU.

![zOS Memory Map for K64F](../images/zOS_Memory_Map_for_K64F.png)

<br>


### Application Interface

As per most operating systems, zOS provides an API which a suitably compiled application can use to decrease it's size and complexity and reuse features within the OS such as the serial line
connectivity. 

Rather than designing a custom API set of methods, it made more sense to expose methods within zOS to the application, ie. xprintf. An application calling xprintf to display output on the
terminal will actually be calling xprintf within zOS. This is made possible by creating a jump table at a fixed vector within zOS.  An application is then compiled with a set of macros to
convert calls to xprintf to jumps into the zOS vector table.

At the moment, these API methods are exposed:

| Vector No  |      Method                    | Prototype                                                                       | Description                                                                |
| ---------  |      ------                    | ---------                                                                       | -----------                                                                |
| 000        |      _break                    | Break handler.                                                                  | When a breakpoint instruction is encountered, method to handle it.         |
| 001        |      _putchar                  | void putchar(char c)                                                            | Send a single character at the lowest level to serial output device.       |
| 002        |      xputc                     | void xputc (char c)                                                             | Normal method to write a single character to the output stream.            | 
| 003        |      xfputc                    | void xfputc (void (*func)(unsigned char), char c)                               | Write a character to the specified output device or stream.                |
| 004        |      xputs                     | void xputs (const char* str)                                                    | Put a string onto the output stream.                                       |
| 005        |      xgets                     | int xgets (char* buff, int len)                                                 | Get a string from the input stream.                                        |
| 006        |      xfgets                    | int xfgets (unsigned char (*func)(void), char* buff, int len)                   | Get a string from the specified input device or stream.                    |
| 007        |      xfputs                    | void xfputs (void (*func)(unsigned char), const char* str)                      | Put a string onto the specified output device or stream.                   |
| 008        |      xatoi                     | int xatoi (char** str, long* res)                                               | Convert a string into a long. String can be any format or base.            |
| 009        |      uxatoi                    | int uxatoi(char **, uint32_t *)                                                 | Convert a string into an unsigned 32bit. String can be any format or base. |
| 010        |      xprintf                   | void xprintf (const char* fmt, ...)                                             | Print a formatted string onto the output stream. Floating point is not supported. |
| 011        |      xvprintf                  | void xvprintf (const char*, va_list )                                           | Print a varargs formatted string onto the output stream. Floating point is not supported. |
| 012        |      xsprintf                  | void xsprintf (char* buff, const char* fmt, ...)                                | Print a formatted string into a string. Floating point is not supported.   |
| 013        |      xfprintf                  | void xfprintf (void (*func)(unsigned char), const char* fmt, ...)               | Print a formatted string onto the specified output device or stream.       |
| 014        |      getserial                 | char getserial(void)                                                            | Wait and Get a single character at the lowest level from the serial input device. |
| 015        |      getserial_nonblocking     | int8_t getserial_nonblocking(void)                                              | Get a single character at the lowest level from the serial input device, return -1 if no character available. |
| 016        |      crc32_init                | unsigned int crc32_init(void)                                                   | Initialise a CRC32 generator.                                              |
| 017        |      crc32_addword             | unsigned int crc32_addword(unsigned int crc_in, unsigned int word)              | Add a word into the generated CRC32, returns the current CRC32 value after adding the word. |
| 018        |      get_dword                 | unsigned int get_dword(void)                                                    | Get an unsigned 32bit word (binary format, big endian) from the serial input device. |
| 019        |      rtcSet                    | uint8_t rtcSet(RTC *time)                                                       | Set the onboard Real Time Clock to the given date and time.                |
| 020        |      rtcGet                    | void rtcGet(RTC *time)                                                          | Get the current date and time from the onboard Real Time Clock.            |
| 021        |      f_open                    | f_open (FIL* fp, const TCHAR* path, BYTE mode)                                  | Open or create a file                                                      |
| 022        |      f_close                   | f_close (FIL* fp)                                                               | Close an open file object                                                  |
| 023        |      f_read                    | f_read (FIL* fp, void* buff, UINT btr, UINT* br)                                | Read data from the file                                                    |
| 024        |      f_write                   | f_write (FIL* fp, const void* buff, UINT btw, UINT* bw)                         | Write data to the file                                                     |
| 025        |      f_lseek                   | f_lseek (FIL* fp, FSIZE_t ofs)                                                  | Move file pointer of the file object                                       |
| 026        |      f_truncate                | f_truncate (FIL* fp)                                                            | Truncate the file                                                          |
| 027        |      f_sync                    | f_sync (FIL* fp)                                                                | Flush cached data of the writing file                                      |
| 028        |      f_opendir                 | f_opendir (DIR* dp, const TCHAR* path)                                          | Open a directory                                                           |
| 029        |      f_closedir                | f_closedir (DIR* dp)                                                            | Close an open directory                                                    |
| 030        |      f_readdir                 | f_readdir (DIR* dp, FILINFO* fno)                                               | Read a directory item                                                      |
| 031        |      f_findfirst               | f_findfirst (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern)    | Find first file                                                            |
| 032        |      f_findnext                | f_findnext (DIR* dp, FILINFO* fno)                                              | Find next file                                                             |
| 033        |      f_mkdir                   | f_mkdir (const TCHAR* path)                                                     | Create a sub directory                                                     |
| 034        |      f_unlink                  | f_unlink (const TCHAR* path)                                                    | Delete an existing file or directory                                       |
| 035        |      f_rename                  | f_rename (const TCHAR* path_old, const TCHAR* path_new)                         | Rename/Move a file or directory                                            |
| 036        |      f_stat                    | f_stat (const TCHAR* path, FILINFO* fno)                                        | Get file status                                                            |
| 037        |      f_chmod                   | f_chmod (const TCHAR* path, BYTE attr, BYTE mask)                               | Change attribute of a file/dir                                             |
| 038        |      f_utime                   | f_utime (const TCHAR* path, const FILINFO* fno)                                 | Change timestamp of a file/dir                                             |
| 039        |      f_chdir                   | f_chdir (const TCHAR* path)                                                     | Change current directory                                                   |
| 040        |      f_chdrive                 | f_chdrive (const TCHAR* path)                                                   | Change current drive                                                       |
| 041        |      f_getcwd                  | f_getcwd (TCHAR* buff, UINT len)                                                | Get current directory                                                      |
| 042        |      f_getfree                 | f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs)                      | Get number of free clusters on the drive                                   |
| 043        |      f_getlabel                | f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn)                        | Get volume label                                                           |
| 044        |      f_setlabel                | f_setlabel (const TCHAR* label)                                                 | Set volume label                                                           |
| 045        |      f_forward                 | f_forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf)          | Forward data to the stream                                                 |
| 046        |      f_expand                  | f_expand (FIL* fp, FSIZE_t szf, BYTE opt)                                       | Allocate a contiguous block to the file                                    |
| 004        |      f_mount                   | f_mount (FATFS* fs, const TCHAR* path, BYTE opt)                                | Mount/Unmount a logical drive                                              |
| 007        |      f_mkfs                    | f_mkfs (const TCHAR* path, BYTE opt, DWORD au, void* work, UINT len)            | Create a FAT volume                                                        |
| 048        |      f_fdisk                   | f_fdisk (BYTE pdrv, const DWORD* szt, void* work)                               | Divide a physical drive into some partitions                               |
| 049        |      f_setcp                   | f_setcp (WORD cp)                                                               | Set current code page                                                      |
| 050        |      f_putc                    | f_putc (TCHAR c, FIL* fp)                                                       | Put a character to the file                                                |
| 051        |      f_puts                    | f_puts (const TCHAR* str, FIL* cp)                                              | Put a string to the file                                                   |
| 052        |      f_printf                  | f_printf (FIL* fp, const TCHAR* str, ...)                                       | Put a formatted string to the file                                         |
| 053        |      f_gets                    | f_gets (TCHAR* buff, int len, FIL* fp)                                          | Get a string from the file                                                 |
| 054        |      disk_read                 | DRESULT disk_read ( BYTE drv, BYTE *buff, DWORD sector, UINT count )            | Read full or partial sectors directly from the SD card                     |
| 055        |      disk_write                | DRESULT disk_write ( BYTE drv, const BYTE *buff, DWORD sector, UINT count )     | Write full or partial sectors directly to the SD card                      |
| 056        |      disk_ioctl                | DRESULT disk_ioctl ( BYTE drv, BYTE ctrl, void *buff )                          | Issue direct control commands to the SD card.                              |
| 057        |      getStrParam               | char *getStrParam(char **ptr)                                                   | Get a pointer within a string parameter list of the next string parameter. The pointer to the string parameter list is updated to point to the next+1 parameter. within a passed parameter string. |
| 058        |      getUintParam              | uint32_t getUintParam(char **ptr)                                               | Get the next 32bit unsigned integer from a string parameter list. The pointer to the string parameter list is updated to point to the next+1 parameter. |
| 059        |      set_serial_output         | __inline void set_serial_output(uint8_t c)                                      | For boards with more than 1 serial port, assign the given device as the primary output stream device. |
| 060        |      printBytesPerSec          | void printBytesPerSec(uint32_t bytes, uint32_t mSec, const char *action)        | Calculate and print the performance of an SD card transaction.             |
| 061        |      printFSCode               | void printFSCode(FRESULT result)                                                | Print as text the result code from the Fat FS calls.                       |

As well as the above methods, two global parameter blocks are exposed and passed to the application which it can freely use. These are:

The global parameter block, accessed by the variable *G*:

````c
// Global parameters accessible in applications.
typedef struct {
    uint8_t                  fileInUse;                                /* Flag to indicate if file[0] is in use. */
    FIL                      File[MAX_FILE_HANDLE];                    /* Maximum open file objects */
    FATFS                    FatFs[FF_VOLUMES];                        /* Filesystem object for each logical drive */
    BYTE                     Buff[512];                                /* Working disk buffer */
    DWORD                    Sector;                                   /* Sector to read */
  #if defined __K64F__
    uint32_t volatile        *millis;                                  /* Pointer to the K64F millisecond tick */
  #endif
} GLOBALS;
````
An example of use within a program would be: xprintf("Sector:%d\n", G->Sector);

The configuration block, accessed by the variable *cfgSoC*:

````c
// Configuration values.
typedef struct
{
    uint32_t                           addrInsnBRAM;
    uint32_t                           sizeInsnBRAM;
    uint32_t                           addrBRAM;
    uint32_t                           sizeBRAM;
    uint32_t                           addrRAM;
    uint32_t                           sizeRAM;
    uint32_t                           addrSDRAM;
    uint32_t                           sizeSDRAM;
    uint32_t                           addrWBSDRAM;
    uint32_t                           sizeWBSDRAM;
    uint32_t                           resetVector;
    uint32_t                           cpuMemBaseAddr;
    uint32_t                           stackStartAddr;
    uint16_t                           zpuId;
    uint32_t                           sysFreq;
    uint32_t                           memFreq;
    uint32_t                           wbMemFreq;
    uint8_t                            implSoCCFG;    
    uint8_t                            implWB;
    uint8_t                            implWBSDRAM;
    uint8_t                            implWBI2C;
    uint8_t                            implInsnBRAM;
    uint8_t                            implBRAM;
    uint8_t                            implRAM;
    uint8_t                            implSDRAM;
    uint8_t                            implIOCTL;
    uint8_t                            implPS2;
    uint8_t                            implSPI;
    uint8_t                            implSD;
    uint8_t                            sdCardNo;
    uint8_t                            implIntrCtl;
    uint8_t                            intrChannels;
    uint8_t                            implTimer1;
    uint8_t                            timer1No;
} SOC_CONFIG;
````
An example of use in a program would be: if(cfgSoC->implSDRAM) { ... }


To build an application, it is the same as creating a normal C program (C++ to follow) but instead of using the standard 'main' entry point with argument list a new method has been 
provided, namely 'app'. This has the prototype:

uint32_t app(uint32_t param1, uint32_t param2)

param1 and param2 can be any 32bit value passed when fileExec is called within zOS. Under standard operating conditions, zOS calls an application with one parameter only, param1 which is
a (char *) pointer to the command line parameters which invoked the application.

The return code from the application to zOS is a 32bit unsigned integer. 0 means the application executed successfully, 0xFFFFFFFF indicates a failure and anything inbetween is for reference or to be used 
by future commands for conditional processing.

The standard template for an app() is as follows:

````c
// Main entry and start point of a zOS/ZPUTA Application. Only 2 parameters are catered for and a 32bit return code, additional parameters can be added by changing the appcrt0.s
// startup code to add them to the stack prior to app() call.
//
// Return code for the ZPU is saved in _memreg by the C compiler, this is transferred to _memreg in zOS/ZPUTA in appcrt0.s prior to return.
// The K64F ARM processor uses the standard register passing conventions, return code is stored in R0.
//
uint32_t app(uint32_t param1, uint32_t param2)
{
    // Initialisation.
    //
    char      *ptr = (char *)param1;
    long      startAddr;
    long      endAddr;
    long      bitWidth;
    long      xferSize;
  #if defined __K64F__
    uint32_t  perfTime;
  #endif
    uint32_t  retCode = 0;

    if (!xatoi(&ptr, &startAddr))
    {
        xprintf("Illegal <start addr> value.\n");
        retCode = 0xFFFFFFFF;
    } else if (!xatoi(&ptr, &endAddr))
    {
        xprintf("Illegal <end addr> value.\n");
        retCode = 0xFFFFFFFF;
    } else
    {
        xatoi(&ptr,  &bitWidth);
        if(bitWidth != 8 && bitWidth != 16 && bitWidth != 32)
        {
            bitWidth = 32;
        }
        if(!xatoi(&ptr,  &xferSize))
        {
            xferSize = 10;
        }
    }

    ... application logic

    return(retCode);
}
````


## Software Build

### Paths

For ease of reading, the following shortnames refer to the corresponding path in this chapter.

|  Short Name      |                                                                            |
|------------------|----------------------------------------------------------------------------|
| \[\<ABS PATH>\]  | The path where this repository was extracted on your system.               |
| \<apps\>         | \[\<ABS PATH>\]/zsoft/apps                                                 |
| \<build\>        | \[\<ABS PATH>\]/zsoft/build                                                |
| \<common\>       | \[\<ABS PATH>\]/zsoft/common                                               |
| \<libraries\>    | \[\<ABS PATH>\]/zsoft/libraries                                            |
| \<teensy3\>      | \[\<ABS PATH>\]/zsoft/teensy3                                              |
| \<include\>      | \[\<ABS PATH>\]/zsoft/include                                              |
| \<startup\>      | \[\<ABS PATH>\]/zsoft/startup                                              |
| \<iocp\>         | \[\<ABS PATH>\]/zsoft/iocp                                                 |
| \<zOS\>          | \[\<ABS PATH>\]/zsoft/zOS                                                  |
| \<zputa\>        | \[\<ABS PATH>\]/zsoft/zputa                                                |
| \<rtl\>          | \[\<ABS PATH>\]/zsoft/rtl                                                  |
| \<docs\>         | \[\<ABS PATH>\]/zsoft/docs                                                 |
| \<tools\>        | \[\<ABS PATH>\]/zsoft/tools                                                |


### Tools

All development has been made under Linux, specifically Debian/Ubuntu. Besides the standard Linux buildchain, the following software is needed.

|                                                           |                                                                                                                     |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
[ZPU GCC ToolChain](https://github.com/zylin/zpugcc)        | The GCC toolchain for ZPU development. Install into */opt* or similar common area.                                  |
[Arduino](https://www.arduino.cc/en/main/software)          |  The Arduino development environment, not really needed unless adding features to the K64F version of zOS from the extensive Arduino library. Not really needed, more for reference. |
[Teensyduino](https://www.pjrc.com/teensy/td_download.html) | The Teensy3 Arduino extensions to work with the Teensy3.5 board at the Arduino level. Not really needed, more for reference. |

For the Teensy3.5/K64F the ARM compatible toolchain is stored in the repo within the build tree.


### Build Tree

The software is organised into the following tree/folders:

| Folder    | Src File | Description                                                  |
| -------   | -------- | ------------------------------------------------------------ |
| apps      |          | The zOS/ZPUTA applications applets tree. These are separate standalone disk based applets which run under zOS/ZPUTA. <br/>All applets for zOS/ZPUTA are stored within this folder. |
| build     |          | Build tree output suitable for direct copy to an SD card.<br/> The initial bootloader and/or application as selected are compiled directly into a VHDL file for preloading in BRAM in the devices/sysbus/BRAM folder. |
| common    |          | Common C modules such as Elm Chan's excellent Fat FileSystem. |
| docs      |          | All documents relevant to this project.                       |
| libraries |          | Code libraries, soon to be populated with umlibc and FreeRTOS |
| rtl       |          | Register Transfer Level templates and generated files. These files contain the preloaded VHDL definition of a BRAM based RAM/ROM used with the ZPU.|
| teensy3   |          | Teensy 3.5 files required for building a K64F processor target. |
| include   |          | C Include header files.                                      |
| iocp      |          | A small bootloader/monitor application for initialization of the ZPU. Depending upon configuration this program can either boot an application from SD card or via the Serial Line and also provide basic tools such as memory examination. |
| startup   |          | Assembler and Linker files for generating ZPU applications. These files are critical for defining how GCC creates and links binary images as well as providing the micro-code for ZPU instructions not implemented in hardware. |
| tools     |          | Some small tools for converting binary images into VHDL initialization data. |
| zputa     |          | The ZPU Test Application. This is an application for testing the ZPU and the SoC components. It can either be built as a single image for pre-loading into a BRAM via VHDL or as a standalone application loaded by the IOCP bootloader from an SD card. The services it provides can either be embedded or available on the SD card as applets depending on memory restrictions. |
| zOS       |          | The z-Operating System. This is a light OS derived from ZPUTA with the aim of providing a framework, both for application building and runtime control, within a ZPU or K64F project. The OS provides disk based services, common hardware access API and additional features to run and service any application built for this enviroment. Applications can be started autonomously or interactively via the serial based VT100 console. |
|           | build.sh | Unix shell script to build IOCP, zOS, ZPUTA and Apps for a given design.<br> ![build.sh](../images/build_sh.png) |


### Build.sh

zOS is built using the 'build.sh' script. This script encapsulates the Makefile system with it's plethora of flags and options. The synopsis of the script is below:
```bash
NAME
    build.sh -  Shell script to build a ZPU/K64F program or OS.

SYNOPSIS
    build.sh [-CIOoMBAsdxh]

DESCRIPTION

OPTIONS
    -C <CPU>      = Small, Medium, Flex, Evo, K64F - defaults to Evo.
    -I <iocp ver> = 0 - Full, 1 - Medium, 2 - Minimum, 3 - Tiny (bootstrap only)
    -O <os>       = zputa, zos
    -o <os ver>   = 0 - Standalone, 1 - As app with IOCP Bootloader,
                    2 - As app with tiny IOCP Bootloader, 3 - As app in RAM 
    -M <size>     = Max size of the boot ROM/BRAM (needed for setting Stack).
    -B <addr>     = Base address of <os>, default -o == 0 : 0x00000 else 0x01000 
    -A <addr>     = App address of <os>, default 0x0C000
    -N <size>     = Required size of heap
    -n <size>     = Required size of application heap
    -S <size>     = Required size of stack
    -s <size>     = Required size of application stack
    -a <size>     = Maximum size of an app, defaults to (BRAM SIZE - App Start Address - Stack Size) 
                    if the App Start is located within BRAM otherwise defaults to 0x10000.
    -d            = Debug mode.
    -x            = Shell trace mode.
    -h            = This help screen.

EXAMPLES
    build.sh -O zputa -B 0x00000 -A 0x50000

EXIT STATUS
     0    The command ran successfully

     >0    An error ocurred.
```

Sensible defaults are configured into the script, the overriding flags are described below.

| Flag&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;             | Description                                                                                                                         |
| ---------------------------------------------------------------------------------- | -----------                                                                                                                         |
| -C \<CPU\>       | The target CPU of the finished image. This flag is necessary As each ZPU has a different configuration and therefore the underlying Make needs to be directed into its choice of startup code and configuration. With the inclusion of the ARM based K64F this flag makes further sense.<br><br>The choices are:<br>*Small* - An absolute minimum ZPU hardware design with most instructions implemented in microcode.<br>*Medium* - An interim ZPU design using more hardware and better performance, majority of instructions are embedded in hardware.<br>*Flex* - A ZPU small based design but with many enhanced features, including the majority of instructions being embedded in hardware.<br>*Evo* - The latest evolution of the ZPU, all instructions are embedded in hardware, framework to add extended instructions and 2 stage cache to aid performance.<br>*K64F* - An ARM Cortex-M4 based processor from Freescale.<br>*Evo is the detault.* |
| -I \<iocp ver\>  | Set the memory model for the build. The memory models are described above.<br><br>The choices are:<br>*0*- Full<br>*1* - Medium<br>*2* - Minimum<br>*3* - Tiny (bootstrap only).<br>*Default is set to 3.* |
| -O \<os\>        | Set the target operating system.<br><br>The choices are:<br>*zputa* - The ZPU Test Application which also runs on the K64F.<br>*zos* - The zOS Operating System.<br>There is no default, if an OS is being built then this flag must be given. |
| -o \<os ver\>    | Set the operating system target model.<br><br>The choices are:<br>*0* - Standalone, the operating system is the startup firmware.<br>*1* - As app with IOCP Bootloader, the operating system is booted by IOCP from an SD card, IOCP remains memory resident.<br>*2* - As app with tiny IOCP Bootloader, the operating system is booted from SD card by the smallest IOCP possible, IOCP remains memory resident.<br>*3* - As app in RAM, the operating system is targetted to be loaded in external (to the FPGA) RAM, either static or dynamic, IOCP remains memory resident in BRAM.<br>*The default is 2.*<br>**This flag has no meaning for the K64F target.** |
| -M \<size\>      | Specify the maximum size of the boot ROM/BRAM (needed for setting Stack).<br>**This flag has no meaning on the K64F target.** |
| -B \<addr\>      | Base address of \<os\>.<br>*default: if -o == 0 : 0x00000 else 0x01000*<br>**This flag has no meaning on the K64F target.**                                                                        |
| -A \<addr\>      | App starting/load address for the given \<os\>.<br>*default 0x0C000 for ZPU, 0x1FFF0000 for K64F*                                                                                                |
| -N \<size\>      | Required size of \<os\> heap. The OS and application heaps are currently distinct. Should RTOS be integrated into zOS then this will change. |
| -n \<size\>      | Required size of application heap. The application has a seperate heap allocated and is managed by the application. |
| -S \<size\>      | Required size of stack. Normally the \<os\> manages the stack which is also used by an application. If the application is considered dangerous then it should be allocated a local stack. |
| -s \<size\>      | Required size of application stack. Normally this should not be required as the application shares the \<os\> stack. If the application is dangerous then set this value to allocate a local stack. |
| -a \<size\>      | Maximum size of an application.<br>*defaults to (BRAM SIZE - App Start Address - Stack Size) for the ZPU if the App Start is located within BRAM otherwise defaults to 0x10000, or 0x20030000 - Stack Size - Heap Size - 0x4000 for the K64F. If the App Start is located within BRAM otherwise defaults to 0x10000.*                                                              |
| -d               | Debug mode. Enable more verbose output on state and decisions made by the script. |
| -x               | Shell trace mode. A very low level, line by line trace of shell execution. Only used during debugging. |
| -h               | This help screen.                                                                                                                   |

<br>
To build the standard zOS to be run on a Freescale K64F processor, you would issue the command:

```bash
<ABS PATH>/build.sh -C K64F -O zos -B 0x00000000 -N 0x00002000 -S 0x00001000 -A 0x1FFF0000 -n 0x00020000 -s 0x00000000 
```

This specifies that the target CPU is the K64F processor (-C K64F), the target OS is zOS (-O zos), the base or starting address of the generated firmware is 0x00000000 (-B 0x00000000), the heap for zOS is 0x00002000 bytes in size (-N 0x00002000),
the stack for zOS is 0x000010000 bytes in size (-S 0x000010000), the starting address for applications is base of RAM at 0x1FFF0000 (-A 0x1FFF0000), the application heap size is 0x00020000 (-n 0x00020000), the application doesnt have a local
stack (-s 0x00000000).

When complete, the output will be stored in two locations:<br>
\<zOS>/main.(hex | bin | srec) - The main zOS firmware in 3 standard formats which can be flashed directly into the K64F FlashRAM at 0x00000000.<br>
\<build>/SD - Applications (and kernels if using the ZPU processor with IOCP) which can be copied directly to a FAT32 formatted SD card.<br>



<br>
## Credits

Where I have used or based any component on a 3rd parties design I have included the original authors copyright notice within the headers or given due credit. All 3rd party software, to my knowledge and research, is open source and freely useable, if there is found to be any component with licensing restrictions, it will be removed from this repository and a suitable link/config provided.


<br>
## Licenses

The original ZPU uses the Free BSD license and such the Evo is also released under FreeBSD. SoC components and other developments written by me are currently licensed using the GPL. 3rd party components maintain their original copyright notices.

### The FreeBSD license
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those of the authors and should not be interpreted as representing official policies, either expressed or implied, of the this project.

### The Gnu Public License v3
 The source and binary files in this project marked as GPL v3 are free software: you can redistribute it and-or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

 The source files are distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/.


