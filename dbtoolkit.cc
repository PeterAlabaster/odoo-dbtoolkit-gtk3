#include "dbtoolkit.h"

#include <stdlib.h>

#include <iostream>
#include <pqxx/pqxx>

#include "cpptoml.h"

using namespace std;
using namespace pqxx;

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
      btn_restore_db("Drop, then restore _bak db"),
      btn_getextid("Get External ID"),
      btn_getfields("Get Fields") {
    // High level window settings
    set_title("DBToolkit");
    set_border_width(10);
    scrollwindow_output.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    // ROADMAP: Expand & contract min content height as content fills / unfills
    scrollwindow_output.set_min_content_height(180);
    // TOML Configuration parse (~/.dbtoolkit.toml)
    // ROADMAP: Secure password storage
    homedir = getenv("HOME");
    config = cpptoml::parse_file(string(homedir) + "/.dbtoolkit.toml");
    pg_user = config->get_qualified_as<string>("postgresql.user")
                  .value_or("postgres");
    pg_pass = config->get_qualified_as<string>("postgresql.password")
                  .value_or("admin");
    pg_host = config->get_qualified_as<string>("postgresql.host")
                  .value_or("127.0.0.1");
    pg_port =
        config->get_qualified_as<string>("postgresql.port").value_or("5432");
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
    btn_restore_db.signal_clicked().connect(
        sigc::mem_fun(*this, &DbToolkit::on_btn_restoredb_clicked));
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
    vbox.pack_start(btn_restore_db);
    vbox.pack_start(hbox_output_header);
    vbox.pack_start(scrollwindow_output);
    scrollwindow_output.add(output);
    // vbox.pack_start(output);
    // Show everything
    show_all_children();
    // Populate the database list on init
    this->on_btn_updatedblist_clicked();
}

DbToolkit::~DbToolkit() {}

auto DbToolkit::execute_query(Glib::ustring db, Glib::ustring query,
                              bool transactional, bool single_column) {
    string result_str;
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
            result_str = "Succesfully ran " + query;
            return result_str;
        } else {  // SELECT (or database creation/drop)
            nontransaction N(C);
            result R(N.exec(query));
            cout << "Executed non-transactional query: " << query << endl;
            if (single_column) {  // For db list, or extid
                for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
                    string line_entry;
                    for (auto i : c) {
                        line_entry += i.as<string>();
                    }
                    result_str += line_entry + "\r\n";
                }
            } else {  // ROADMAP Pad stuff so it is easy to read in the output
                for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
                    string line_entry;
                    for (auto i : c) {
                        line_entry += i.as<string>();
                    }
                    result_str += line_entry + "\r\n";
                }
            }
            C.disconnect();
            return result_str;
        }
    } catch (const sql_error& e) {
        cerr << "SQL error: " << e.what() << endl;
        result_str = e.what();
        return result_str;
    } catch (const exception& e) {
        cerr << e.what() << endl;
        result_str = e.what();
        return result_str;
    }
}

void DbToolkit::kick_sessions(Glib::ustring db) {
    this->execute_query(
        "postgres",
        "SELECT pg_terminate_backend(pg_stat_activity.pid) FROM "
        "pg_stat_activity WHERE pg_stat_activity.datname = '" +
            db + "' AND pid <> pg_backend_pid();",
        false, false);
}

void DbToolkit::clear_output() { output.get_buffer()->set_text(""); }

void DbToolkit::set_output_to(Glib::ustring value) {
    this->clear_output();
    auto refBuffer = output.get_buffer();
    auto iter = refBuffer->get_iter_at_offset(0);
    refBuffer->insert(iter, value);
}

void DbToolkit::on_btn_clearoutput_clicked() { this->clear_output(); }
void DbToolkit::on_btn_updatedblist_clicked() {
    // Use postgres database for this step, as we can rely on it existing.
    string result = this->execute_query(
        "postgres", "SELECT datname FROM pg_database ORDER BY datname;", false,
        true);
    // Clear and re-populate db_selector, and db_array
    db_selector.remove_all();
    db_array.clear();
    istringstream iss(result);
    for (string line; getline(iss, line);) {
        // Remove newlines.
        line.erase(line.length() - 1);
        db_array.push_back(line);
        db_selector.append(line);
    }
}
void DbToolkit::on_btn_setpasswords_clicked() {
    string result = this->execute_query(db_selector.get_active_text(),
                                        "UPDATE res_users SET password='admin'",
                                        true, false);
    this->set_output_to(result);
}
void DbToolkit::on_btn_disablecron_clicked() {
    string result = this->execute_query(
        db_selector.get_active_text(),
        "UPDATE ir_cron SET active=false WHERE active=true", true, false);
    this->set_output_to(result);
}
void DbToolkit::on_btn_getextid_clicked() {
    string db = db_selector.get_active_text();
    string model = extid_model_entry.get_text();
    string id = extid_id_entry.get_text();
    string result = this->execute_query(
        db,
        "SELECT CONCAT(module, '.', name) AS external_id FROM "
        "ir_model_data WHERE res_id=" +
            id + " AND model='" + model + "'",
        false, false);
    this->set_output_to(result);
}
void DbToolkit::on_btn_getfields_clicked() {
    string db = db_selector.get_active_text();
    string model = fields_model_entry.get_text();
    // ROADMAP: Make this show more useful info
    string result = this->execute_query(
        db,
        "SELECT name, ttype, field_description FROM ir_model_fields WHERE "
        "model_id IN (SELECT id FROM ir_model WHERE model='" +
            model + "')",
        false, false);
    this->set_output_to(result);
}
void DbToolkit::on_btn_bak_db_clicked() {
    // Use postgres database, put db_selector db in query
    string db = db_selector.get_active_text();
    this->kick_sessions(db);
    int active = db_selector.get_active_row_number();
    string result = this->execute_query(
        "postgres", "CREATE DATABASE " + db + "_bak WITH TEMPLATE " + db, false,
        false);
    this->on_btn_updatedblist_clicked();
    this->set_output_to(result);
    // Set the db selector back to the db we initially had
    // NB: Potentially could lead to mis-setting due to the list changing,
    // but it seems to work fine as the _bak suffixed always ends up later
    // in the list
    db_selector.set_active(active);
}
void DbToolkit::on_btn_drop_db_clicked() {
    // Use postgres database, put db_selector db in query
    string db = db_selector.get_active_text();
    this->kick_sessions(db);
    string result =
        this->execute_query("postgres", "DROP DATABASE " + db, false, false);
    this->on_btn_updatedblist_clicked();
    this->set_output_to(result);
}

void DbToolkit::on_btn_restoredb_clicked() {
    string db = db_selector.get_active_text();
    string suffix = "_bak";
    int active = db_selector.get_active_row_number();
    // FIXME: Don't need both bools and strings, just need strings
    // but not sure what truthy check looks like
    string target_db_to_drop;
    string target_db_to_restore;
    bool has_bak = false;
    bool has_orig = false;
    string result;
    bool has_suffix_bak =
        db.size() >= suffix.size() &&
        db.compare(db.size() - suffix.size(), suffix.size(), suffix) == 0;
    if (has_suffix_bak == true) {
        target_db_to_restore = db;
        target_db_to_drop = db.erase(db.length() - 4);
        has_bak = true;
        for (auto i : db_array) {
            if (i == target_db_to_drop) {
                has_orig = true;
            }
        }
    } else {  // Find if we have a _bak suffixed db in db_array
        target_db_to_drop = db;
        for (auto i : db_array) {
            if (i == db + "_bak") {
                target_db_to_restore = i;
                has_orig = true;
                has_bak = true;
            }
        }
    }
    if (has_bak == true && has_orig == true) {
        // Execute the drop &  restore.
        this->set_output_to("Dropping " + target_db_to_drop +
                            "\r\n& Restoring " + target_db_to_restore + "...");
        this->kick_sessions(target_db_to_drop);
        this->execute_query("postgres", "DROP DATABASE " + target_db_to_drop,
                            false, false);
        this->execute_query("postgres",
                            "CREATE DATABASE " + target_db_to_drop +
                                " WITH TEMPLATE " + target_db_to_restore,
                            false, false);
        this->on_btn_updatedblist_clicked();
        this->set_output_to("Finished dropping " + target_db_to_drop +
                            " and restoring " + target_db_to_restore);
        db_selector.set_active(active);
    } else {
        this->set_output_to(
            "Could not determine both a _bak suffixed database, and a non-_bak "
            "\r\n"
            "suffixed database. Doing nothing");
    }
}
