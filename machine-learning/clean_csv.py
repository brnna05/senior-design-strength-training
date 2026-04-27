import csv
import sys
from pathlib import Path

REQUIRED_COLUMNS = 9
TIMESTAMP_HEADER = "timestamp"  # Edge Impulse expects exactly this

def clean_csv(input_path: str, output_path: str = None):
    input_file = Path(input_path)

    if not input_file.exists():
        print(f"Error: file '{input_path}' not found.")
        sys.exit(1)

    if output_path is None:
        output_path = input_file.stem + "_fixed" + input_file.suffix

    kept = 0
    removed = 0

    with open(input_file, newline="", encoding="utf-8-sig") as infile, \
         open(output_path, "w", newline="", encoding="utf-8") as outfile:

        reader = csv.reader(infile)
        writer = csv.writer(outfile)

        for line_num, row in enumerate(reader, start=1):
            if line_num == 1:
                # Fix the timestamp header for Edge Impulse
                row[0] = TIMESTAMP_HEADER
                writer.writerow(row)
                continue

            if len(row) != REQUIRED_COLUMNS:
                print(f"  Removed line {line_num}: has {len(row)} column(s) -> {row}")
                removed += 1
                continue

            # Convert timestamp from microseconds to milliseconds
            try:
                row[0] = float(row[0]) / 1000.0
            except ValueError:
                print(f"  Removed line {line_num}: invalid timestamp -> {row}")
                removed += 1
                continue

            writer.writerow(row)
            kept += 1

    print(f"\nDone. Kept {kept} rows, removed {removed} rows.")
    print(f"Output saved to: {output_path}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python clean_csv.py <input.csv> [output.csv]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) >= 3 else None

    clean_csv(input_path, output_path)