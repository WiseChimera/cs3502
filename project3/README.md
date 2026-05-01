# Project 3 File System Implementation
## File Manager
## Build instructions - compile
''gcc main.c gui.c FileOperations.c -o filemanager `pkg-config --cflags --libs gtk+-3.0`''
<br />
'./filemanager'
## Testing (Error Handling)
- Testing files are included in TestDirectory
- './lockholder' can be used to hold the lock for smallTestFile.txt for concurrent access testing
