# dbtoolkit (c++)
GTK3 UI, helper for common odoo db commands written in C++

Uses `gtkmm` for gtk api, `pqxx` to interface with postgresql, `cpptoml` to parse configuration file.

First C++ endeavour.
## Dependencies
### Compiling on debian base
```bash
$ sudo apt install -y libpqxx-dev libgtkmm-3.0-dev libcpptoml-dev
$ ./compile.sh
```
### Running on debian base
```bash
$ sudo apt install -y libpqxx-6.4
$ ./dbtoolkitgtk
```
## Configuration
`~/.dbtoolkit.toml` required, example contents.

All values must be quoted, fallbacks commented.

If fallback suffice, the key can be emitted:
```
[postgresql]
user = "odoo"  # "postgres"
password = "voodoo"  # "admin"
host = "192.168.0.66"  # "127.0.0.1"
port = "5436"  # "5432"
```
## Features
- Database lister with update button, shows all databases on the configured postgres instance
- Admin password setter, sets all `res.users` passwords on the currently selected database to `admin`
- Disable CRON button - sets all active `ir.cron` records on the currently selected database to `False`
- External ID getter - give a model `_name` and a database `ID` on the currently selected database and the _external_ ID will be displayed
- Field getter - give a model `_name` and get all columns available on the model
- Database template backup - create a `_bak` suffixed database using the currently selected database as a template
- Database dropper - drop a database

## Roadmap
- Some gtk tidyup, output is scuffed when we have multiple columns, and has no scrollbar.
- Some code tidyup, get rid of globals
- Secure password storage in `~/.dbtoolkit.toml`?
- Db drop/backup buttons should try to kick all active postgres sessions on the db
- Add db `_bak` restorer, which drops the non `_bak` suffixed database, then creates it again using the `_bak` suffixed database
- Extend field getter to show more useful information
- Add compiled binary to releases

## References
- https://docs.huihoo.com/gtkmm/programming-with-gtkmm-3/3.4.1/en
- http://pqxx.org/development/libpqxx/#quick-example
- https://github.com/skystrife/cpptoml#obtaining-basic-values
