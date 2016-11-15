#!/usr/bin/env python

"""
Python script to obtain GitHub Release download count and other statistics.
"""

import os
import sys
import json
import time

try:
    import urllib2
except ImportError:
    import urllib.request as urllib2

try:
    input = raw_input
except NameError:
    pass

__version__ = "1.0.1"
__author__ = "Alexander Gorishnyak"
__email__ = "kefir500@gmail.com"
__license__ = "MIT"


class ConnectionError(Exception):
    """
    Raised on connection error.
    """

    def __init__(self, reason=None):
        """
        :param reason: Connection error reason.
        """
        self.reason = reason if reason else "Unknown reason."

    def __str__(self):
        return "Connection error: " + str(self.reason)


class GithubError(Exception):
    """
    Generic exception raised on GitHub API HTTP error.
    """

    def __init__(self, code, message=None):
        """
        :param code: HTTP error code.
        :param message: Exception message text.
        """
        self.code = code
        self.message = message

    def __str__(self):
        return "GitHub API HTTP {0}{1}".format(self.code, (": " + self.message) if self.message else "")


class GithubRepoError(GithubError):
    """
    Raised when accessing nonexistent GitHub username, repository or release tag.
    """

    def __init__(self, message=None):
        """
        :param message: Exception message text.
        """
        self.code = 404
        self.message = message if message else "Invalid GitHub username, repository or release tag."


class GithubLimitError(GithubError):
    """
    Raised when GitHub API request limit is exceeded.
    """

    def __init__(self, message=None):
        """
        :param message: Exception message text.
        """
        self.code = 403
        self.message = message if message else "Request limit exceeded."


class GithubTokenError(GithubError):
    """
    Raised when trying to pass invalid GitHub OAuth token.
    """

    def __init__(self, message=None):
        """
        :param message: Exception message text.
        """
        self.code = 401
        self.message = message if message else "Unauthorized. Check your \"GITHUB_TOKEN\" environment variable."


class _Text:
    """
    Definitions for colored output (no ANSI colors on Windows).
    """
    if os.name != "nt":
        HEADER = "\033[1;36m"
        SUCCESS = "\033[1;32m"
        ERROR = "\033[1;31m"
        BOLD = "\033[1m"
        ITALIC = "\033[3m"
        UNDERLINE = "\033[4m"
        END = "\033[0m"
    else:
        HEADER = SUCCESS = ERROR = BOLD = ITALIC = UNDERLINE = END = ""


def error(message):
    """
    Halt script due to critical error.
    :param message: Error text.
    """
    sys.exit(_Text.ERROR + "Error: " + message + _Text.END)


def get_env_token():
    """
    Get GitHub OAuth token from "GITHUB_TOKEN" environment variable.
    :return: GitHub OAuth token.
    """
    token = None if "GITHUB_TOKEN" not in os.environ else os.environ["GITHUB_TOKEN"]
    return token


def print_help():
    """
    Display command line help.
    """
    print("GitHub Download Stats")
    print("Python script to obtain GitHub Release download count and other statistics.\n")
    print("Usage:\n"
          "  python ghstats.py [{0}user{1}] [{0}repo{1}] [{0}tag{1}] [{0}options{1}]\n"
          "  python ghstats.py [{0}user{1}/{0}repo{1}] [{0}tag{1}] [{0}options{1}]\n"
          .format(_Text.ITALIC, _Text.END))
    print("Arguments:\n"
          "  {0}user{1}  Repository owner. If not present, user will be prompted for input.\n"
          "  {0}repo{1}  Repository title. If not present, user will be prompted for input.\n"
          "  {0}tag{1}   Release tag name. If not present, prints the total number of downloads.\n"
          .format(_Text.BOLD, _Text.END))
    print("Options:\n"
          "  {0}-d{1}, {0}--detail{1}  Print detailed statistics for release(s).\n"
          "  {0}-q{1}, {0}--quiet{1}   Print only resulting numbers and errors.\n"
          "                Overrides -d option.\n"
          "  {0}-l{1}, {0}--latest{1}  Get stats for the latest release.\n"
          "                Tag argument will be ignored.\n"
          "  {0}-h{1}, {0}--help{1}    Show this help.\n"
          .format(_Text.BOLD, _Text.END))
    print("Environment Variables:\n"
          "  {0}GITHUB_TOKEN{2}   GitHub OAuth token.\n"
          "                 Use to increase API request limit.\n"
          "                 {1}https://github.com/settings/tokens{2}"
          .format(_Text.BOLD, _Text.UNDERLINE, _Text.END))
    sys.exit(0)


def print_greeting():
    """
    Display greeting message.
    """
    print("GitHub Download Stats\n"
          "Author: Alexander Gorishnyak <kefir500@gmail.com>\n")


def download_stats(user=None, repo=None, tag=None, latest=False, token=None, quiet=False):
    """
    Get download statistics from GitHub API.
    :param user: GitHub repository owner username. If empty, user will be prompted for input.
    :param repo: GitHub repository name. If empty, user will be prompted for input.
    :param tag: Release tag name. If empty, get stats for all releases.
    :param latest: If True, ignore "tag" parameter and get stats for the latest release.
    :param token: GitHub OAuth token. If empty, API request limit will be reduced.
    :param quiet: If True, print nothing.
    :return: Statistics on downloads.
    :raises GithubRepoError: When accessing nonexistent GitHub username, repository or release tag.
    :raises GithubLimitError: When GitHub API request limit is exceeded.
    :raises GithubTokenError: When trying to pass invalid GitHub API OAuth token.
    :raises ConnectionError: On connection error.
    """
    if not user:
        user = input("GitHub Username: ")
    if not repo:
        repo = input("GitHub Repository: ")
    if not quiet:
        print("Downloading {0}/{1} stats...".format(user, repo))
    url = "https://api.github.com/repos/{0}/{1}/releases".format(user, repo)
    url += ("" if not tag else "/tags/" + tag) if not latest else "/latest"
    headers = {} if not token else {"Authorization": "token " + token}
    request = urllib2.Request(url, headers=headers)
    start = time.time()
    try:
        response = urllib2.urlopen(request).read().decode("utf-8")
    except urllib2.HTTPError as e:
        if e.code == 404:
            raise GithubRepoError()    # Invalid GitHub username, repository or release tag.
        elif e.code == 403:
            raise GithubLimitError()   # GitHub API request limit exceeded.
        elif e.code == 401:
            raise GithubTokenError()   # Invalid GitHub OAuth token.
        else:
            raise GithubError(e.code)  # Generic GitHub API exception.
    except urllib2.URLError as e:
        raise ConnectionError(e.reason)
    stats = json.loads(response)
    if not quiet:
        end = time.time()
        print("Downloaded in {0:.3f}s".format(end - start))
    return stats


def get_release_downloads(release, quiet=False):
    """
    Get number of downloads for a single release.
    :param release: Release statistics from GitHub API.
    :param quiet: If True, print nothing.
    :return: Number of downloads for a release.
    """
    downloads_total = 0
    if not quiet:
        published = time.strptime(release["published_at"], "%Y-%m-%dT%H:%M:%SZ")
        print("")
        print("          --- " + _Text.HEADER + release["name"] + _Text.END + " ---\n")
        print("         Tag: " + release["tag_name"])
        print("      Author: " + release["author"]["login"])
        print("         URL: " + _Text.UNDERLINE + release["html_url"] + _Text.END)
        print("Published at: " + time.strftime("%c", published))
        print("")
    if "assets" in release:
        indent = 13 + len(_Text.BOLD + _Text.END)
        for package in release["assets"]:
            downloads_release = package["download_count"]
            downloads_total += downloads_release
            if not quiet:
                print("{1:>{0}} {2}".format(
                      indent, _Text.BOLD + str(downloads_release) + _Text.END,
                      (package["label"] if package["label"] else package["name"])))
        if not quiet:
            print("{1:>{0}} Total".format(indent, _Text.BOLD + str(downloads_total) + _Text.END))
    return downloads_total


def get_stats_downloads(stats, quiet=False):
    """
    Get number of downloads from statistics.
    :param stats: Statistics from GitHub API.
    :param quiet: If True, print nothing.
    :return: Number of downloads.
    """
    total = 0
    if isinstance(stats, dict):
        total = get_release_downloads(stats, quiet)
    else:
        for release in stats:
            total += get_release_downloads(release, quiet)
    return total


def print_total(total, quiet=False, tag=None):
    """
    Print total number of downloads.
    :param total: Total number of downloads.
    :param quiet: If True, print only number of downloads without an additional text.
    :param tag: Release tag name (optional).
    :return: Total number of downloads passed via "total" parameter.
    """
    if not quiet:
        print("\n{0}Total Downloads{1}: {2}"
              .format(_Text.BOLD,
                      "" if not tag else " (" + tag + ")",
                      _Text.SUCCESS + str(total) + _Text.END))
    elif __name__ == "__main__":
        print(str(total))
    return total


def main(user=None, repo=None, tag=None, latest=False, detail=False, token=None, quiet=False):
    """
    Get number of downloads for GitHub release(s).
    :param user: GitHub repository owner username. If empty, user will be prompted for input.
    :param repo: GitHub repository name. If empty, user will be prompted for input.
    :param tag: Release tag name. If empty, get stats for all releases.
    :param latest: If True, ignore "tag" parameter and get stats for the latest release.
    :param detail: Detailed output containing release information.
    :param token: GitHub OAuth token. If empty, API request limit will be reduced.
    :param quiet: If True, print nothing.
    :return: Number of downloads.
    """
    if not quiet:
        print_greeting()
    try:
        stats = download_stats(user, repo, tag, latest, token, quiet)
    except GithubError as e:
        error(e.message)
    except ConnectionError:
        error("Connection error.")
    else:
        total = get_stats_downloads(stats, quiet or not detail)
        print_total(total, quiet, tag or (stats["tag_name"] if latest else None))
        return total


def main_cli(args):
    """
    Parse command line arguments and pass to the main function.
    :param args: Command line arguments (without script name).
    :return: Number of downloads.
    """
    user = None              # GitHub username
    repo = None              # GitHub repository
    tag = None               # GitHub release tag
    latest = False           # Latest release
    detail = False           # Detailed output
    quiet = False            # Quiet output
    token = get_env_token()  # GitHub token
    for arg in args:
        if arg == "-q" or arg == "--quiet":
            quiet = True
        elif arg == "-d" or arg == "--detail":
            detail = True
        elif arg == "-l" or arg == "--latest":
            latest = True
        elif arg == "-h" or arg == "--help" or arg == "-?":
            print_help()
        else:
            if not user:
                if "/" not in arg:
                    user = arg
                else:
                    userrepo = arg.split("/")
                    user = userrepo[0]
                    repo = userrepo[1]
            elif not repo:
                repo = arg
            elif quiet and detail and latest:
                break
            elif not tag:
                tag = arg
    return main(user, repo, tag, latest, detail, token, quiet)


if __name__ == "__main__":
    try:
        main_cli(sys.argv[1:])
    except KeyboardInterrupt:
        print("")
