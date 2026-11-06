# MP3 Tag Reader and Editor

A command-line application developed in C to read, display, and edit metadata stored in MP3 files using the ID3v2 tagging format.

## 1. Project Overview

MP3 files contain audio data along with metadata such as song title, artist, album, year, genre, and comments.

This project implements an MP3 Tag Reader and Editor that allows users to:

- View metadata stored in an MP3 file.
- Edit selected metadata fields through command-line arguments.
- Preserve the original audio data while modifying metadata.
- Work with commonly used ID3v2.3 and ID3v2.4 tags.

The project focuses on binary file handling, structures, pointers, dynamic memory allocation, command-line argument processing, and file manipulation in C.

## 2. Features

### View Mode

Displays the metadata available in an MP3 file.

Supported fields:

- Title
- Artist
- Album
- Year
- Genre
- Comment

### Edit Mode

Allows users to modify existing metadata fields.

| Option | Metadata field | ID3 Frame ID |
|---|---|---|
| `-t` | Title | TIT2 |
| `-a` | Artist | TPE1 |
| `-A` | Album | TALB |
| `-y` | Year | TYER / TDRC |
| `-m` | Genre | TCON |
| `-c` | Comment | COMM |

The editor supports existing frames. It does not add a new frame if the requested metadata field is absent.

## 3. Technologies and Concepts Used

- **Programming Language:** C
- **File Handling:** Binary file operations using `fopen()`, `fread()`, `fwrite()`, `fseek()`, `ftell()`, and `fclose()`.
- **Structures:** Used to maintain MP3 file information and file pointers.
- **Pointers:** Used for passing structures, handling buffers, and accessing dynamically allocated memory.
- **Dynamic Memory Allocation:** Used `malloc()` and `free()` for variable-sized frame data.
- **Command-Line Arguments:** Used `argc` and `argv` to process user commands.
- **String Handling:** Used functions such as `strcmp()`, `strlen()`, and `memcpy()`.
- **Bitwise Operations:** Used for decoding and encoding synchsafe and big-endian integers.
- **File Manipulation:** Used `rename()` and `remove()` to replace the original file after editing.

## 4. Project File Structure

```text
MP3_Tag_Reader/
│
├── mp3_header.h
├── mp3_main.c
├── functions.c
├── mp3_view.c
└── mp3_edit.c
```

### File Responsibilities

| File | Responsibility |
|---|---|
| `mp3_header.h` | Contains header inclusions, macros, structure definitions, and function declarations. |
| `mp3_main.c` | Entry point; validates command-line arguments and dispatches view/edit operations. |
| `functions.c` | Handles command-line argument validation and operation selection. |
| `mp3_view.c` | Reads the ID3 header and frames, decodes metadata, and displays tag information. |
| `mp3_edit.c` | Replaces existing metadata frames and writes the updated MP3 file. |

## 5. Understanding ID3 Metadata

ID3 is a metadata format used to store information inside MP3 files.

This project works with ID3v2.3 and ID3v2.4.

### ID3 Header

An ID3v2 header is 10 bytes long.

| Field | Size |
|---|---:|
| Identifier (`ID3`) | 3 bytes |
| Version | 1 byte |
| Revision | 1 byte |
| Flags | 1 byte |
| Tag Size | 4 bytes |

The first three bytes identify the presence of an ID3 tag.

The tag size represents the size of the tag data following the 10-byte header. It does not include the header itself.

### ID3 Frames

Metadata is stored in individual frames inside the ID3 tag.

Each frame contains:

| Field | Size |
|---|---:|
| Frame ID | 4 bytes |
| Frame Size | 4 bytes |
| Frame Flags | 2 bytes |
| Frame Data | Variable |

Examples:

- `TIT2` – Title
- `TPE1` – Artist
- `TALB` – Album
- `TYER` – Year in ID3v2.3
- `TDRC` – Recording time in ID3v2.4
- `TCON` – Genre
- `COMM` – Comment

### Difference Between ID3v2.3 and ID3v2.4

One important difference is how frame sizes are stored.

**ID3v2.3: Big-endian format**

The most significant byte is stored first.

**ID3v2.4: Synchsafe integer format**

Only seven bits of each byte are used to represent the value. The most significant bit is always zero.

Synchsafe integers help prevent metadata bytes from being mistaken for MPEG audio synchronization patterns.

The project implements separate decoding and encoding functions to handle these differences.

## 6. Working Principle

### View Operation

```text
              User Command
                   |
                   v
          Validate Arguments
                   |
                   v
           Open MP3 File
                   |
                   v
           Read ID3 Header
                   |
                   v
        Validate ID3 Version
                   |
                   v
           Read Tag Size
                   |
                   v
          Read Frame Header
                   |
                   v
          Decode Frame Size
                   |
                   v
           Read Frame Data
                   |
                   v
        Identify Metadata Frame
                   |
                   v
           Display Tag Data
                   |
                   v
             Close File
```

### Edit Operation

```text
              User Command
                   |
                   v
          Validate Arguments
                   |
                   v
           Open Original MP3
                   |
                   v
           Read ID3 Header
                   |
                   v
          Create Temporary File
                   |
                   v
           Read Existing Frames
                   |
                   v
        Identify Target Frame
                   |
          +--------+--------+
          |                 |
          v                 v
     Target Frame      Other Frames
          |                 |
          v                 v
    Build New Frame     Copy Original
          |                 |
          +--------+--------+
                   |
                   v
          Update Tag Size
                   |
                   v
         Copy Original Audio Data
                   |
                   v
          Close Both Files
                   |
                   v
        Replace Original File
```

## 7. Compilation

Compile all source files together using GCC:

```bash
gcc -Wall -Wextra -std=c11 mp3_main.c functions.c mp3_view.c mp3_edit.c -o mp3_reader
```

## 8. Usage Instructions

### Display Help

```bash
./mp3_reader --help
```

### View MP3 Metadata

```bash
./mp3_reader -v sample.mp3
```

### Edit Title

```bash
./mp3_reader -e -t "New Song Title" sample.mp3
```

### Edit Artist

```bash
./mp3_reader -e -a "New Artist" sample.mp3
```

### Edit Album

```bash
./mp3_reader -e -A "New Album" sample.mp3
```

### Edit Year

```bash
./mp3_reader -e -y "2026" sample.mp3
```

### Edit Genre

```bash
./mp3_reader -e -m "Rock" sample.mp3
```

### Edit Comment

```bash
./mp3_reader -e -c "My new comment" sample.mp3
```

After editing, run the view command again to verify the changes:

```bash
./mp3_reader -v sample.mp3
```

## 9. Important Implementation Decisions

### 1. Why Binary File Handling?

MP3 files contain binary audio data and structured metadata. Opening them in text mode could cause unwanted data transformations.

Therefore, binary modes such as `rb` and `wb` are used.

### 2. Why Use a Structure?

The `struct MP3` structure groups related information such as the filename, file pointers, selected metadata option, and new metadata value.

This makes it easier to pass project information between functions.

### 3. Why Use Dynamic Memory Allocation?

ID3 frame sizes vary depending on the metadata stored in them.

Using `malloc()` allows memory to be allocated according to the actual frame size rather than relying on a fixed-size buffer.

Allocated memory is released using `free()`.

### 4. Why Use a Temporary File?

Directly modifying metadata inside a binary file can be difficult because the replacement frame may have a different size from the original frame.

A temporary file provides a safer approach:

1. Copy the unchanged frames.
2. Write the modified frame.
3. Copy the original audio data.
4. Update the ID3 tag size.
5. Replace the original file after successful writing.

This avoids shifting audio data manually inside the original file.

### 5. Why Separate Functions for Big-endian and Synchsafe Integers?

ID3v2.3 and ID3v2.4 use different frame-size representations.

Separate functions make the code easier to understand and ensure the correct conversion is used for each version.

## 10. Limitations

This is an educational implementation and does not support every feature of the ID3 specification.

- Supports ID3v2.3 and ID3v2.4 common frame layouts.
- Supports editing existing metadata frames only.
- Does not implement extended headers or unsynchronisation.
- Does not handle compressed or encrypted frames.
- Does not fully support all Unicode encodings.
- Does not preserve all original file permissions and filesystem metadata.

Always test editing operations on backup copies of MP3 files.

## 11. Key Learnings

Through this project, I gained practical experience in:

- Understanding binary file structures and reading data at specific offsets.
- Working with file pointers and navigating files using `fseek()` and `ftell()`.
- Understanding the difference between big-endian and synchsafe integer representations.
- Applying bitwise operations for binary data conversion.
- Using structures and pointers to organize project data.
- Allocating memory dynamically based on data size.
- Processing command-line arguments to build a CLI application.
- Reading and interpreting structured binary metadata.
- Rewriting files using temporary files and safely handling file operations.
- Dividing a larger C application into multiple source and header files.

## 12. Conclusion

The MP3 Tag Reader and Editor project demonstrates how C can be used to inspect and modify structured binary data in real-world file formats.

Beyond basic C programming, this project helped develop a better understanding of binary file handling, memory management, bitwise operations, file-pointer navigation, and modular programming.

It provided practical experience in building a command-line application that reads, interprets, and modifies metadata without intentionally changing the underlying audio content.
