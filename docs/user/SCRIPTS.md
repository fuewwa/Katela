## What is this
These are not the same kind of scripts as those found in Linux systems. Shell scripts are used to automate command processes within Katela’s own shell. However, these system scripts are treated as applications, so they are isolated in User mode (Ring 3) to prevent them from harming the system or causing a panic in the event of an error

## Special syntax
All programs must be written in swiss. To let swiss know which file to write the text to, type ``?<script_name>`` on the first line, replace "script_name" with the name of the file you want to write to. The commands are no different from the usual ones, but you need to add `` :`` at the end so as not to confuse the code

## How to execute
It's easy. Just type ``exec file_name`` and it will execute
