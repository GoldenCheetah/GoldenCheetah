import re
import os
import sys

# List of known legacy violations to exclude.
# Format: "RelativeFilePath:LineNumber" or just "RelativeFilePath" if we want to exclude the whole file.
# We will use "RelativeFilePath:LineNumber" for precision.
EXCLUSIONS = {}

def check_file(filepath, relative_path):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    unsafe_matches = []

    indices = [m.start() for m in re.finditer(r'connect\s*\(', content)]

    for idx in indices:
        chunk = content[idx:idx+500]
        inner = chunk[chunk.find('(')+1:]

        args = []
        current_arg = ""
        paren_depth = 0
        in_quote = False

        for char in inner:
            if char == '"':
                in_quote = not in_quote
            elif not in_quote:
                if char == '(':
                    paren_depth += 1
                elif char == ')':
                    if paren_depth == 0:
                        args.append(current_arg.strip())
                        break
                    paren_depth -= 1
                elif char == ',' and paren_depth == 0:
                    args.append(current_arg.strip())
                    current_arg = ""
                    continue
            current_arg += char

        if len(args) >= 3:
            third_arg = args[2]
            if third_arg.startswith('['):
                line_no = content[:idx].count('\n') + 1

                # Check exclusion
                if relative_path in EXCLUSIONS and line_no in EXCLUSIONS[relative_path]:
                    continue

                # We also check if the exclusion is off by one or two lines due to edits?
                # For now strict line check.

                unsafe_matches.append((line_no, chunk.split('\n')[0]))

    return unsafe_matches

def scan_directory(root_dir):
    print(f"Scanning {root_dir}...")
    count = 0
    found_violations = 0

    # Normalize path for exclusion check
    # We assume we are running from root or passed the src root.
    # If root_dir is absolute, we need to handle it.

    base_path = os.path.abspath(root_dir)
    # If we are given say /.../src, then relative path for exclusion should simply be src/...
    # But our exclusions start with 'src/'.

    # Let's try to detect the 'src' part.
    parent_of_src = os.path.dirname(base_path)
    if os.path.basename(base_path) == 'src':
        project_root = parent_of_src
    else:
        project_root = base_path # fallback

    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                filepath = os.path.join(root, file)

                # Create relative path for exclusion check (e.g., src/Gui/Agenda.cpp)
                rel_path = os.path.relpath(filepath, project_root)

                matches = check_file(filepath, rel_path)
                if matches:
                    print(f"File: {rel_path}")
                    for line, text in matches:
                        print(f"  Line {line}: UNSAFE PATTERN (New Violation)")
                    found_violations += len(matches)
                count += 1

    if found_violations > 0:
        print(f"FAILED: Found {found_violations} new unsafe signal connections.")
        sys.exit(1)
    else:
        print(f"SUCCESS: Scanned {count} files. No new unsafe patterns found.")
        sys.exit(0)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        scan_directory(sys.argv[1])
    else:
        # Default assume running from repo root
        if os.path.exists("src"):
            scan_directory("src")
        elif os.path.exists("../src"):
            scan_directory("../src")
        else:
             print("Could not find src directory.")
             sys.exit(1)
