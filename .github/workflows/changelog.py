import os
import sys

import requests
from datetime import datetime, timedelta, timezone

# Set up your personal access token and the repository details
token = os.environ.get('GH_TOKEN')
repo_owner = "portapack-mayhem"
repo_name = "mayhem-firmware"


def print_stable_changelog(previous_sha):
    url = f"compare/{previous_sha}...next"
    commits = handle_get_request(url)
    for commit in commits["commits"]:
        # Print the commit details
        print(format_output(commit))


def print_nightly_changelog():
    # Calculate the date and time 24 hours ago
    since_time = (datetime.now(timezone.utc) - timedelta(hours=24)).isoformat()  # Check that this is UTC
    url = "commits"
    commits = handle_get_request(url, since_time)
    for commit in commits:
        # Print the commit details
        print(format_output(commit))


def handle_get_request(path, offset=None):
    headers = {} if token == None else {"Authorization": f"Bearer {token}"}
    params = {"since": offset}
    url_base = f"https://api.github.com/repos/{repo_owner}/{repo_name}/"
    response = requests.get(url_base + path, headers=headers, params=params)

    # Check if the request was successful (status code 200)
    if response.status_code == 200:
        return response.json()
    else:
        print(f"Request failed with status code: {response.status_code}")
    return None


def format_output(commit):
    message_lines = commit["commit"]["message"].split("\n")
    author = commit["author"]["login"] if commit["author"] and "login" in commit["author"] else commit["commit"]["author"]["name"]
    return '- ' + commit["sha"][:8] + ' - @' + author + ': ' + message_lines[0]


if len(sys.argv) < 2:
    print_nightly_changelog()
else:
    past_version = sys.argv[1]
    print_stable_changelog(past_version)
