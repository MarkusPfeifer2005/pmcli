# Psql Docker Container
If you plan to use a psql docker container on a synology nas running DSM 7.1
set the following environment variables:
- POSTGRES_PASSWORD
- POSTGRES_USER
- POSTGRES_DB

# Volume Settings (to Persist Data)
Click "Add Folder" and create/select a folder like `/docker/postgres/data`
Mount it to `/var/lib/postgresql/data`

# Connect to Database
create a "db_config.h" file and within create a preprocessor directive like:
`#define CONN_STR "dbname=lego user=USER password=PASSWORD host=localhost"`