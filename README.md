

# Dependencies
This project requires:
- pugixml
- pqxx

## libpqxx - C++ and PostgreSQL
This program uses libpqxx version 7.10 which comes preinstalled on Debian 13.
https://pqxx.org/libpqxx/
https://libpqxx.readthedocs.io/stable/

## pqxx
Follow the install instructions here: https://pugixml.org/docs/manual.html#install
Download the ``.tar.gz`` folder and extract it to ``libs/``.


# Psql Docker Container
If you plan to use a psql docker container on a synology nas running DSM 7.1
set the following environment variables:
- POSTGRES_PASSWORD
- POSTGRES_USER
- POSTGRES_DB

# Volume Settings (to Persist Data)
Click "Add Folder" and create/select a folder like `/docker/postgres/data`
Mount it to `/var/lib/postgresql/data`

# Usage
Upon first usage you are prompted to enter the database credentials. They are saved in
`~/.pmcli/login.txt`. Since the file is *unencrypted* it is *not* advised to save the password.
