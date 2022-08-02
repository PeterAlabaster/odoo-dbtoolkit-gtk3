#ifndef GTKMM_DBTOOLKIT
#define GTKMM_DBTOOLKIT

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/textview.h>
#include <gtkmm/window.h>

#include <vector>

#include "cpptoml.h"

class DbToolkit : public Gtk::Window {
   public:
    DbToolkit();
    virtual ~DbToolkit();

   protected:
    // Helper functions
    auto execute_query(Glib::ustring db, Glib::ustring query,
                       bool transactional, bool single_column);
    void clear_output();
    void set_output_to(Glib::ustring value);
    // Button click handlers
    void on_btn_restoredb_clicked();
    void on_btn_clearoutput_clicked();
    void on_btn_updatedblist_clicked();
    void on_btn_setpasswords_clicked();
    void on_btn_disablecron_clicked();
    void on_btn_getextid_clicked();
    void on_btn_getfields_clicked();
    void on_btn_bak_db_clicked();
    void on_btn_drop_db_clicked();
    // Widgets
    Gtk::Box vbox, hbox_extid, hbox_fields, hbox_output_header;
    Gtk::Button btn_clearoutput, btn_updatedblist, btn_setpasswords,
        btn_disablecron, btn_bak_db, btn_drop_db, btn_restore_db, btn_getextid,
        btn_getfields;
    Gtk::Entry extid_model_entry, extid_id_entry, fields_model_entry;
    Gtk::Label output_label;
    Gtk::TextView output;
    Gtk::ComboBoxText db_selector;
    // Track current dbs in a simple vector, that we can iterate easily.
    std::vector<std::string> db_array;
    // TOML Configuration
    char* homedir;
    std::shared_ptr<cpptoml::table> config;
    std::string pg_user;
    std::string pg_pass;
    std::string pg_host;
    std::string pg_port;
};

#endif  // GTKMM_DBTOOLKIT
