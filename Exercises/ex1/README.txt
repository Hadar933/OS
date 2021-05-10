shulik10, hadar933
Shalom Kachko (313446320), Hadar Sharvit (208287599)
EX: 1

FILES:
osm.cpp
graph.png
README
Makefile

Assignment 1:

the program WhatIDo performs as followed:
given 0, 2 or more arguments:
  -duplicates a file with file descriptor 2 (using dup(2)) and checks it's flags
   and access modes. the return value is Read-Write mode.
  -this process is followed with a write of the following error messsage:
  "Error. the program should receive a single argument. Exiting"
  -this message is followed with a "success" message
  -the program exits with exit code 0.


given exactly 1 argument:
  -a "Welcome" directory is created
  -a "To" directory is created inside directory "Welcome" (Welcome/To)
  -a file "OS2021" is created inside directory "To" (Welcome/To/OS2021)
  -writing: "username
            if you havent read the course guidelines yet --- do it right now!
            prints the input"
  for example- given the username shulik10 and the input 4:
  "shulik10
  if you havent read the course guidelines yet --- do it right now!
  4"
  -deletes OS2021
  -deletes directory "To"
  -deleted directory "Welcome"
  -exits with exit code 0
