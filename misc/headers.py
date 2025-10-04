# Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
# Original version by Ryan Nicholl, improved by ChatGPT

import os
import sys
import re
import argparse
from typing import Optional, Tuple

def find_header_guards(directory: str, fix_in_place: bool = False):
    """
    Recursively find all header files in a specified directory,
    and check their header guards. Optionally fix mismatches in-place.

    Args:
        directory (str): The root directory to search for header files.
        fix_in_place (bool): If True, update the header guards in the files.
    """
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.hpp', '.h', '.hh', '.hxx', '.h++', '.hp')):  # Include other header extensions as needed
                file_path = os.path.join(root, file)
                header_guard, define_guard, lines, ifndef_idx, define_idx = extract_header_guards(file_path)
                expected_guard = generate_expected_guard(file_path)

                if header_guard and header_guard != expected_guard:
                    print(f"Mismatch in {file_path}:")
                    print(f"  Found #ifndef: {header_guard}")
                    print(f"  Found #define: {define_guard}")
                    print(f"  Expected: {expected_guard}\n")

                    if fix_in_place:
                        updated = update_header_guards(file_path, lines, ifndef_idx, define_idx, expected_guard)
                        if updated:
                            print(f"Updated header guards in {file_path}\n")
                        else:
                            print(f"Failed to update header guards in {file_path}\n")
                elif not header_guard:
                    print(f"No header guard found in {file_path}\n")

def extract_header_guards(file_path: str) -> Tuple[Optional[str], Optional[str], list, Optional[int], Optional[int]]:
    """
    Extract the header guard from a given header file, including the define guard.

    Args:
        file_path (str): Path to the header file.

    Returns:
        Tuple containing:
            - The header guard from #ifndef (str or None)
            - The header guard from #define (str or None)
            - All lines of the file as a list
            - Line index of the #ifndef header guard (int or None)
            - Line index of the #define header guard (int or None)
    """
    header_guard = None
    define_guard = None
    lines = []
    ifndef_idx = None
    define_idx = None

    try:
        with open(file_path, 'r') as file:
            for idx, line in enumerate(file):
                lines.append(line)
                stripped_line = line.strip()
                if not header_guard and stripped_line.startswith('#ifndef'):
                    parts = stripped_line.split()
                    if len(parts) >= 2:
                        header_guard = parts[1]
                        ifndef_idx = idx
                elif header_guard and not define_guard and stripped_line.startswith('#define'):
                    parts = stripped_line.split()
                    if len(parts) >= 2:
                        define_guard = parts[1]
                        define_idx = idx
                        # No break; continue reading to capture all lines
    except IOError as e:
        print(f"Error opening file {file_path}: {e}")

    return header_guard, define_guard, lines, ifndef_idx, define_idx

def generate_expected_guard(file_path: str) -> str:
    """
    Generate the expected header guard based on the file path,
    excluding header file extensions like .hpp, .h, .hh.

    Args:
        file_path (str): Path to the header file.

    Returns:
        The expected header guard string.
    """
    # Normalize path separators to '/'
    normalized_path = file_path.replace(os.sep, '/')

    # Extract the portion of the path after 'include/' if present
    if 'include/' in normalized_path:
        guard_part = normalized_path.split('include/')[-1]
    else:
        guard_part = normalized_path

    # Split the path into components
    path_components = guard_part.split('/')

    # Remove the file extension from the last component if it's a known header extension
    filename = path_components[-1]
    filename_no_ext, ext = os.path.splitext(filename)
    if ext.lower() in ['.hpp', '.h', '.hh', '.hxx', '.h++', '.hp']:
        path_components[-1] = filename_no_ext  # Replace with filename without extension

    # Reconstruct the guard part without the file extension
    guard_part_no_ext = '/'.join(path_components)

    # Replace non-alphanumeric characters with underscores and convert to upper case
    guard = re.sub(r'[^a-zA-Z0-9]', '_', guard_part_no_ext).upper()

    return f"{guard}_HEADER"

def update_header_guards(file_path: str, lines: list, ifndef_idx: Optional[int], define_idx: Optional[int], expected_guard: str) -> bool:
    """
    Update the #ifndef and #define lines with the expected header guard.

    Args:
        file_path (str): Path to the header file.
        lines (list): List of all lines in the file.
        ifndef_idx (int): Line index of the #ifndef header guard.
        define_idx (int): Line index of the #define header guard.
        expected_guard (str): The expected header guard.

    Returns:
        True if the file was successfully updated, False otherwise.
    """
    try:
        if ifndef_idx is not None:
            # Update the #ifndef line
            lines[ifndef_idx] = f"#ifndef {expected_guard}\n"

        if define_idx is not None:
            # Update the #define line
            lines[define_idx] = f"#define {expected_guard}\n"

        # Write back to the file
        with open(file_path, 'w') as file:
            file.writelines(lines)

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
    parser = argparse.ArgumentParser(description="Check and optionally fix header guards in header files.")
    parser.add_argument('directory', help="Directory to search for header files.")
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
