#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *combo;
    char distro[256];
} AppWidgets;

// Function to safely detect distribution (unchanged)
bool detect_distro(char *distro, size_t distro_size) {
    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp) {
        strncpy(distro, "unknown", distro_size - 1);
        distro[distro_size - 1] = '\0';
        return false;
    }

    char buffer[256];
    bool found = false;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "ID=", 3) == 0) {
            strncpy(distro, buffer + 3, distro_size - 1);
            distro[distro_size - 1] = '\0';
            char *quote = strchr(distro, '"');
            if (quote) *quote = '\0';
            distro[strcspn(distro, "\n")] = '\0';
            found = true;
            break;
        }
    }
    
    fclose(fp);
    if (!found) {
        strncpy(distro, "unknown", distro_size - 1);
        distro[distro_size - 1] = '\0';
    }
    return found;
}

// Updated PackageManager struct with update command
typedef struct {
    const char *pkg_manager;
    const char *install_cmd;
    const char *update_cmd;  // New field for update command
    bool needs_sudo;
} PackageManager;

PackageManager get_package_manager(const char *distro) {
    PackageManager pm = {NULL, NULL, NULL, false};
    
    if (strcmp(distro, "debian") == 0 || strcmp(distro, "ubuntu") == 0 || 
        strstr(distro, "debian") || strstr(distro, "ubuntu")) {
        pm.pkg_manager = "apt";
        pm.install_cmd = "install -y";
        pm.update_cmd = "update";
        pm.needs_sudo = true;
    }
    else if (strcmp(distro, "fedora") == 0 || strstr(distro, "fedora")) {
        pm.pkg_manager = "dnf";
        pm.install_cmd = "install -y";
        pm.update_cmd = "makecache";
        pm.needs_sudo = true;
    }
    else if (strcmp(distro, "arch") == 0 || strcmp(distro, "manjaro") == 0 || 
             strstr(distro, "arch")) {
        pm.pkg_manager = "pacman";
        pm.install_cmd = "-S --noconfirm";
        pm.update_cmd = "-Sy";
        pm.needs_sudo = true;
    }
    else if (strcmp(distro, "opensuse") == 0 || strstr(distro, "suse")) {
        pm.pkg_manager = "zypper";
        pm.install_cmd = "install -y";
        pm.update_cmd = "refresh";
        pm.needs_sudo = true;
    }
    else if (strcmp(distro, "alpine") == 0 || strstr(distro, "alpine")) {
        pm.pkg_manager = "apk";
        pm.install_cmd = "add";
        pm.update_cmd = "update";
        pm.needs_sudo = true;
    }
    else if (strcmp(distro, "gentoo") == 0 || strstr(distro, "gentoo")) {
        pm.pkg_manager = "emerge";
        pm.install_cmd = "-av";
        pm.update_cmd = "sync";
        pm.needs_sudo = true;
    }
    
    return pm;
}

// Updated install function with repository update
void install_de(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    
    const char *selection = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(app->combo));
    
    if (!selection || strlen(selection) == 0) {
        gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Please select a desktop environment");
        return;
    }

    PackageManager pm = get_package_manager(app->distro);
    if (!pm.pkg_manager) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Unsupported distribution: %s", app->distro);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    // Package mapping (unchanged)
    struct {
        const char *name;
        const char *debian_pkg;
        const char *fedora_pkg;
        const char *arch_pkg;
        const char *suse_pkg;
        const char *alpine_pkg;
        const char *gentoo_pkg;
    } de_packages[] = {
        {"Xfce", "xfce4", "xfce4-session", "xfce4", "xfce4-session", 
         "xfce4", "xfce4"},
        {"KDE", "plasma-desktop", "plasma-desktop", "plasma-desktop", 
         "plasma5-desktop", "plasma-workspace", "kde-plasma/plasma-workspace"},
        {"GNOME", "gnome-session", "gnome", "gnome", "gnome-shell", 
         "gnome-shell", "gnome-base/gnome"},
        {"Cinnamon", "cinnamon", "cinnamon", "cinnamon", "cinnamon", 
         "cinnamon", "gnome-extra/cinnamon"},
        {"LXDE", "lxde", "lxde", "lxde", "lxde", 
         "lxde-base", "lxde-base/lxde-common"},
        {NULL, NULL, NULL, NULL, NULL, NULL, NULL}
    };

    const char *package = NULL;
    for (int i = 0; de_packages[i].name; i++) {
        if (strcmp(selection, de_packages[i].name) == 0) {
            if (pm.pkg_manager == "apt") package = de_packages[i].debian_pkg;
            else if (pm.pkg_manager == "dnf") package = de_packages[i].fedora_pkg;
            else if (pm.pkg_manager == "pacman") package = de_packages[i].arch_pkg;
            else if (pm.pkg_manager == "zypper") package = de_packages[i].suse_pkg;
            else if (pm.pkg_manager == "apk") package = de_packages[i].alpine_pkg;
            else if (pm.pkg_manager == "emerge") package = de_packages[i].gentoo_pkg;
            break;
        }
    }

    if (!package) {
        return;
    }

    char update_command[512];
    char install_command[512];
    const char *prefix = pm.needs_sudo ? "pkexec" : "";
    
    // Construct update command
    if (pm.update_cmd) {
        if (snprintf(update_command, sizeof(update_command), "%s %s %s", 
                     prefix, pm.pkg_manager, pm.update_cmd) >= (int)sizeof(update_command)) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Update command buffer overflow error");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }

    // Construct install command
    if (snprintf(install_command, sizeof(install_command), "%s %s %s %s", 
                 prefix, pm.pkg_manager, pm.install_cmd, package) >= (int)sizeof(install_command)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Install command buffer overflow error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    int result = 0;
    // Execute update command first if available
    if (pm.update_cmd) {
        result = system(update_command);
        if (result != 0) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING,
                GTK_BUTTONS_OK,
                "Repository update failed, attempting installation anyway");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }

    // Execute install command
    result = system(install_command);
    GtkMessageType msg_type = (result == 0) ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR;
    const char *msg = (result == 0) ? 
        "Installation completed successfully" : 
        "Installation failed";

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        msg_type,
        GTK_BUTTONS_OK,
        "%s: %s", msg, selection);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Rest of the code remains unchanged
static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;

    if (!widgets) {
        g_error("Application widgets structure is NULL");
        return;
    }

    widgets->window = gtk_application_window_new(app);
    if (!widgets->window) {
        g_error("Failed to create application window");
        return;
    }

    gtk_window_set_title(GTK_WINDOW(widgets->window), "Desktop Environment Installer");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 300, 200);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(widgets->window), box);
    gtk_container_set_border_width(GTK_CONTAINER(widgets->window), 10);

    char label_text[512];
    PackageManager pm = get_package_manager(widgets->distro);
    if (snprintf(label_text, sizeof(label_text), 
         "Detected Distribution: %s (Using %s)", 
         widgets->distro, pm.pkg_manager ? pm.pkg_manager : "unknown") >= (int)sizeof(label_text)) {
        strcpy(label_text, "Detected Distribution: buffer overflow");
    }
    GtkWidget *label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    widgets->combo = gtk_combo_box_text_new();
    if (!widgets->combo) {
        g_error("Failed to create combo box");
        return;
    }

    const char *options[] = {"Xfce", "KDE", "GNOME", "Cinnamon", "LXDE", NULL};
    for (int i = 0; options[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->combo), options[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->combo), 0);
    gtk_box_pack_start(GTK_BOX(box), widgets->combo, FALSE, FALSE, 0);

    GtkWidget *button = gtk_button_new_with_label("Install");
    g_signal_connect(button, "clicked", G_CALLBACK(install_de), widgets);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    gtk_widget_show_all(widgets->window);
}

int main(int argc, char **argv) {
    AppWidgets widgets = {0};
    
    if (!detect_distro(widgets.distro, sizeof(widgets.distro))) {
        g_warning("Failed to reliably detect distribution");
    }
    
    GtkApplication *app = gtk_application_new("com.example.de_installer", 
                                            G_APPLICATION_DEFAULT_FLAGS);
    if (!app) {
        g_error("Failed to create GTK application");
        return 1;
    }

    g_signal_connect(app, "activate", G_CALLBACK(activate), &widgets);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
