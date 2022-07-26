#include "dbtoolkit.h"

#include <stdlib.h>

#include <iostream>
#include <pqxx/pqxx>

#include "cpptoml.h"

using namespace std;
using namespace pqxx;

// GLOBALS
// Parse ~/.dbtoolkit.toml
// TODO: Can add these to header to make them accessible globally instead of
// this?
// ROADMAP: Secure password storage
const char* homedir = getenv("HOME");
auto config = cpptoml::parse_file(string(homedir) + "/.dbtoolkit.toml");
auto pg_user =
    config->get_qualified_as<string>("postgresql.user").value_or("postgres");
auto pg_pass =
    config->get_qualified_as<string>("postgresql.password").value_or("admin");
auto pg_host =
    config->get_qualified_as<string>("postgresql.host").value_or("127.0.0.1");
auto pg_port =
    config->get_qualified_as<string>("postgresql.port").value_or("5432");

DbToolkit::DbToolkit()
    : vbox(Gtk::ORIENTATION_VERTICAL, 5),
      hbox_extid(Gtk::ORIENTATION_HORIZONTAL, 5),
      hbox_fields(Gtk::ORIENTATION_HORIZONTAL, 5),
      hbox_output_header(Gtk::ORIENTATION_HORIZONTAL, 5),
      output_label("Output:"),
      btn_clearoutput("Clear"),
      btn_updatedblist("Update Database List"),
      btn_setpasswords("Set passwords to admin"),
      btn_disablecron("Disable CRONs"),
      btn_bak_db("Create _bak suffixed Database"),
      btn_drop_db("Drop Database"),
      btn_getextid("Get External ID"),
      btn_getfields("Get Fields") {
    // High level window settings
    set_title("DBToolkit");
    set_border_width(10);
    // Add the main vbox which everything will sit in
    add(vbox);
    // Add hboxes
    add(hbox_extid);
    add(hbox_fields);
    add(hbox_output_header);
    // Connect button click handlers
    btn_clearoutput.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_clearoutput_clicked));
    btn_updatedblist.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_updatedblist_clicked));
    btn_setpasswords.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_setpasswords_clicked));
    btn_disablecron.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_disablecron_clicked));
    btn_bak_db.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_bak_db_clicked));
    btn_drop_db.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_drop_db_clicked));
    btn_getextid.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_getextid_clicked));
    btn_getfields.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_getfields_clicked));

    // Configure placeholders on the entry boxes so we don't need labels
    extid_model_entry.set_placeholder_text("model.name");
    extid_id_entry.set_placeholder_text("23");
    fields_model_entry.set_placeholder_text("model.name");
    // Configure the output box
    output.set_editable(false);
    output.set_monospace(true);

    // Pack the hboxes
    hbox_extid.pack_start(extid_model_entry);
    hbox_extid.pack_start(extid_id_entry);
    hbox_extid.pack_start(btn_getextid);

    hbox_fields.pack_start(fields_model_entry);
    hbox_fields.pack_start(btn_getfields);

    hbox_output_header.pack_start(output_label);
    hbox_output_header.pack_start(btn_clearoutput);
    // Add everything to the vbox
    vbox.pack_start(db_selector);
    vbox.pack_start(btn_updatedblist);
    vbox.pack_start(btn_setpasswords);
    vbox.pack_start(btn_disablecron);
    vbox.pack_start(hbox_extid);
    vbox.pack_start(hbox_fields);
    vbox.pack_start(btn_bak_db);
    vbox.pack_start(btn_drop_db);
    vbox.pack_start(hbox_output_header);
    // TODO: Set min value to trigger scroll? Instead of expanding window
    vbox.pack_start(output);
    // Show everything
    show_all_children();
    // Populate the database list on init
    this->on_btn_updatedblist_clicked();
}

DbToolkit::~DbToolkit() {}

auto DbToolkit::execute_query(Glib::ustring db, Glib::ustring query,
                              bool transactional) {
    list<string> result_list;
    try {
        connection C("dbname = " + db + " user = " + pg_user + " password = " +
                     pg_pass + " hostaddr = " + pg_host + " port = " + pg_port);
        if (C.is_open()) {
            cout << "Opened database successfully: " << C.dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
        }
        if (transactional == true) {  // UPDATE/CREATE
            work W(C);
            W.exec(query);
            W.commit();
            cout << "Executed and committed transactional query: " << query
                 << endl;
            C.disconnect();
            // TODO: Any way to check result like UPDATE 1?
            return result_list;
        } else {  // SELECT (or database creation/drop)
            nontransaction N(C);
            result R(N.exec(query));
            cout << "Executed non-transactional query: " << query << endl;
            // Convert to array
            for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
                string list_entry;
                for (auto i : c) {
                    // TODO: Fix spacing for longer / shorter lines, gets offset
                    // Also keep in mind we don't want trailing space added
                    list_entry += i.as<string>();
                }
                result_list.push_back(list_entry);
            }
            C.disconnect();
            return result_list;
        }
    } catch (const sql_error& e) {
        cerr << "SQL error: " << e.what() << endl;
        return result_list;
    } catch (const exception& e) {
        cerr << e.what() << endl;
        // Satisfy non-void function
        return result_list;
    }
}

void DbToolkit::clear_output() { output.get_buffer()->set_text(""); }

void DbToolkit::on_btn_clearoutput_clicked() { this->clear_output(); }
void DbToolkit::on_btn_updatedblist_clicked() {
    // Use postgres database for this step, as we can rely on it existing.
    list<string> result = this->execute_query(
        "postgres", "SELECT datname FROM pg_database ORDER BY datname;", false);
    // Clear and re-populate db_selector
    db_selector.remove_all();
    for (auto i : result) {
        db_selector.append(i);
    }
}
void DbToolkit::on_btn_setpasswords_clicked() {
    this->execute_query(db_selector.get_active_text(),
                        "UPDATE res_users SET password='admin'", true);
}
void DbToolkit::on_btn_disablecron_clicked() {
    this->execute_query(db_selector.get_active_text(),
                        "UPDATE ir_cron SET active=false WHERE active=true",
                        true);
}
void DbToolkit::on_btn_getextid_clicked() {
    string db = db_selector.get_active_text();
    string model = extid_model_entry.get_text();
    string id = extid_id_entry.get_text();
    list<string> result = this->execute_query(
        db,
        "SELECT CONCAT(module, '.', name) AS external_id FROM "
        "ir_model_data WHERE res_id=" +
            id + " AND model='" + model + "'",
        false);
    this->clear_output();
    auto refBuffer = output.get_buffer();
    auto iter = refBuffer->get_iter_at_offset(0);
    for (auto i : result) {
        iter = refBuffer->insert(iter, i + "\r\n");
    }
}
void DbToolkit::on_btn_getfields_clicked() {
    string db = db_selector.get_active_text();
    string model = fields_model_entry.get_text();
    // ROADMAP: Make this show more useful info
    list<string> result = this->execute_query(
        db,
        "SELECT name, ttype, field_description FROM ir_model_fields WHERE "
        "model_id IN (SELECT id FROM ir_model WHERE model='" +
            model + "')",
        false);
    this->clear_output();
    auto refBuffer = output.get_buffer();
    auto iter = refBuffer->get_iter_at_offset(0);
    for (auto i : result) {
        iter = refBuffer->insert(iter, i + "\r\n");
    }
}
void DbToolkit::on_btn_bak_db_clicked() {
    // ROADMAP: Kick all sessions first
    // Use postgres database, put db_selector db in query
    string db = db_selector.get_active_text();
    int active = db_selector.get_active_row_number();
    this->execute_query("postgres",
                        "CREATE DATABASE " + db + "_bak WITH TEMPLATE " + db,
                        false);
    this->on_btn_updatedblist_clicked();
    // Set the db selector back to the db we initially had
    // NB: Potentially could lead to mis-setting due to the list changing,
    // but it seems to work fine as the _bak suffixed always ends up later
    // in the list
    db_selector.set_active(active);
}
void DbToolkit::on_btn_drop_db_clicked() {
    // ROADMAP: Kick all sessions first
    // Use postgres database, put db_selector db in query
    string db = db_selector.get_active_text();
    this->execute_query("postgres", "DROP DATABASE " + db, false);
    this->on_btn_updatedblist_clicked();
}
