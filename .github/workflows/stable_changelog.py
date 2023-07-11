import os
import re
import sys
import json
from urllib.error import HTTPError

past_version = sys.argv[1]

main_repo_url = "https://github.com/eried/portapack-mayhem.git"
main_repo_identifier = "eried/portapack-mayhem"
# ^^&^ change these if main repo changed

os.system('gh repo set-default ' + main_repo_url)

raw_git = os.popen('git log ' + past_version + '..next --pretty=format:"- ^%h^ - {USERNAME}*_%al_%an*: %s"').read()


# ^ as github's rule, a real username can contains "-" but not "_" and "*", so use these two to seperate things

# during test at 2023-07-10, %al could return 3 different types of values:
# 123+abc <<<< abc is real username
# def <<<< def is 3rd level domain of unhidden email, drop this, fetch info from github cli client
# note that github will probably change in the future

def compute_username(line):
    line_org = line
    stripped = re.search(r'(?<=\*)(.*?)(?=\*)', line).group(0)  # get the string between "*" and "*"
    now_item_hash = re.search(r'(?<=\^)(.*?)(?=\^)', line).group(0)  # get the string between "*" and "*"
    stripped_org = stripped
    stripped = re.search(r'(?<=\_)(.*?)(?=\_)', stripped).group(0)  # get the string between "_" and "_"
    stripped_fallback = stripped
    # now it's like "123+abc" or "def"

    if ("+" in stripped):  # 123+abc
        stripped = re.sub(r'^.*?\+', "", stripped)
    elif not ("+" in stripped):  # maybe not real username, dropped, fetch from github cli client
        fetched_now_item_json = os.popen(
            'gh search commits repo:' + main_repo_identifier + ' --json author --hash ' + now_item_hash).read()
        stripped = extract_first_login(fetched_now_item_json)
        if stripped is False:  # 403
            return "@" + stripped_fallback
        elif stripped == "app/":
            return "@" + stripped_fallback
    else:  # exception and edge cases
        return "@" + stripped_fallback

    if stripped is None:  # if commit hash can't find AKA commit is only in fork
        return "@" + stripped_fallback

    return "@" + stripped


def compile_line(line):
    username = compute_username(line)
    line = re.sub(r'[*].*[*]', "", line)
    line = line.replace("{USERNAME}", username)
    line = re.sub(r'\^', '', line, count=2)

    return line


def extract_first_login(json_data):
    if not json_data:  # if returned null
        return None

    try:
        data = json.loads(json_data)

        if isinstance(data, list) and len(data) > 0:
            first_object = data[0]
            if 'author' in first_object and 'login' in first_object['author']:
                return first_object['author']['login']

    except HTTPError as e:
        if e.code == 403 and "API rate limit exceeded" in str(e):
            return False
    except json.decoder.JSONDecodeError:
        return None

    return None


for row in raw_git.splitlines():
    print(compile_line(row))
