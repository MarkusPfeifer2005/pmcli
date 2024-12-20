import psycopg
import csv


def get_db_login(conf_path: str = "db_config.txt"):
    with open(conf_path, 'r') as config_file:  # very simple text file with 4 lines to avoid giving away my password
        user = config_file.readline().replace('\n', '')
        pw = config_file.readline().replace('\n', '')
        dbname = config_file.readline().replace('\n', '')
        port = config_file.readline().replace('\n', '')
    return {"user": user, "password": pw, "dbname": dbname, "port": port}


def insert_bricklink_data(txt_path: str = "Parts.txt"):
    print("Inserting BrickLink data...")
    # get catalog here: https://www.bricklink.com/catalogDownload.asp
    with open(txt_path, 'r') as txt_file, psycopg.connect(**get_db_login()) as conn:
        reader = csv.reader(txt_file, delimiter='\t')
        header = next(reader)
        number_idx = header.index("Number")
        name_idx = header.index("Name")
        alt_idx = header.index("Alternate Item Number")
        with conn.cursor() as curs:
            for row in reader:
                if row == []:
                    continue
                curs.execute("insert into part values(%s, %s);", (row[number_idx], row[name_idx]))
                if row[alt_idx] != '':
                    for num in row[alt_idx].split(','):
                        curs.execute("insert into alternate_num values(%s, %s);", (row[number_idx], num))
                conn.commit()
    print("Completed.")


def insert_legacy_data(csv_path: str = "part_to_location.csv"):
    print("Inserting legacy data...")
    with open(csv_path, 'r') as csv_file, psycopg.connect(**get_db_login()) as conn:
        csv_reader = csv.reader(csv_file, delimiter=',')
        with conn.cursor() as curs:
            for row in csv_reader:
                if row == []:
                    continue
                curs.execute("insert into owning values(%s, %s);", row)
                conn.commit()
    print("Completed.")


def main():
    insert_bricklink_data()
    insert_legacy_data()


if __name__ == "__main__":
    main()
