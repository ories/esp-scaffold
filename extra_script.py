import datetime
import subprocess

Import("env")


def git(*args):
    try:
        return subprocess.check_output(
            ["git", *args], stderr=subprocess.DEVNULL
        ).decode().strip()
    except Exception:
        return ""


commit = git("rev-parse", "--short", "HEAD") or "nogit"
if git("status", "--porcelain"):
    commit += "-dirty"
timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

env.Append(CPPDEFINES=[
    ("GIT_COMMIT", '\\"%s\\"' % commit),
    ("BUILD_TIMESTAMP", '\\"%s\\"' % timestamp),
])
