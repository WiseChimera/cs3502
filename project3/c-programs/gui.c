#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "FileOperations.h"

char current_file[1024] = "";

/* ---------- UI ---------- */
GtkWidget *file_list;
GtkWidget *path_label;
GtkWidget *status_label;
GtkWidget *file_view;

/* ---------- STATE ---------- */
char current_path[1024] = "TestDirectory";

typedef struct {
    char full_path[1024];
    int is_dir;
} ItemData;

ItemData *selected_item = NULL;

/* ---------- STATUS ---------- */
void set_status(const char *msg)
{
    gtk_label_set_text(GTK_LABEL(status_label), msg);
}

void set_error(const char *msg)
{
    char buf[1200];
    snprintf(buf, sizeof(buf), "ERROR: %s", msg);
    gtk_label_set_text(GTK_LABEL(status_label), buf);
}

/* ---------- PATH ---------- */
void update_path_display()
{
    char buf[1200];
    snprintf(buf, sizeof(buf), "Current Path: %s", current_path);
    gtk_label_set_text(GTK_LABEL(path_label), buf);
}

/* ---------- INPUT DIALOG ---------- */
char *show_input_dialog(const char *title)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, NULL, GTK_DIALOG_MODAL, "OK", GTK_RESPONSE_OK, 
        "Cancel", GTK_RESPONSE_CANCEL, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(dialog);
    char *result = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && strlen(text) > 0)
            result = strdup(text);
    }
    gtk_widget_destroy(dialog);
    return result;
}

/* ---------- ERROR POP UP ---------- */
void show_error_popup(const char *msg)
{
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ---------- MAPS ERROR MSG ---------- */
const char *get_error_message()
{
    switch (errno)
    {
        case EEXIST:
            return "File or directory already exists";
        case ENOENT:
            return "File or directory not found";
        case EACCES:
            return "Permission denied";
        case ENOSPC:
            return "No space left on device";
        case EBUSY:
            return "File is currently in use";
        case EIO:
            return "Network drive disconnections";
        case EROFS:
            return "Read-only file system";
        case ENOTEMPTY:
            return "Directory is not empty";
        default:
            return strerror(errno);
    }
}

/* ---------- REFRESH ---------- */
void refresh_file_list(int update_status)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(file_list));
    for (GList *i = children; i; i = i->next)
        gtk_widget_destroy(GTK_WIDGET(i->data));
    g_list_free(children);
    gtk_list_box_unselect_all(GTK_LIST_BOX(file_list));
    selected_item = NULL;

    DIR *dir = opendir(current_path);
    if (!dir)
    {
        show_error_popup(get_error_message());
        return;
    }

    struct dirent *entry;
    struct stat st;
    char full[1024];

    while ((entry = readdir(dir)))
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // builds a full path, example: TestDirectory/test1.txt
        snprintf(full, sizeof(full), "%s/%s", current_path, entry->d_name);

        if (stat(full, &st) != 0)
            continue;

        ItemData *data = malloc(sizeof(ItemData));
        if (!data) continue;
        // copy full path (string) to buffer 
        strncpy(data->full_path, full, sizeof(data->full_path) - 1);
        data->full_path[sizeof(data->full_path) - 1] = '\0';
        data->is_dir = S_ISDIR(st.st_mode);

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(entry->d_name);

        gtk_container_add(GTK_CONTAINER(row), label);
        g_object_set_data_full(G_OBJECT(row), "item", data, free);
        gtk_list_box_insert(GTK_LIST_BOX(file_list), row, -1);
    }
    closedir(dir);
    gtk_widget_show_all(file_list);
    update_path_display();
    if(update_status) 
        set_status("Loaded");
}

/* ---------- FILE OPEN ---------- */
void open_file(const char *path)
{
    char *content = read_file(path);
    if (!content)
    {
        show_error_popup(get_error_message());
        return;
    }
    strncpy(current_file, path, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(file_view));
    gtk_text_buffer_set_text(tb, content, -1);
    free(content);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(file_view), TRUE);
    set_status("File loaded");
}

/* ---------- SELECTS ---------- */
void on_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    if (!row)
    {
        selected_item = NULL;
        return;
    }

    selected_item = g_object_get_data(G_OBJECT(row), "item");
    gtk_text_view_set_editable(GTK_TEXT_VIEW(file_view), FALSE);
    set_status("Selected");
}

/* ---------- CLICKS INTO FILE/FOLDER ---------- */
void on_open()
{
    if (!selected_item)
    {
        set_error("No item selected");
        return;
    }

    if (selected_item->is_dir)
    {
        strncpy(current_path, selected_item->full_path, sizeof(current_path) - 1);
        current_path[sizeof(current_path) - 1] = '\0';
        refresh_file_list(1);
        set_status("Opened folder");
    }
    else
    {
        open_file(selected_item->full_path);
    }
}

/* ---------- BUTTON ACTIONS ---------- */

void on_create_file()
{
    char *name = show_input_dialog("Enter file name");
    if (!name) return;
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", current_path, name);
    if (create_file(path) == 0)
        set_status("File created");
    else
        show_error_popup(get_error_message());
    free(name);
    refresh_file_list(0);
}

void on_create_folder()
{
    char *name = show_input_dialog("Enter folder name");
    if (!name) return;
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", current_path, name);
    if (create_directory(path) == 0)
        set_status("Folder created");
    else
        show_error_popup(get_error_message());
    free(name);
    refresh_file_list(0);
}

void on_delete()
{
    if (!selected_item)
    {
        set_error("No item selected");
        return;
    }
    /* ---------- CONFIRMATION DIALOG ---------- */
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, 
        "Are you sure you want to delete this item?");
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response != GTK_RESPONSE_YES)
        return;

    int res = selected_item->is_dir ? delete_directory(selected_item->full_path) : delete_file(selected_item->full_path);
    if (res == 0)
        set_status("Deleted");
    else
        show_error_popup(get_error_message());
    refresh_file_list(0);
}

void on_rename()
{
    if (!selected_item)
    {
        set_error("No item selected");
        return;
    }
    char *name = show_input_dialog("Enter new name");
    if (!name) return;

    char new_path[1024];
    snprintf(new_path, sizeof(new_path), "%s/%s", current_path, name);
    if (rename_item(selected_item->full_path, new_path) == 0)
        set_status("Renamed");
    else
        show_error_popup(get_error_message());
    free(name);
    refresh_file_list(0);
}

void on_back()
{
    if (strcmp(current_path, "TestDirectory") == 0)
    {
        set_error("Already at root");
        return;
    }
    char *last_slash = strrchr(current_path, '/');
    if (!last_slash)
    {
        strcpy(current_path, "TestDirectory");
    }
    else
    {
        *last_slash = '\0';
    }
    selected_item = NULL;
    refresh_file_list(1);
    set_status("Went back");
}

void on_save()
{
    if (strlen(current_file) == 0)
    {
        set_error("No file open");
        return;
    }
    if(!selected_item) {
        set_error("No file selected");
        return;
    }
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(file_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(tb, &start);
    gtk_text_buffer_get_end_iter(tb, &end);
    char *text = gtk_text_buffer_get_text(tb, &start, &end, FALSE);
    if (update_file(current_file, text) == 0)
        set_status("File saved");
    else
        show_error_popup(get_error_message());
    g_free(text);
}

void on_move()
{
    if (!selected_item)
    {
        set_error("No item selected");
        return;
    }

    char *dest_dir = show_input_dialog("Enter destination directory path");
    if (!dest_dir)
        return;

    // get name of file/folder
    const char *name = strrchr(selected_item->full_path, '/');
    name = (name) ? name + 1 : selected_item->full_path;

    // build new path
    char new_path[1024];
    snprintf(new_path, sizeof(new_path), "%s/%s", dest_dir, name);

    // basic safety: prevent moving into same location
    if (strcmp(new_path, selected_item->full_path) == 0)
    {
        set_error("Source and destination are the same");
        free(dest_dir);
        return;
    }

    if (rename_item(selected_item->full_path, new_path) == 0)
    {
        set_status("Moved successfully");
    }
    else
    {
        show_error_popup(get_error_message());
    }

    free(dest_dir);
    refresh_file_list(0);
}

/* ---------- GUI ---------- */
static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *scroll;
    GtkWidget *btn_box;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    path_label = gtk_label_new("");
    status_label = gtk_label_new("Ready");
    gtk_box_pack_start(GTK_BOX(box), path_label, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box), status_label, FALSE, FALSE, 2);

    file_list = gtk_list_box_new();
    g_signal_connect(file_list, "row-selected", G_CALLBACK(on_row_selected), NULL);
    
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), file_list);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 5);

    file_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(file_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(file_view), GTK_WRAP_WORD_CHAR);
    GtkWidget *file_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(file_scroll), file_view);
    gtk_box_pack_start(GTK_BOX(box), file_scroll, TRUE, TRUE, 5);

    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *b1 = gtk_button_new_with_label("Create File");
    GtkWidget *b2 = gtk_button_new_with_label("Create Folder");
    GtkWidget *b3 = gtk_button_new_with_label("Delete");
    GtkWidget *b4 = gtk_button_new_with_label("Rename");
    GtkWidget *b_back = gtk_button_new_with_label("Back");
    GtkWidget *b_save = gtk_button_new_with_label("Save");
    GtkWidget *b_open = gtk_button_new_with_label("Open");
    GtkWidget *b_move = gtk_button_new_with_label("Move");

    gtk_box_pack_start(GTK_BOX(btn_box), b1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b3, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b4, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b_back, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b_save, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b_open, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b_move, FALSE, FALSE, 5);

    g_signal_connect(b1, "clicked", G_CALLBACK(on_create_file), NULL);
    g_signal_connect(b2, "clicked", G_CALLBACK(on_create_folder), NULL);
    g_signal_connect(b3, "clicked", G_CALLBACK(on_delete), NULL);
    g_signal_connect(b4, "clicked", G_CALLBACK(on_rename), NULL);
    g_signal_connect(b_back, "clicked", G_CALLBACK(on_back), NULL);
    g_signal_connect(b_save, "clicked", G_CALLBACK(on_save), NULL);
    g_signal_connect(b_open, "clicked", G_CALLBACK(on_open), NULL);
    g_signal_connect(b_move, "clicked", G_CALLBACK(on_move), NULL);

    gtk_box_pack_start(GTK_BOX(box), btn_box, FALSE, FALSE, 5);

    gtk_widget_show_all(window);
    refresh_file_list(1);
}

/* ---------- START ---------- */
int gui_start(int argc, char **argv)
{
    GtkApplication *app;
    app = gtk_application_new("com.filemanager.app", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
