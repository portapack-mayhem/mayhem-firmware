import os
import re
import sys

past_version = sys.argv[1]

raw_git = os.popen('git log ' + past_version + '..next --pretty=format:"- %h - {USERNAME}*+%al-%an*: %s"').read()
raw_merge_log = os.popen('git log ' + past_version + '..next --merges --pretty=format:"%s"').read()


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


def merge_log_line_to_author_username(line):
    if re.search(r'Merge pull request #\d+ from ([A-Za-z\d-]+)', line) is None:
        return None
    return "@" + re.search(r'Merge pull request #\d+ from ([A-Za-z\d-]+)/([A-Za-z\d-]+)', line).group(1)


def rank_authors():
    authors = {}
    for name_item in raw_merge_log.splitlines():
        author = merge_log_line_to_author_username(name_item)
        if author is None:
            continue
        if author in authors:
            authors[author] += 1
        else:
            authors[author] = 1
    return authors


for row in raw_git.splitlines():
    print(compile_line(row))

print("\n\ncontributors:")

authors = rank_authors()
for author in authors:
    print(author + " : " + str(authors[author]) + " PRs")
