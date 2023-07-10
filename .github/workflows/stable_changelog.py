import os
import re
import sys

past_version = sys.argv[1]

raw_git = os.popen('git log ' + past_version + '..next --pretty=format:"- %h - {USERNAME}*_%al_%an*: %s"').read()
# ^ as github's rule, a real username can contains "-" but not "_" and "*", so use these two to seperate things

# during test at 2023-07-10, %al could return 3 different types of values:
# 123+abc
# def@ghi.mgl+abc
# abc
# admin
# ^ abc is real username, note that github will probably change in the future!!!

def compute_username(line):
    stripped = re.search(r'(?<=\*)(.*?)(?=\*)', line).group(0) # get the string between "*" and "*"
    stripped_org = stripped
    stripped = re.search(r'(?<=\_)(.*?)(?=\_)', stripped).group(0) # get the string between "_" and "_"
    # now it's like "123+abc" or "def@ghi+mgl+abc" or "abc" or "admin"

    if ("+" in stripped): # like 123+abc or def@ghi.mgl+abc
        stripped = re.sub(r'^.*?\+', "", stripped)
    elif (stripped == "admin"):
        # remove "_admin_" from stripped_org and return "@" + stripped_org
        stripped = re.sub(r'_admin_', "", stripped_org)
    else: # like abc or exception
        pass
    return "@" + stripped


def compile_line(line):
    username = compute_username(line)
    line = re.sub(r'[*].*[*]', "", line)
    line = line.replace("{USERNAME}", username)
    return line


for row in raw_git.splitlines():
    print(compile_line(row))
