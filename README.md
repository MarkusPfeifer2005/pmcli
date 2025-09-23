# libpqxx - C++ and PostgreSQL
This program uses libpqxx version 7.10 which comes preinstalled on Debian 13.
https://pqxx.org/libpqxx/
https://libpqxx.readthedocs.io/stable/

# Psql Docker Container
If you plan to use a psql docker container on a synology nas running DSM 7.1
set the following environment variables:
- POSTGRES_PASSWORD
- POSTGRES_USER
- POSTGRES_DB

# Volume Settings (to Persist Data)
Click "Add Folder" and create/select a folder like `/docker/postgres/data`
Mount it to `/var/lib/postgresql/data`

