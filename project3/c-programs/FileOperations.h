#ifndef FILEOPERATIONS_H
#define FILEOPERATIONS_H

int create_file(const char *path);
int create_directory(const char *path);
char *read_file(const char *path);
int update_file(const char *path, const char *content);
int delete_file(const char *path);
int delete_directory(const char *path);
int rename_item(const char *old_path, const char *new_path);
int navigate_directory(const char *path);

#endif
