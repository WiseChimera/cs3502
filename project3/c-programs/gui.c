#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include "FileOperations.h"

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
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, NULL, GTK_DIALOG_MODAL,"OK", GTK_RESPONSE_OK,"Cancel", GTK_RESPONSE_CANCEL,NULL);

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

/* ---------- FILE OPEN ---------- */
void open_file(const char *path)
{
    char *content = read_file(path);

    if (!content)
    {
        set_error("Cannot read file");
        return;
    }

    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(file_view));

    gtk_text_buffer_set_text(tb, content, -1);

    free(content);

    set_status("File loaded");
}

/* ---------- CLICK ---------- */
void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    selected_item = g_object_get_data(G_OBJECT(row), "item");

    if (!selected_item) return;

    if (selected_item->is_dir)
    {
        strcpy(current_path, selected_item->full_path);
        refresh_file_list();
        set_status("Entered folder");
    }
    else
    {
        open_file(selected_item->full_path);
    }
}

/* ---------- REFRESH ---------- */
void refresh_file_list()
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(file_list));
    for (GList *i = children; i; i = i->next)
        gtk_widget_destroy(GTK_WIDGET(i->data));
    g_list_free(children);

    DIR *dir = opendir(current_path);
    if (!dir)
    {
        set_error("Cannot open directory");
        return;
    }

    struct dirent *entry;
    struct stat st;
    char full[1024];

    while ((entry = readdir(dir)))
    {
        if (!strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, ".."))
            continue;

        snprintf(full, sizeof(full), "%s/%s", current_path, entry->d_name);

        if (stat(full, &st) != 0)
            continue;

        ItemData *data = malloc(sizeof(ItemData));
        strcpy(data->full_path, full);
        data->is_dir = S_ISDIR(st.st_mode);

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(entry->d_name);

        gtk_container_add(GTK_CONTAINER(row), label);

        g_object_set_data(G_OBJECT(row), "item", data);

        gtk_list_box_insert(GTK_LIST_BOX(file_list), row, -1);
    }

    closedir(dir);

    gtk_widget_show_all(file_list);

    update_path_display();
    set_status("Loaded");
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
        set_error("Create file failed");

    free(name);
    refresh_file_list();
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
        set_error("Create folder failed");

    free(name);
    refresh_file_list();
}

void on_delete()
{
    if (!selected_item)
    {
        set_error("No item selected");
        return;
    }

    int res = selected_item->is_dir ? delete_directory(selected_item->full_path) : delete_file(selected_item->full_path);

    if (res == 0)
        set_status("Deleted");
    else
        set_error("Delete failed");

    refresh_file_list();
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
        set_error("Rename failed");

    free(name);
    refresh_file_list();
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
        *last_slash = '\0';  // cut off last folder
    }

    selected_item = NULL;
    refresh_file_list();
    set_status("Went back");
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

    g_signal_connect(file_list, "row-activated", G_CALLBACK(on_row_activated), NULL);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), file_list);

    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 5);

    file_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(file_view), FALSE);

    gtk_box_pack_start(GTK_BOX(box), file_view, TRUE, TRUE, 5);

    /* BUTTONS */
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *b1 = gtk_button_new_with_label("Create File");
    GtkWidget *b2 = gtk_button_new_with_label("Create Folder");
    GtkWidget *b3 = gtk_button_new_with_label("Delete");
    GtkWidget *b4 = gtk_button_new_with_label("Rename");
    GtkWidget *b_back = gtk_button_new_with_label("Back");


    gtk_box_pack_start(GTK_BOX(btn_box), b1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b3, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b4, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(btn_box), b_back, FALSE, FALSE, 5);

    g_signal_connect(b1, "clicked", G_CALLBACK(on_create_file), NULL);
    g_signal_connect(b2, "clicked", G_CALLBACK(on_create_folder), NULL);
    g_signal_connect(b3, "clicked", G_CALLBACK(on_delete), NULL);
    g_signal_connect(b4, "clicked", G_CALLBACK(on_rename), NULL);
    g_signal_connect(b_back, "clicked", G_CALLBACK(on_back), NULL);

    gtk_box_pack_start(GTK_BOX(box), btn_box, FALSE, FALSE, 5);

    gtk_widget_show_all(window);

    refresh_file_list();
}

/* ---------- START ---------- */
int gui_start(int argc, char **argv)
{
    GtkApplication *app;

    app = gtk_application_new("com.filemanager.app",
                              G_APPLICATION_FLAGS_NONE);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}
