import os
import sys
import re
import argparse
from typing import Optional, Tuple

def find_header_guards(directory: str, fix_in_place: bool = False):
    """
    Recursively find all .hpp files in a specified directory,
    and check their header guards. Optionally fix mismatches in-place.

    Args:
        directory (str): The root directory to search for .hpp files.
        fix_in_place (bool): If True, update the header guards in the files.
    """
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith('.hpp'):
                file_path = os.path.join(root, file)
                header_guard, define_guard, lines = extract_header_guards(file_path)
                expected_guard = generate_expected_guard(file_path)

                if header_guard and header_guard != expected_guard:
                    print(f"Mismatch in {file_path}:")
                    print(f"  Found #ifndef: {header_guard}")
                    print(f"  Found #define: {define_guard}")
                    print(f"  Expected: {expected_guard}\n")

                    if fix_in_place:
                        updated = update_header_guards(file_path, lines, header_guard, define_guard, expected_guard)
                        if updated:
                            print(f"Updated header guards in {file_path}\n")
                        else:
                            print(f"Failed to update header guards in {file_path}\n")
                elif not header_guard:
                    print(f"No header guard found in {file_path}\n")

def extract_header_guards(file_path: str) -> Tuple[Optional[str], Optional[str], list]:
    """
    Extract the header guard from a given .hpp file, including the define guard.

    Args:
        file_path (str): Path to the .hpp file.

    Returns:
        Tuple containing:
            - The header guard from #ifndef (str or None)
            - The header guard from #define (str or None)
            - All lines of the file as a list
    """
    header_guard = None
    define_guard = None
    lines = []

    try:
        with open(file_path, 'r') as file:
            for idx, line in enumerate(file):
                lines.append(line)
                if not header_guard and line.strip().startswith('#ifndef'):
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        header_guard = parts[1]
                elif header_guard and not define_guard and line.strip().startswith('#define'):
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        define_guard = parts[1]
                        break  # Assuming #define follows #ifndef immediately
    except IOError as e:
        print(f"Error opening file {file_path}: {e}")

    return header_guard, define_guard, lines

def generate_expected_guard(file_path: str) -> str:
    """
    Generate the expected header guard based on the file path.

    Args:
        file_path (str): Path to the .hpp file.

    Returns:
        The expected header guard string.
    """
    # Normalize path separators
    normalized_path = file_path.replace(os.sep, '/')

    # Extract the portion of the path after 'include/' if present
    if 'include/' in normalized_path:
        guard_part = normalized_path.split('include/')[-1]
    else:
        guard_part = normalized_path

    # Replace non-alphanumeric characters with underscores and convert to upper case
    guard = re.sub(r'[^a-zA-Z0-9]', '_', guard_part).upper()

    return f"{guard}_HEADER_GUARD"

def update_header_guards(file_path: str, lines: list, current_ifndef: str, current_define: str, expected_guard: str) -> bool:
    """
    Update the #ifndef and #define lines with the expected header guard.

    Args:
        file_path (str): Path to the .hpp file.
        lines (list): List of all lines in the file.
        current_ifndef (str): Current header guard from #ifndef.
        current_define (str): Current header guard from #define.
        expected_guard (str): The expected header guard.

    Returns:
        True if the file was successfully updated, False otherwise.
    """
    try:
        # Update lines
        updated_lines = []
        for line in lines:
            if line.strip().startswith('#ifndef') and current_ifndef:
                updated_line = f"#ifndef {expected_guard}\n"
                updated_lines.append(updated_line)
            elif line.strip().startswith('#define') and current_define:
                updated_line = f"#define {expected_guard}\n"
                updated_lines.append(updated_line)
            else:
                updated_lines.append(line)

        # Write back to the file
        with open(file_path, 'w') as file:
            file.writelines(updated_lines)

        return True
    except IOError as e:
        print(f"Error updating file {file_path}: {e}")
        return False

def parse_arguments():
    """
    Parse command-line arguments.

    Returns:
        Namespace with parsed arguments.
    """
    parser = argparse.ArgumentParser(description="Check and optionally fix header guards in .hpp files.")
    parser.add_argument('directory', help="Directory to search for .hpp files.")
    parser.add_argument('-i', '--in-place', action='store_true', help="Update header guards in-place if mismatched.")
    return parser.parse_args()

def main():
    args = parse_arguments()
    directory = args.directory
    fix_in_place = args.in_place

    if not os.path.isdir(directory):
        print(f"Error: '{directory}' is not a valid directory.")
        sys.exit(1)

    find_header_guards(directory, fix_in_place)

if __name__ == "__main__":
    main()
