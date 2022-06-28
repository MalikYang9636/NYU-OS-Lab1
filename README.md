# NYU-OS-Lab1

Implements a two-pass linker in Operating System.

Compile:

Makefile is in included, so go to the directory where the linker.cpp is ang type 'make' in terminal, the executable file 'linker' will be created.
    
Test: 

The executable file 'linker' and the labsamples including all input files, runit.sh and gradeit.sh should be in the same directory. Move on to the labsamples, creat a new directory <your-outdir>. Then run:

    ./runit.sh <your-outdir>
    ./gradeit.sh . <your-outdir>

The first command will take input files in labsamples directory, and give outcomes in <your-outdir>. The second one will compare them with expected results. If there is different result, there will be file called log contains which cases you got wrong and what the difference are; otherwise, nothing generated.
