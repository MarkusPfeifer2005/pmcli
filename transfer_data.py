import psycopg
import csv
import os
from tqdm import tqdm

def get_db_login(conf_path: str = "login.txt") -> str:
    with open(conf_path, 'r') as config_file:
        conn_str = config_file.readline()
    return conn_str

def insert_bricklink_data(txt_path: str = "Parts.txt"):
    print("Inserting BrickLink data...")
    # get catalog here: https://www.bricklink.com/catalogDownload.asp
    # Catalog Items: Parts
    # or
    # 
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
                curs.execute("INSERT INTO part VALUES(%s, %s) ON CONFLICT DO NOTHING;",
                            (row[number_idx], row[name_idx]))
                if row[alt_idx] != '':
                    alternate_numbers = set(row[alt_idx].split(','))
                    for num in alternate_numbers:
                        curs.execute("INSERT INTO alternate_num VALUES(%s, %s) ON CONFLICT DO NOTHING;",
                                    (row[number_idx], num))
                conn.commit()
    print("Completed.")


def remove_decorated_parts(txt_path: str = "Parts.txt"):
    with open(txt_path, 'r') as txt_file, psycopg.connect(get_db_login()) as conn:
        reader = csv.reader(txt_file, delimiter='\t')
        header = next(reader)
        number_idx = header.index("Number")
        category_name_idx = header.index("Category Name")
        with conn.cursor() as curs:
            for row in reader:
                if row == [] or "Decorated" not in row[category_name_idx]:
                    continue
                curs.execute("DELETE FROM part WHERE number = %s;", (row[number_idx], ))
                conn.commit()


def insert_color_data(path: str = "colors.txt"):
    with open(path, 'r') as file, psycopg.connect(get_db_login()) as conn:
        reader = csv.reader(file, delimiter='\t')
        header = next(reader)
        id_idx = header.index("Color ID")
        name_idx = header.index("Color Name")
        rgb_idx = header.index("RGB")
        with conn.cursor() as curs:
            for row in reader:
                if row == []:
                    continue
                curs.execute("INSERT INTO color VALUES(%s, %s, %s) ON CONFLICT DO NOTHING;",
                            (row[id_idx], row[name_idx], row[rgb_idx]))
                conn.commit()


def insert_available_colors(path: str = "codes.txt"):
    with open(path, 'r') as file, psycopg.connect(get_db_login()) as conn:
        reader = csv.reader(file, delimiter="\t")
        header = next(reader)
        part_number_idx = header.index("Item No")
        color_name_idx = header.index("Color")
        with conn.cursor() as curs:
            for row in tqdm(reader):
                curs.execute("SELECT number from part WHERE number=%s;",
                             (row[part_number_idx], ))
                if len(curs.fetchall()) == 0:
                    continue
                curs.execute("INSERT INTO available_colors VALUES(%s, (SELECT id FROM color where name=%s)) ON CONFLICT DO NOTHING;",
                             (row[part_number_idx], row[color_name_idx]))
                conn.commit()


def main():
    insert_bricklink_data(os.path.expanduser("~/Downloads/Parts.txt"))
    insert_color_data(os.path.expanduser("~/Downloads/colors.txt"))
    insert_available_colors(os.path.expanduser("~/Downloads/codes.txt"))


if __name__ == "__main__":
    main()
