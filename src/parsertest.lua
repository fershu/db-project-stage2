local pretty = require('pl.pretty')
dofile('src/parser.lua')

local input = [[
CREATE TABLE students (sid int primary key, name varchar(20), major varchar(4));
INSERT INTO students (sid, major, name) values (101062116, "CS", "Jack Force");
INSERT INTO students values (101062124, "hydai", "CS");
SELECT COUNT(*) from students;
SELECT COUNT(students) from students;
SELECT COUNT(students.sid) from students;
]]


local results = parseCommand(input)
pretty.dump(results)

results = parseCommand(nil, "input.txt")
pretty.dump(results)