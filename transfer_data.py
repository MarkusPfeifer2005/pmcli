import psycopg
import csv


def get_db_login(conf_path: str = "db_config.h") -> str:
    with open(conf_path, 'r') as config_file:
        conn_str = config_file.readline().split('"')[1]
    return conn_str

def insert_bricklink_data(txt_path: str = "Parts.txt"):
    print("Inserting BrickLink data...")
    # get catalog here: https://www.bricklink.com/catalogDownload.asp
    with open(txt_path, 'r') as txt_file, psycopg.connect(get_db_login()) as conn:
        reader = csv.reader(txt_file, delimiter='\t')
        header = next(reader)
        number_idx = header.index("Number")
        name_idx = header.index("Name")
        alt_idx = header.index("Alternate Item Number")
        category_name_idx = header.index("Category Name")
        with conn.cursor() as curs:
            for row in reader:
                if row == [] or "Decorated" in row[category_name_idx]:
                    continue
                curs.execute("INSERT INTO part VALUES(%s, %s);", (row[number_idx], row[name_idx]))
                if row[alt_idx] != '':
                    for num in row[alt_idx].split(','):
                        curs.execute("INSERT INTO alternate_num VALUES(%s, %s);", (row[number_idx], num))
                conn.commit()
    print("Completed.")


def remove_decorated_parts(txt_path: str = "Parts.txt"):
    with open(txt_path, 'r') as txt_file, psycopg.connect(get_db_login()) as conn:
        reader = csv.reader(txt_file, delimiter='\t')
        header = next(reader)
        number_idx = header.index("Number")
        alt_idx = header.index("Alternate Item Number")
        category_name_idx = header.index("Category Name")
        with conn.cursor() as curs:
            for row in reader:
                if row == [] or "Decorated" not in row[category_name_idx]:
                    continue
                curs.execute("DELETE FROM part WHERE number = %s;", (row[number_idx], ))
                conn.commit()


def insert_legacy_data(csv_path: str = "part_to_location.csv"):
    print("Inserting legacy data...")
    with open(csv_path, 'r') as csv_file, psycopg.connect(get_db_login()) as conn:
        csv_reader = csv.reader(csv_file, delimiter=',')
        with conn.cursor() as curs:
            for row in csv_reader:
                if row == []:
                    continue
                curs.execute("insert into owning values(%s, %s);", row)
                conn.commit()
    print("Completed.")


def main():
    #insert_bricklink_data()
    #insert_legacy_data()
    remove_decorated_parts()


if __name__ == "__main__":
    main()
