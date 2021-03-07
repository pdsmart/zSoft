### Application Commands

The advantages of using an SD card with zOS/ZPUTA is it allows for application applets to be stored on disk and run as needed.
These applications are more akin to MSDOS commands as they utilise core API functionality within zOS/ZPUTA but otherwise
manage their own run and memory environments.

Listed below are all the current application applets built for zOS/ZPUTA.
<br>

#### ed

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | ed  \<file\>                                                                                            |
  | Parameters:  | \<file\> - Filename, including relative or absolute pathname.                                           |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |

  A Basic WYSIWYG File Editor.

  This is a *What You See Is What You Get* visual editor. It doesnt offer many features but is great for quickly editting or adding files. It is designed
  around the VT100 terminal and thus uses its control sequences to obtain a visual editor. In order to use this editor
  you must be using a VT100 emulator program such as Minicom on the serial connection of the ZPU/K64F.
 
  The commands and control keys for the editor are:
 
  | Command / Key                        | Description                                                                     |
  | -------------                        | -----------                                                                     |
  | CTRL C                               | Ignored.                                                                        |
  | CTRL Q                               | Quit. If changes have been made to the file you will be warned. To ignore the warning, press CTRL-Q 3 times. |
  | CTRL S                               | Save. Save the file contents to disk.                                           |
  | CTRL F                               | Find. Search for a word in the document.                                        |
  | CTRL H<br>BACKSPACE                  | Backspace, delete character to the left of cursor.                              |
  | DELETE                               | Delete, delete character under cursor, shifting characters on the right, left by 1. |
  | HOME                                 | Go to start of line.                                                            |
  | END                                  | Go to end of line                                                               |
  | PAGE UP                              | Scroll up by 1 page.                                                            |
  | PAGE DOWN                            | Scroll down by 1 page.                                                          |
  | ARROW UP                             | Move cursor up 1 row. If at top of screen but not top of document, scroll screen down 1 row. |
  | ARROW DOWN                           | Move cursor down 1 row. If at bottom of screen but not bottom of document, scroll screen up 1 row. |
  | ARROW RIGHT                          | Move cursor right by 1. If at end of line, wrap around to beginning of next line. |
  | ARROW LEFT                           | Move cursor left by 1. If at beginning of line, wrap around to end of previous line. |
  | CTRL L                               | Refresh/re-draw screen.                                                         |
  | ESCAPE                               | Ignored.                                                                        |




#### kilo 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | kilo \<file\>                                                                                           |
  | Parameters:  | \<file\> - Filename, including relative or absolute pathname.                                           |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |

  A simple WYSIWYG File Editor.

  This is a *What You See Is What You Get* visual editor. It offers a bit more functionality than 'ed', for example syntax
  highlighting of recognised source files and highlighting of matched words in the find function. It is also designed around
  the VT100 terminal and thus uses its control sequences to obtain a visual editor. In order to use this editor you must be
  using a VT100 emulator program such as Minicom on the serial connection of the ZPU/K64F.
 
  The commands and control keys for the editor are:
 
  | Command / Key                        | Description                                                                     |
  | -------------                        | -----------                                                                     |
  | CTRL C                               | Ignored.                                                                        |
  | CTRL Q                               | Quit. If changes have been made to the file you will be warned. To ignore the warning, press CTRL-Q 3 times. |
  | CTRL S                               | Save. Save the file contents to disk.                                           |
  | CTRL F                               | Find. Search for a word in the document.                                        |
  | CTRL H<br>BACKSPACE                  | Backspace, delete character to the left of cursor.                              |
  | DELETE                               | Delete, delete character under cursor, shifting characters on the right, left by 1. |
  | HOME                                 | Go to start of line.                                                            |
  | END                                  | Go to end of line                                                               |
  | PAGE UP                              | Scroll up by 1 page.                                                            |
  | PAGE DOWN                            | Scroll down by 1 page.                                                          |
  | ARROW UP                             | Move cursor up 1 row. If at top of screen but not top of document, scroll screen down 1 row. |
  | ARROW DOWN                           | Move cursor down 1 row. If at bottom of screen but not bottom of document, scroll screen up 1 row. |
  | ARROW RIGHT                          | Move cursor right by 1. If at end of line, wrap around to beginning of next line. |
  | ARROW LEFT                           | Move cursor left by 1. If at beginning of line, wrap around to end of previous line. |
  | CTRL L                               | Refresh/re-draw screen.                                                         |
  | ESCAPE                               | Ignored.                                                                        |



#### mbasic

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mbasic \[\<file\>\]                                                                                     |
  | Parameters:  | \<file\> - Filename of a basic program, including relative or absolute pathname.                        |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |

  An Interactive Mini-Basic Interpreter.

  This is a fully interactive BASIC interpreter which utilises an enhanced version of Malcom McLean's original [Mini-Basic](https://malcolmmclean.github.io/minibasic/web/MiniBasicHome.html) 
  which provides a full screen editor, a command processor with the required interactive commands and a readline based text entry for easy editting of commands.
  The BASIC program once captured is sent to the BASIC interpreter when the 'run' command is issued. The BASIC interpreter has been enhanced to include additional commands for use in an
  embedded environment.

  Please consult Malcolm McLean's [Mini-Basic](https://malcolmmclean.github.io/minibasic/web/MiniBasicHome.html) homepage
  for the standard BASIC language manual, this document only details the enhancements.

  The original Mini-Basic was designed to run within Windows or Linux and take a file containing a BASIC program and run it. Within
  an embedded environment this is not so convenient as you want to be able to enter lines or commands and get feedback immediately.
  This is generally the case in an environment such as this where you are programming or controlling hardware under development.

  When I was looking around for a BASIC interpreter for zOS/ZPUTA one of the requirements was size and the other being the 
  limited libraries provided by GCC for the ZPU, also GCC being an old version it only worked with C99/C++98. I came across a BASIC
  interpreter called [MY_BASIC](https://github.com/paladin-t/my_basic) which seemed to fit the bill but its compiled size
  is over 250K and wouldnt compile with zpu-elf-gcc. A further search turned up Tiny Basic which is small and useable
  but not so easy to expand. Malcolm McLean's Mini-Basic on the other hand was small, well written and easily extendable 
  hence deciding to work with it and add the missing interactive features.

  The concept of MiniBasic in this environment is the integration of 'ed' above to give a full visual editor to enter
  your code, a readline processor to give command line recall and editting along with direct BASIC entry outside of the editor
  and a command processor to add the missing commands such as RUN, LIST, NEW, LOAD, SAVE, QUIT etc. 
  
  On starting MiniBasic, you are greeted by a familiar 'Ready' prompt. You can now enter BASIC directly on the command line
  or invoke the full screen editor using the 'EDIT' command. When finished, save-quit and issue the 'RUN' command. Use this
  development loop to create the desired program.
  
  The sections below list the commands and functions available within each section of the Mini-Basic Interpreter.

  Command Line Entry

  Command line entry, after the 'Ready' prompt uses a basic version of readline with history and so has the following key
  commands/combinations. Readline is also invoked whenever the BASIC interpreter requires input.

  | Command / Key                        | Description                                                                     |
  | -------------                        | -----------                                                                     |
  | CTRL A<br>HOME                       | Move cursor to start of line.                                                   |
  | CTRL B<br>ARROW LEFT                 | Move cursor to left by 1 character.                                             |
  | CTRL C                               | Kill any running program.                                                       |
  | CTRL D                               | Ignored.                                                                        |
  | CTRL E<br>END                        | Move cursor to end of line.                                                     |
  | CTRL F<br>ARROW RIGHT                | Move cursor to right by 1 character.                                            |
  | DELETE                               | Delete, delete character under cursor, shifting characters on the right, left by 1. |
  | PAGE UP                              | Ignored.                                                                        |
  | PAGE DOWN                            | Ignored.                                                                        |
  | CTRL K                               | Clear the line.                                                                 |
  | CTRL P<br>ARROW UP                   | Recall the previous command or line entered (ie. history buffer)                |
  | CTRL N<br>ARROW DOWN                 | Recall the next command or line entered (ie. history buffer)                    |

  In addition to the key commands, readline also understands some typed commands, namely:

  | Command     | Action                                                       |
  | -------     | ------                                                       |
  | !\<number\> | Recall and execute given historised command identified by \<number\>. |
  | hist\[ory\] | List the history buffer.                                     |

  Sat in front of readline is another command processor which interprets the BASIC commands not handled by the BASIC interpreter. To the
  user, this command interpreter and readline appear the same as they are entered at the same prompt.

  The list of BASIC commands understood by the command processor are:

  | Command&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Description                                                  |
  | -------                       | -----------                                                  |
  | edit                          | Invoke the full screen editor with the BASIC program, control keys specified below. |
  | run                           | Execute the BASIC program.                                   |
  | list \[\[\<start>\] \<end>\]  | List the BASIC program to the terminal. Without options the whole program is listed. If \<start> is provided then the listing commences at this line number, if \<end> is provided then the listing stops at this line number. |
  | new                           | Delete the BASIC program stored in memory and reset environment. |
  | load \"\<file.bas\>\"         | Load a BASIC program from SD disk into memory. If a program is already in memory it will be overwritten. |
  | save \"\<file.bas\>\"         | Save the BASIC program in memory onto the SD disk using the filename given. |
  | help                          | Print out the commands understood by this command processor. |
  | quit                          | Clear memory and exit to zOS/ZPUTA. If an unsaved BASIC program is held in memory the command will abort with a warning. |

  The BASIC interpreter has also been enhanced to include the following necessary embedded commands:

  | Command&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Description                                                  |
  | -------                              | -----------                                                  |
  | POKE \<width>, \<addr>, \<data>      | Write \<data> into memory address \<addr> using the given \<width> (32, 16 or 8bit). For 32 and 16bit writes the address must be word or half-word aligned.<br>Example:<br>&nbsp;&nbsp;10 POKE 32, 0x20028000, 0xaa55aa55 |

  ... and embedded functions:

  | Function&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Description                                                  |
  | --------                             | -----------                                                  |
  | PEEK(\<width>, \<addr>)              | Read memory address \<addr> and return the value found according to the \<width> specification. Width = 32, 16 or 8. For 32 or 16bit reads the address must be word or half-word aligned,<br>Example:&nbsp;&nbsp;20 PRINT PEEK(32, 0x20028000) |

  Last but not least is the visual editor, 'ed'. This is a variant of the application 'ed' which has been adapted to blend in better with an interactive BASIC session. Its commands may differ, so listed below is all the control keys understood by the inbuilt 'ed'.

 
  | Command / Key                        | Description                                                                     |
  | -------------                        | -----------                                                                     |
  | CTRL C                               | Ignored.                                                                        |
  | CTRL Q                               | Quit. If changes have been made to the file you will be warned. To ignore the warning, press CTRL-Q 3 times. |
  | CTRL S                               | Save. Save the file contents to disk.                                           |
  | CTRL F                               | Find. Search for a word in the document.                                        |
  | CTRL H<br>BACKSPACE                  | Backspace, delete character to the left of cursor.                              |
  | DELETE                               | Delete, delete character under cursor, shifting characters on the right, left by 1. |
  | HOME                                 | Go to start of line.                                                            |
  | END                                  | Go to end of line                                                               |
  | PAGE UP                              | Scroll up by 1 page.                                                            |
  | PAGE DOWN                            | Scroll down by 1 page.                                                          |
  | ARROW UP                             | Move cursor up 1 row. If at top of screen but not top of document, scroll screen down 1 row. |
  | ARROW DOWN                           | Move cursor down 1 row. If at bottom of screen but not bottom of document, scroll screen up 1 row. |
  | ARROW RIGHT                          | Move cursor right by 1. If at end of line, wrap around to beginning of next line. |
  | ARROW LEFT                           | Move cursor left by 1. If at beginning of line, wrap around to end of previous line. |
  | CTRL L                               | Refresh/re-draw screen.                                                         |
  | ESCAPE                               | Ignored.                                                                        |


#### tbasic

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | tbasic                                                                                                  |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |

  Tiny Basic

  This is an interactive BASIC originally written by [RodionGork](https://github.com/RodionGork/TinyBasic) and enhanced by [Miskatino](https://github.com/Miskatino/miskatino-basic) for 
  use on STM32 and Arduino Micro development boards.

  I've ported it to the zOS/ZPUTA environment so that it compiles with both the ZPU and K64F processor architectures. I still need to change the PIN drivers so that they 
  address ZPU SoC and K64F GPIO pins but otherwise is fully functional.

  The [full manual](https://github.com/Miskatino/miskatino-basic/wiki/Miskatino-Basic-Manual) for Tiny Basic can be found [here](https://github.com/Miskatino/miskatino-basic/wiki/Miskatino-Basic-Manual).

### tranZPUterSW Commands

When zOS is compiled for the tranZPUter SW board a set of applications are compiled which can be used to monitor, probe, change or interface with the host machine. ie. download the current memory 
state for debugging or capture the video frame buffer.

#### tzload

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | tzload                                                                                                  |
  | Parameters:  | See the synopsis below.                                                                                 |
  | Architecture:| K64F                                                                                                    |
  | Host:        | zOS                                                                                                     |
  | Description: | Upload and Download files to the tranZPUter memory, grab a video frame or set a new frame.              |

```bash 
TZLOAD v1.1

Commands:-
  -h | --help              This help text.
  -d | --download <file>   File into which memory contents from the tranZPUter are stored.
  -u | --upload   <file>   File whose contents are uploaded into the traZPUter memory.
  -U | --uploadset <file>:<addr>,...,<file>:<addr>
                           Upload a set of files at the specified locations. --mainboard specifies mainboard is target, default is tranZPUter.
  -V | --video             The specified input file is uploaded into the video frame buffer or the specified output file is filled with the video frame buffe.

Options:-
  -a | --addr              Memory address to read/write.
  -l | --size              Size of memory block to read. This option is only used when reading tranZPUter memory, for writing, the file size is used.
  -s | --swap              Read tranZPUter memory and store in <infile> then write out <outfile> to the same memory location.
  -f | --fpga              Operations will take place in the FPGA memory. Default without this flag is to target the tranZPUter memory.
  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter memory.
  -z | --mzf               File operations are to process the file as an MZF format file, --addr and --size will override the MZF header values if needed.
  -v | --verbose           Output more messages.

Examples:
  tzload --download monitor.rom -a 0x000000      # Load the file monitor.rom into the tranZPUter memory at address 0x000000.
```

#### tzdump

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | tzdump                                                                                                  |
  | Parameters:  | See the synopsis below.                                                                                 |
  | Architecture:| K64F                                                                                                    |
  | Host:        | zOS                                                                                                    |
  | Description: | Dump tranZPUter memory to screen.                                                                       |

```bash 
TZDUMP v1.1

Commands:-
  -h | --help              This help text.
  -a | --start             Start address.

Options:-
  -e | --end               End address (alternatively use --size).
  -s | --size              Size of memory block to dump (alternatively use --end).
  -f | --fpga              Operations will take place in the FPGA memory. Default without this flag is to target the tranZPUter memory.
  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter memory.
  -v | --verbose           Output more messages.

Examples:
  tzdump -a 0x000000 -s 0x200   # Dump tranZPUter memory from 0x000000 to 0x000200.
```

#### tzclear

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | tzclear                                                                                                 |
  | Parameters:  | See the synopsis below.                                                                                 |
  | Architecture:| K64F                                                                                                    |
  | Host:        | zOS                                                                                                     |
  | Description: | Clear tranZPUter memory.                                                                                |

```bash
TZCLEAR v1.1

Commands:-
  -h | --help              This help text.
  -a | --start             Start address.

Options:-
  -e | --end               End address (alternatively use --size).
  -s | --size              Size of memory block to clear (alternatively use --end).
  -b | --byte              Byte value to place into each cleared memory location, defaults to 0x00.
  -f | --fpga              Operations will take place in the FPGA memory. Default without this flag is to target the tranZPUter memory.
  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter memory.
  -v | --verbose           Output more messages.

Examples:
  tzclear -a 0x000000 -s 0x200 -b 0xAA  # Clears memory locations in the tranZPUter memory from 0x000000 to 0x000200 using value 0xAA.
```

#### tzclk

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | tzclk                                                                                                   |
  | Parameters:  | See the synopsis below.                                                                                 |
  | Architecture:| K64F                                                                                                    |
  | Host:        | zOS                                                                                                     |
  | Description: | Set the Z80 alternative CPU frequency.                                                                  |

```bash
TZCLK v1.0

Commands:-
  -h | --help              This help text.
  -f | --freq              Desired CPU clock frequency.

Options:-
  -e | --enable            Enable the secondary CPU clock.
  -d | --disable           Disable the secondary CPU clock.
  -v | --verbose           Output more messages.

Examples:
  tzclk --freq 4000000 --enable  # Set the secondary CPU clock frequency to 4MHz and enable its use on the tranZPUter board.
```


#### tzreset

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | tzreset                                                                                                 |
  | Parameters:  | See the synopsis below.                                                                                 |
  | Architecture:| K64F                                                                                                    |
  | Host:        | zOS                                                                                                     |
  | Description: | Reset the tranZPUter.                                                                                   |

```bash
TZRESET v1.0

Commands:-
  -h | --help              This help text.
  -r | --reset             Perform a hardware reset.
  -l | --load              Reload the default ROMS.
  -m | --memorymode <val>  Set the memory mode.

Options:-
  -v | --verbose           Output more messages.

Examples:
  tzreset -r        # Resets the Z80 and associated tranZPUter logic.
```

#### tzio

| -------  | ----------------------------------------------- |
| tzio     | Z80 I/O Port read/write tool.                   |

```bash
TZIO v1.1

Commands:-
  -h | --help              This help text.
  -o | --out <port>        Output to I/O address.
  -i | --in <port>         Input from I/O address.

Options:-
  -b | --byte              Byte value to send to the I/O port in the --out command, defaults to 0x00.
  -m | --mainboard         Operations will take place on the MZ80A mainboard. Default without this flag is to target the tranZPUter I/O domain.
  -v | --verbose           Output more messages.

Examples:
  tzio --out 0xf8 --byte 0x10    # Setup the FPGA Video mode at address 0xf8.
```

#### tzflupd


| -------  | ----------------------------------------------- |
| tzflupd  | K64F FlashRAM update tool. Update the running zOS/ZPUTA kernel with a later version. |

```bash
TZFLUPD v1.1

Commands:-
  -h | --help              This help text.
  -f | --file              Binary file to upload and flash into K64F.

Options:-
  -v | --verbose           Output more messages.

Examples:
  tzflupd -f zOS_22012021_001.bin --verbose   # Upload and program the zOS_22012021_001.bin file into the K64F flash memory.
```




### Performance Testing Commands

One of the key questions with ZPU hardware or indeed any CPU is how it's performance compares to other CPU's. It is also a key indicator if design changes, such as CPU Cache algorithms are
beneficial for general processing purposes.

To this end I have ported two well known Performance benchmark programs to the zOS/ZPUTA environment, namely Dhrystone and CoreMark. Dhrystone was the original defacto indicator of performance
and it ascertains a Dhrystone MIPS value which can then be compared against a VAX 11/780 which benchmarks at 1DMIPS. This gives a reasonable indicator of CPU performance.

A later addition to the performance testing came about with CoreMark which tries to overcome the deficiencies in Dhrystone testing. CoreMark is generally used now in preference to Dhrystone.

Interestingly, running CoreMark, the 120MHz K64F ARM Cortex-M4 (Score 86) is about 4 times faster than the ZPU Evo (Score 22) showing the advantages of more advanced architectures, caching algorithms
and silicon fabrication as opposed to a standard programmed FPGA. No doubt on better FPGA's than I've been using the ZPU Evo will narrow the gap but the price is prohibitive (ie. Arria/Stratix FPGA's).
<br>

#### dhry 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | dhry                                                                                                    |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Run the [Dhrystone Test v2.1](https://en.wikipedia.org/wiki/Dhrystone) to ascertain the performance of the CPU and memory subsystem. This is useful to compare CPU models. |

#### coremark

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | coremark                                                                                                |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Run the [CoreMark Test v1.0](https://www.eembc.org/coremark/) to ascertain a more recent performance analysis of the CPU and memory subsystem. This is useful to compare CPU models. |


### Disk IO Commands

This is a set of low level disk utilities working just above the SD SPI interface. The commands are issued direct to the SD card and data read/written accordingly. Generally, these commands
are used in testing or repairing an SD card.
<br>

#### ddump

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | ddump \[<pd#> \<sect>]<br>                                                                              |
  | Parameters:  | \[<pd#> \<sect>]<br>All parameters are optional. If no parameter given then the previous or default will be used.<br>\<pd#\> - Physical disk Id, ie. 0 for first drive.<br>\<sect\> - LBA sector number to dump. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Dump a requested disk sector to screen as a hex listing.                                                |

#### dinit

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | dinit \<pd#> \[\<card type>]                                                                            |
  | Parameters:  | \<pd#\> - Physical disk Id, ie. 0 for first drive.<br>\<card type\> - Optional card type, 1 = MMC ver 3, 2 = SD ver 1, 4 = SD ver 2.  |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Initialize the SD disk system, required whenever there is a card change, performed automatically on boot. |

#### dstat

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | dstat \<pd#>                                                                                            |
  | Parameters:  | \<pd#\> - Physical disk Id, ie. 0 for first drive.                                                      |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Show the disk status (ie. initialised).                                                                 |

#### dioctl

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | dioctl \<pd#>                                                                                           |
  | Parameters:  | \<pd#\> - Physical disk Id, ie. 0 for first drive.                                                      |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Currently ioctl isnt fully implemented, only CTRL_SYNC exists which flushes any data held in memory to the SD disk.| 

### Disk Buffer Commands

This is a set of low level SD disk commands which work with a buffer, enabling fetching and manipulation of data on the SD card. Generally, these commands
are used in testing or repairing an SD card.

#### bdump

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | bdump \<ofs>                                                                                            |
  | Parameters:  | \<ofs> - Offset within the disk buffer to start dumping from.                                           |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Dump the in-memory disk buffer, which was filled with the bread, bfill, bedit commands.                 |

#### bedit

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | bedit \<ofs> \[\<data>] ...                                                                             |
  | Parameters:  | \<ofs> - Offset within the disk buffer to make edit changes.\<data> ... - space seperated list of bytes to place into the disk buffer. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Edit the in-memory disk buffer to make changes prior to SD card update with bwrite.                     |

#### bfill

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | bfill \<val>                                                                                            |
  | Parameters:  | \<val> - 8bit value with which to fill the disk buffer.                                                 |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Fill the in-memory disk buffer with a fixed 8bit value \<val>.                                          |

#### blen 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | blen \<len>                                                                                             |
  | Parameters:  | \<len> - Set the size of the disk buffer and the bread/bwrite block size to value \<len>.               |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Set the read/write length for fread/fwrite command, also sets the in-memory buffer size.                |

#### bread

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | bread \<pd#> \<sect> \[\<num>]                                                                          |
  | Parameters:  | \<pd#\> - Physical disk Id, ie. 0 for first drive.<br>\<sect\> - LBA sector number to read.<br>\<num> - Number of sectors to read. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Read a sector(s) into the in-memory disk buffer to view/change.                                         |

#### bwrite

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | bwrite \<pd#> \<sect> \[\<num>]                                                                         |
  | Parameters:  | \<pd#\> - Physical disk Id, ie. 0 for first drive.<br>\<sect\> - LBA sector number to read.<br>\<num> - Number of sectors to write. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Write contents of the in-memory disk buffer to the SD card disk for the given number of sector(s).      |

### Filesystem Commands

These are the Fat Filesystem level commands operating directly with Elm-Chan FatFS libraries. They format, mount, read, write and manipulate files
stored within the Microsoft FAT32/exFAT filesystem. The commands are very similar to MSDOS commands just preceeded with an 'f' to
differentiate them from the other commands within zOS/ZPUTA.

#### falloc

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | falloc \<fsz> \<opt>                                                                                    |
  | Parameters:  | \<fsz> - Expand file to this size.<br>\<opt> - 0 to preallocate, 1 to allocate immediately.             |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Allocate contiguous blocks to the open file (ie. preallocate space.) The file contents are wiped so do not use this to expand an existing file. |

#### fattr

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fattr \<atrr> \<mask> \<name>                                                                           |
  | Parameters:  |\<attr> - Logical OR of one or more of the following attributes: 0x1 = Read Only, 0x2 = Hidden, 0x4 - System, 0x20 = Archived.<br>\<mask> - Attribute mask that specifies which attribute is changed.<br>\<name> - Filename to change. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Change a files attribute/mode, similar to chmod on Unix.                                                |

#### fcat

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fcat \<name>                                                                                            |
  | Parameters:  | \<name> - Absolute or relative path and filename.                                                       |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Output the contents of file to the console.                                                             |

#### fcd

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fcd \<path>                                                                                             |
  | Parameters:  | \<path> - Absolute or relative path.                                                                    |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Change current directory to the one given.                                                              |

#### fclose

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fclose                                                                                                  |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Close a file previously opened by the fopen command.                                                    |

#### fconcat

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fconcat \<src fn1> \<src fn2> \<dst fn>                                                                 |
  | Parameters:  | \<src fn1> - Source filename 1, including absolute or relative path.<br>\<src fn2> - Source filename 1, including absolute or relative path.<br>\<dst fn> - Destination file to be created, including absolute or relative path.|
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Combine the two given source files into the destination file.                                           |

#### fcp

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fcp \<src file> \<dst file>                                                                             |
  | Parameters:  | \<src file> - Source file, including absolute or relative path.<br>\<dst file> - Destination file, including absolute or relative path. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Copy (duplicate) a file.                                                                                |

#### fdel

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fdel \<filename>                                                                                        |
  | Parameters:  | \<filename> - Filename, including absolute or relative path.                                            |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Delete the given file from the SD disk.                                                                 |

#### fdir

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fdir \[\<path>]                                                                                         |
  | Parameters:  | \<path> - Optional Absolute or relative path.                                                           |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Show the current directory contents or the contents of the directory referenced by \<path>.             |

#### fdrive

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fdrive \<path>                                                                                          |
  | Parameters:  | \<path> - Absolute or relative path.                                                                    |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Change the current drive to the one given.                                                              |

#### fdump 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fdump \<name> \[\<width>]                                                                               |
  | Parameters:  | \<name> - Filename, including absolute or relative path.<br>\<width> - Optional word width, 8, 16 or 32 bit. Defaults to 8 bit. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Dump the ientire contents of a file to the terminal as a hex dump with the optional given word length.  |

#### fexec

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fexec \<name> \<ldAddr> \<xAddr> \<mode>                                                                |
  | Parameters:  | \<name> - Filename, including absolute or relative path.<br>\<ldAddr> - Load address in memory.<br>\<xAddr> - Execution address of the loaded file.<br>\<mode> - Execution mode, 0 = Call and return, 1 = Jump with no return. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Load and execute a given program file. Use this to test programs or to run non-standard applications.   |

#### finit

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | finit \<ld#> \[\<mount>]                                                                                |
  | Parameters:  | \<ld#> - Logical drive number, ie. 0<br>\[\<mount>] - Optional mount point for the logical drive.       |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Force initialise the volume and optionally mount it on a given mount point. This is normally used when an SD card is changed and the dinit command is used or a physical drive has more than one volume on it. |

#### finspect

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | finspect \<start pos> \<len>                                                                            |
  | Parameters:  | \<start pos> - Position in open file where to read.<br>\<len> - Length of data to read from the file.   |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Read part of a file opened with fopen and examine a portion of its contents. The contents are dumped out in 8bit hex format. |

#### flabel

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | flabel \<label>                                                                                         |
  | Parameters:  | \<label> - Upto 8 characters.                                                                           |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Set volume label to the \<label> given.                                                                 |

#### fload

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fload \<name> \[\<addr>]                                                                                |
  | Parameters:  | \<name> - Filename, including absolute or relative path.<br>\[\<addr>] - Optional address where to load the data. If this value isnt given, the file is loaded into the application start address specified at OS build time. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Load a file into memory.                                                                                |

#### fmkdir 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fmkdir \<dir name>                                                                                      |
  | Parameters:  | \<dir name> - An absolute or relative pathname for a directory.                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Create a new directory using the \<dir name> given.                                                     |

#### fmkfs 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fmkfs \<ld#> \<type> \<au>                                                                              |
  | Parameters:  | \<ld#> - Logical drive number, ie. 0<br>\<type> - 0x01 = FAT, 0x02 = FAT32, 0x04 = exFAT.\<au> - Allocation unit size. 0 = default at OS build time. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Create FAT volume on the given Logical drive.                                                           |

#### fopen 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fopen \<mode> \<file>                                                                                   |
  | Parameters:  | \<mode> - Mode to open the file, it is a logical OR of the following values: 0x00 = Open Existing, 0x01 = Read, 0x02 = Write, 0x04 = Create New, 0x08 = Create always, 0x10 = Open always, 0x30 = Open for append. \<file> - Filename, including absolute or relative path. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Open a file within the system using the given modes. The opened file can then be operated upon by commands such as fread, finspect, fwrite etc. |

#### fread 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fread \<len>                                                                                            |
  | Parameters:  | \<len> - Length of data to read.                                                                        |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Read part of a file from the current position into the in-memory buffer. Read \<len> bytes.             |

#### frename

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | frename \<orig name> \<new name>                                                                        |
  | Parameters:  | \<orig name> - Original name of file, including absolute or relative path.<br>\<new name> - New name of file, including absolute or relative path. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Rename a file from \<original> to \<new>.                                                               |

#### fsave 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fsave \<name> \<addr> \<len>                                                                            |
  | Parameters:  | \<name> - Filename, including absolute or relative path.<br>\<addr> - Address within memory as start position of the save operation.<br>\<len> - Length in bytes to write into the file from \<addr>. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Save a section of memory from \<addr> of \<len bytes into a file                                        |

#### fseek

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fseek \<ofs>                                                                                            |
  | Parameters:  | \<ofs> - Position in bytes to place the read/write pointer within the file previously open with fopen.  |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Move the filepointer within a file previously opened with fopen. This is used to move to a position within a file to enable a read/write operation at that position. |

#### fshowdir

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fshowdir                                                                                                |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Show the current directory.                                                                             |

#### fstat 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fstat \[\<path>]                                                                                        |
  | Parameters:  | \[\<path>] - Absolute or relative drive and path, ie: 0:/                                               |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Show information about the given \<path>.                                                               |

#### ftime

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | ftime \<y> \<m> \<d> \<h> \<M> \<s> \<fn>                                                               |
  | Parameters:  | \<y> = Year YYYY<br>\<m> - Month MM<br>\<d> - Day DD<br>\<h> - Hour hh<br>\<M> - Minute mm<br>\<s> - Second ss<br>\<fn> - Filename, including absolute or relative path. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Change timestamp of a given file.                                                                       |

#### ftrunc 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | ftrunc                                                                                                  |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Truncate the file at current file position, ie. if a file is 1000 bytes long, an fseek is made to 500 followed by ftrunc, the file will be shortened to 500 bytes. |

#### fwrite 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fwrite \<len>                                                                                           |
  | Parameters:  | \<len> - Length in bytes of data from the in-memory disk buffer to write to the opened file.            |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Write part of the in-memory disk buffer into the file opened with fopen.                                |

#### fxtract 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | fxtract \<src> \<dst> \<start pos> \<len>                                                               |
  | Parameters:  | \<src> - Source file, including absolute or relative path.<br>\<dst> - Destination file, including absolute or relative path.<br>\<start pos> - Startimg position within the source file to commence reading.<br>\<len> - Length of bytes to read from source file and write to destination file. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Extract a portion of a source file and write it into the destination file.                              |

### Memory Commands

These are low level memory editting, viewing, searching, testing and checking memory performance commands. Generally they are intended for testing
or debugging both hardware and software. If you write an application and it crashes, these are the first commands to turn to in understanding why it crashed.

#### mclear

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mclear \<start\> \<end\> \[\<word\>\]                                                                   |
  | Parameters:  | \<start\> - Start address to perform clear.<br>\<end\> - End address to finish clear.<br>\[\<word\>\] - Optional value (default is 0) to fill memory with. This is a 32bit word. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Clear/fill memory with a fixed value.                                                                   |

#### mcopy 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mcopy \<start\> \<end\> \<dst addr\>                                                                    |
  | Parameters:  | \<start\> - Start address to start copying.<br>\<end\> - End address to finish copying.<br>\<dst addr\> - Destination address to write the copied data. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Copy a block of memory to a given destination,                                                          |

#### mdiff

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mdiff \<start\> \<end\> \<cmp addr\>                                                                    |
  | Parameters:  | \<start\> - Start address to start comparison.<br>\<end\> - End address to finish comparison.<br>\<cmp addr\> - Location at which to start comparison of data, ie \<start> == \<cmp addr> for \<end> - \<start> bytes. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Compare a block of memory with another block.                                                           |

#### mdump 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mdump \[\<start\> \[\<end\>\] \[\<size\>\]\]                                                            |
  | Parameters:  | \<start\> - Start address of memory to dump (optional).<br>\<end\> - End address of memory dump.<br>\<size\> - Number of bytes to dump out.<br> All parameters are optional, if \<start> isnt given then the last location dumped will be taken. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Dump a block of memory to screen in hex format.                                                         |

#### mperf 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mperf \<start\> \<end\> \[\<width\>\] \[\<size\>\]                                                      |
  | Parameters:  | \<start\> - Start address for memory performance test.<br>\<end\> - End address of memory performance test.<br>\<width\> - Width of data to read/write.<br>\<size\> - Number of bytes of memory to iterate over for the test. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Test memory performance.<br>This gives a value for the CPU \<-\> Memory, not actual memory performance. As the ZPU is stack based, it uses memory to perform a memory action, so it will never realise full memory bandwidth, hence the need for extended instructions such as LDIR.|

#### msrch 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | msrch \<start\> \<end\> \<value\>                                                                       |
  | Parameters:  | \<start\> - Start address for memory search.<br>\<end\> - End address of memory search.<br>\<value\> - Value to search memory for (32 bit). |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Search memory for a given value. This app is currently a work in progress and will be expanded to search for multiple values and strings in due course.|

#### mtest

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mtest \[\<start\> \[\<end\>] \[iter\] \[test mask\]                                                     |
  | Parameters:  | \<start\> - Start address for memory test.<br>\<end\> - End address of memory test.<br>\<iter\> - Number of iterations to perform in the test..<br>\<test mask\> - a 32bit mask to indicate which tests to perform. Test mask values are:<br>0x00001000 - Perform 8 bit tests.<br>0x00002000 - perform 16 bit tests.<br>0x00004000 - Perform 32 bit tests.<br>The lower bits specify which tests to perform.<br>0x00000001 - 8bit R/W ascending test pattern.<br>0x00000002 - 8bit R/W walking test pattern.<br>0x00000004 - 8bit Read ascending test pattern.<br>0x00000008 - 8bit Write walking bit pattern.<br>0x00000010 - 8bit echo and sticky bit test.|
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Test memory with several types of tests to locate bad or sticky bits.                                   |

#### meb

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | meb \<addr\> \<byte\> \[...\]                                                                           |
  | Parameters:  | \<addr\> - Address to edit.<br>\<byte\> \[...\] - Optional list of bytes to write. If no bytes provided then enter interactive editting mode. | 
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Edit/change memory at the byte level.                                                                   |

#### meh

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | meh \<addr\> \<h-word\> \[...\]                                                                         |
  | Parameters:  | \<addr\> - Address to edit.<br>\<h-word\> \[...\] - Optional list of 16bit half-words to write. If no bytes provided then enter interactive editting mode. | 
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Edit/change memory at the half-word (16bit) level.                                                      ]

#### mew 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | mew  \<addr\> \<word\> \[...\]                                                                          |
  | Parameters:  | \<addr\> - Address to edit.<br>\<word\> \[...\] - Optional list of 32bit words to write. If no bytes provided then enter interactive editting mode. | 
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Edit/change memory at the word (32bit) level.                                                           ]

### Hardware Commands

These are hardware testing commands normally used to verify a hardware change on the ZPU SoC. They are of little general use.

#### hr 

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | hr                                                                                                      |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU                                                                                                     |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Display the ZPU SoC real-time Register Information. This app is used for debugging.                     |

#### ht

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | ht                                                                                                      |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU                                                                                                     |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Display the real-time Timer information from the ZPU SoC. This app is used for debugging.               |

### Program Execution Commands

These are program control commands for executing a program loaded into memory. Eventually a debugger will be added for completeness.

#### call

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | call \<addr>                                                                                            |
  | Parameters:  | \<addr> - Address in memory to execute code.                                                            |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Call function @ \<addr> and expect it to return with a return code.                                     |

#### jmp

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | jmp \<addr>                                                                                             |
  | Parameters:  | \<addr> - Address in memory to execute code.                                                            |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Execute code @ \<addr> and no return is expected, a reset will be required to continue with the OS normal activities. |

### Miscellaneous Commands

These are the remaining commands which dont have a home in the other groups. One command of note is 'time', this is needed
to set or query the RTC which is present on both the ZPU and the K64F.

#### restart

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | restart                                                                                                 |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Restart the OS, useful if the system goes into an unknown state.                                        |

#### reset

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | reset                                                                                                   |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | Issue the equivalent of a cold start reset of the system.                                               |

#### help

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | help \[\<cmd %>\|\<group %>]                                                                            |
  | Parameters:  | \<cmd %> - Command filter pattern to filter out only the required commands.<br>\<group %> - Group filter pattern to filter out commands which belong to a group. |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | The in-built help screen. The default action is to list out all commands currently understood within the OS including applications. If a filter pattern is given then only those commands or group of commands matching the pattern are displated. |

#### info

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | info                                                                                                    |
  | Parameters:  |                                                                                                         |
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | A simple application to display the configuration information of the underlying CPU and SoC.            |

#### time

  | ------------ | ------------------------------------------------------------------------------------------------------- |
  | Usage:       | time \[\<y> \<m> \<d> \<h> \<M> \<s>]                                                                   |
  | Parameters:  | \<y> - Year YYYY<br>\<m> - Month MM<br>\<d> - Day DD<br>\<h> - Hour hh<br>\<M> - Minute mm<br>\<s> - Second ss.|
  | Architecture:| ZPU, K64F                                                                                               |
  | Host:        | zOS, ZPUTA                                                                                              |
  | Description: | If no parameters are given then show the current time and date. If parameters given, validate and set the RTC to the given date and time. |


