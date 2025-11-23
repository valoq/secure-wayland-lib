#include <gtk/gtk.h>
#include "wayland_security_context.h"

static void on_close_button_clicked(GtkButton *button, gpointer user_data)
{
    GApplication *app = G_APPLICATION(user_data);
    g_application_quit(app); // Quit the application
}

static void activate(GtkApplication *app, gpointer user_data)
{
    HelperInfo *helper_info = (HelperInfo *)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GTK4 Hello World Secure Wayland Context");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    GtkWidget *label = gtk_label_new("Hello, Secure Wayland World!");
    gtk_box_append(GTK_BOX(vbox), label);

    GtkWidget *close_button = gtk_button_new_with_label("Close");
    gtk_box_append(GTK_BOX(vbox), close_button);

    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_button_clicked), app);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[])
{
    HelperInfo helper_info;

    if (setup_wayland_security_context_helper(&helper_info) != 0) {
        fprintf(stderr, "Failed to setup Wayland security context helper\n");
        return 1;
    }

    GtkApplication *app = gtk_application_new("com.example.SecureWaylandDemo", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &helper_info);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    // Cleanup helper and remove socket file
    if (cleanup_wayland_security_context_helper(&helper_info) != 0) {
        fprintf(stderr, "Failed to cleanup Wayland security context helper\n");
    }

    return status;
}