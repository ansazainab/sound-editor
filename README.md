# Sound Editor
This is a command-line based sound editor written in C. It allows users to load, modify and save audio tracks in WAV format. The program provides functionality to manipulate audio data at a low level, enabling flexible editing of sound segments.

# Features

- Load WAV audio files into the editor for processing.
- Add, delete and modify segments of an audio track.
- Insert portions of one track into another, including self-insertions.
- Detect and identify potential ads or noise segments within a track.
- Save edited audio back into a WAV file format.

# Running the Program

# Prerequisites
- GCC Compiler: Required to compile the C source files.
- Make: Required to build the project using the provided Makefile.

# Compilation
- Use the provided Makefile to compile the program using `make sound_seg.o`
- Run the program from the command line using `./sound_seg`
