# Part Management Client (pmcli)
This tool is intendet to manage a LEGO collection from the terminal. I have different boxes and
drawers and need this tool to keep track of what parts are located where.

## Installation
Download the repository and run ``make deb``. Then install the created ``.deb``-file using ``apt``.

## Usage
Start the program by typing `pmcli`. Upon first usage you are prompted to enter the database
credentials. They are saved in `~/.pmcli/login.txt`. Since the file is *unencrypted* it is *not* advised
to save the password.

After you etered the part you are searching for, you can navigate with the arrow-keys (or 'hjkl'), select the
part with enter or start a new search by pressing escape. You can stop the program by
pressing Ctrl-C and you can stop entering a new location by pressing ESC.

There are several command line options available:
- ``--info`` or ``-i`` gives you information about the **container usage**.
- ``--update`` or ``-u`` lets you **update** the underlying **part database**. Download the parts, the colors or the part and color codes as a tab-delimited file from here: https://www.bricklink.com/catalogDownload.asp After that supply the files one by one to pmcli with the ``-u`` flag: ``pmcli -u colors.txt``.
- ``--annotate`` or ``-a`` **annotates** a Bricklink XML-partlist. It adds the storage location and if there are parts in the location (filled/empty).
- ``--extract`` or ``-e`` takes all parts in the provided XML-file **out of the storage**. If you have built a model you can then bulk-extract all used parts, so that the database remains up to date.
- ``--return`` or ``-r`` puts the parts of an XML-file **back into storage**. It therefore is the opposite of ``--extract``.
- ``--missing`` or ``-m`` creates a new part list from an XML-file that **only contains the missing parts** from the part list.
- ``-s`` sets a **storage prefix** when entering part locations. This is handy when inserting a lot of part locations.

## Dependencies
This project requires:
- pugixml
- pqxx

### libpqxx - C++ and PostgreSQL
This program uses libpqxx version 7.10 which comes preinstalled on Debian 13.
https://pqxx.org/libpqxx/
https://libpqxx.readthedocs.io/stable/

### pqxx
Follow the install instructions here: https://pugixml.org/docs/manual.html#install
Download the ``.tar.gz`` folder and extract it to ``libs/``.

## Psql Docker Container
If you plan to use a psql docker container on a synology NAS running DSM 7.1
set the following environment variables:
- POSTGRES_PASSWORD
- POSTGRES_USER
- POSTGRES_DB

To keep your data in the docker container do the following:
1. Click "Add Folder" and create/select a folder like `/docker/postgres/data`.
2. Mount it to `/var/lib/postgresql/data`.

