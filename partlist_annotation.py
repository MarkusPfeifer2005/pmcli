import psycopg
from xml.dom.minidom import parse, Document
import sys
from os.path import expanduser


def get_db_login(conf_path: str = "login.txt") -> str:
    with open(conf_path, 'r') as config_file:
        conn_str = config_file.readline()
    return conn_str


class Part:
    def __init__(self, number: str, color: str, quantity: int):
        self.number = number
        self.color = color
        if not isinstance(quantity, int):
            raise ValueError("quantity must be int")
        self.quantity = quantity
        self.location = ""
    
    def __str__(self):
        return f"Part [{self.number}, {self.color}, {self.quantity}, {self.location}]"


def readXML(path: str) -> list[Part]:
    with open(path, 'r') as file:
        document = parse(file)
        parts = []
        items = document.getElementsByTagName("ITEM")
        for item in items:
            parts.append(Part(
                item.getElementsByTagName("ITEMID")[0].firstChild.nodeValue,
                item.getElementsByTagName("COLOR")[0].firstChild.nodeValue,
                int(item.getElementsByTagName("MINQTY")[0].firstChild.nodeValue)
            ))
    return parts


def writeXML(parts: list[Part], output_path: str):
    # Create a new XML document
    doc = Document()

    # Create the root element <INVENTORY>
    inventory = doc.createElement("INVENTORY")
    doc.appendChild(inventory)

    # Loop through the parts and create <ITEM> elements
    for part in parts:
        item = doc.createElement("ITEM")
        itemtype = doc.createElement("ITEMTYPE")
        itemtype.appendChild(doc.createTextNode('P'))
        item.appendChild(itemtype)


        # Add <ITEMID>
        itemid = doc.createElement("ITEMID")
        itemid.appendChild(doc.createTextNode(part.number))
        item.appendChild(itemid)

        # Add <COLOR>
        color = doc.createElement("COLOR")
        color.appendChild(doc.createTextNode(part.color))
        item.appendChild(color)

        # Add <MINQTY>
        minqty = doc.createElement("MINQTY")
        minqty.appendChild(doc.createTextNode(str(part.quantity)))
        item.appendChild(minqty)

        # Add <REMARKS> (location)
        if hasattr(part, "location") and part.location:
            remarks = doc.createElement("REMARKS")
            remarks.appendChild(doc.createTextNode(part.location))
            item.appendChild(remarks)

        # Append <ITEM> to <INVENTORY>
        inventory.appendChild(item)

    # Write the XML content to a file
    with open(output_path, 'w') as file:
        # Write the XML content without the XML declaration
        file.write(doc.toprettyxml(indent="  ").replace('<?xml version="1.0" ?>\n', ""))


def main():
    all_parts = []
    for arg in sys.argv[1:]:
        if arg.split('.')[-1] == "xml":
            all_parts += readXML(expanduser(arg))

    parts = []
    while len(all_parts):
        part = all_parts.pop()
        for p in parts:
            if p.number == part.number and p.color == part.color:
                p.quantity += part.quantity
                break
        else:
            parts.append(part)
    del all_parts
    
    with psycopg.connect(get_db_login()) as conn:
        with conn.cursor() as curs:
            curs.execute("SELECT * from owning;")
            for row in curs:
                for part in parts:
                    if part.number == row[0].replace(' ', ''):
                        part.location = row[1].replace(' ', '')
    
    writeXML(parts, f"{sys.argv[1].split('.')[0]}_annotated.xml")
        

if __name__ == "__main__":
    main()
