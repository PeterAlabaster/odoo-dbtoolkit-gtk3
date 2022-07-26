#!/bin/bash
g++ main.cc dbtoolkit.cc -o dbtoolkitgtk `pkg-config --cflags --libs gtkmm-3.0 libpqxx`
