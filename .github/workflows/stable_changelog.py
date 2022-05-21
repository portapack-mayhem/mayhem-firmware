import os
import re
import sys

past_version = sys.argv[1]

raw_git = os.popen('git log ' + past_version + '..next --pretty=format:"- %h - {USERNAME}*+%al-%an*: %s"').read()


def compute_username(line):
    stripped = re.search(r'(?<=\*)(.*?)(?=\*)', line).group(0)

    pattern = re.compile("[$@+&?].*[$@+&?]")
    if pattern.match(stripped):
        stripped = re.sub("[$@+&?].*[$@+&?]", "", stripped)
        stripped = re.match(r'.+?(?=-)', stripped).group(0)
    else:
        stripped = re.sub(r'^.*?-', "", stripped)
    return "@" + stripped


def compile_line(line):
    username = compute_username(line)
    line = re.sub(r'[*].*[*]', "", line)
    line = line.replace("{USERNAME}", username)
    return line


for row in raw_git.splitlines():
    print(compile_line(row))
