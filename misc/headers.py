import os
import sys
import re


import os
import re

def find_header_guards(directory):
    """
    Recursively find all .hpp files in a specified directory,
    and print their path if the header guard doesn't match the expected pattern
    based on the file path.
    """
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith('.hpp'):
                file_path = os.path.join(root, file)
                header_guard = extract_header_guard(file_path)
                expected_guard = generate_expected_guard(file_path)
                if header_guard and header_guard != expected_guard:
                    print(f"{file_path}: {header_guard} (Expected: {expected_guard})")

def extract_header_guard(file_path):
    """
    Extract the header guard from a given .hpp file.
    """
    try:
        with open(file_path, 'r') as file:
            for line in file:
                if '#ifndef' in line:
                    # Assuming the header guard is the word following '#ifndef'
                    return line.split()[1]
    except IOError:
        print(f"Error opening file: {file_path}")
    return None

def generate_expected_guard(file_path):
    """
    Generate the expected header guard based on the file path.
    """
    # Extract the portion of the path after 'include/'
    guard_part = file_path.split('include/')[-1]

    # Replace slashes with underscores and convert to upper case
    guard = guard_part.replace('/', '_').replace('.hpp', '').upper()

    return f"{guard}_HEADER_GUARD"



# find header guards of first argument
find_header_guards(sys.argv[1])
