create table if not exists part (
    number char(20) primary key,
    name varchar(1000) not null
);

create table if not exists color (
    id integer primary key,
    name varchar(100) not null,
    rgb char(6) not null
);

create table if not exists owning (
    number char(20) primary key references part(number)
        on delete cascade
        on update cascade,
    location char(10) not null
);

create table if not exists stock (
    number char(20) references owning(number)
        on delete cascade
        on update cascade,
    color integer references color(id)
        on delete cascade
        on update cascade,
    quantity integer not null,
    primary key (number, color)
);

create table if not exists alternate_num (
    number char(20) references part(number)
        on delete cascade
        on update cascade,
    alt_num char(20),
    primary key (number, alt_num)
);

create table if not exists available_colors (
    part_number char(20) references part(number)
        on delete cascade
        on update cascade,
    color_id integer references color(id)
        on delete cascade
        on update cascade,
    primary key (part_number, color_id)
)
