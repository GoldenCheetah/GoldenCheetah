import re
import os
import sys

def check_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Pattern: invisibleRootItem()->child(index)
    # We want to catch cases where this is used directly without checking the result.
    # It's hard to prove "without checking" via regex, but we can look for immediate dereference or usage strings.
    # e.g. "invisibleRootItem()->child(x)->text(0)" is definitely unsafe if child(x) returns null.
    # "QTreeWidgetItem *item = ...->child(x);" is okay IF followed by a check.

    # Let's focus on immediate chaining which is the most dangerous:
    # ->child(...)->
    # or ->child(...).

    warnings = []

    # Regex for immediate dereference: child(...) ->
    regex = re.compile(r'invisibleRootItem\(\)->child\([^)]+\)->')

    for i, line in enumerate(content.splitlines()):
        if regex.search(line):
             warnings.append((i+1, line.strip()))

    return warnings

def scan_directory(root_dir):
    print(f"Scanning {root_dir}...")
    count = 0
    found = 0
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                filepath = os.path.join(root, file)
                matches = check_file(filepath)
                if matches:
                    print(f"File: {filepath}")
                    for line, text in matches:
                        print(f"  Line {line}: UNSAFE CHAINING: {text}")
                    found += len(matches)
                count += 1
    print(f"Scanned {count} files. Found {found} potential unsafe usages.")
    return found

if __name__ == "__main__":
    found = 0
    if len(sys.argv) > 1:
        found = scan_directory(sys.argv[1])
    else:
        if os.path.exists("src"):
            found = scan_directory("src")
        elif os.path.exists("../src"):
            found = scan_directory("../src")
        else:
             print("Could not find src directory.")
             sys.exit(1)

    if found > 0:
        sys.exit(1)
    sys.exit(0)
