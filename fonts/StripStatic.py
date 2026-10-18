#!/usr/bin/env python3
"""
Abyss Engine — Font Stripper
Removes the 'static' subfolder from all Noto font directories.
Keeps the VariableFont files.
"""

import os
import shutil
import sys
from pathlib import Path

def strip_static_folders(base_dir):
    base_path = Path(base_dir)
    
    if not base_path.exists():
        print(f"Error: Directory '{base_path}' does not exist.")
        return

    deleted_count = 0
    skipped_count = 0

    print(f"Scanning {base_path} for 'static' folders...\n")

    # Iterate through every item in the fonts directory
    for item in base_path.iterdir():
        if item.is_dir():
            static_dir = item / "static"
            
            if static_dir.exists() and static_dir.is_dir():
                try:
                    # Remove the static folder and everything inside it
                    shutil.rmtree(static_dir)
                    print(f"[DELETED] {static_dir.relative_to(base_path)}")
                    deleted_count += 1
                except Exception as e:
                    print(f"[ERROR] Could not delete {static_dir}: {e}")
            else:
                skipped_count += 1

    print("\n" + "=" * 40)
    print(f"Done. Deleted {deleted_count} 'static' folders.")
    print(f"Skipped {skipped_count} folders that had no 'static' subfolder.")
    print("=" * 40)

if __name__ == "__main__":
    # Get the directory where the script is located
    script_dir = Path(__file__).parent.resolve()
    
    print("Abyss Engine Font Stripper")
    print("=" * 40)
    print(f"Target directory: {script_dir}")
    
    # Safety confirmation
    confirm = input("\nThis will permanently delete all 'static' subfolders. Continue? (y/n): ")
    
    if confirm.lower() == 'y':
        strip_static_folders(script_dir)
    else:
        print("Aborted. No files were deleted.")