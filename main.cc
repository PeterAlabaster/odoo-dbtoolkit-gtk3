#include <gtkmm/application.h>

#include <iostream>

#include "dbtoolkit.h"

int main(int argc, char *argv[]) {
    Glib::RefPtr<Gtk::Application> app =
        Gtk::Application::create(argc, argv, "org.gtkmm.example");
    DbToolkit dbtoolkit;
    // Shows the window and returns when it is closed.
    return app->run(dbtoolkit);
}
