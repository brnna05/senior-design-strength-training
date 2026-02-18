import os
import glob
import pandas as pd
import matplotlib.pyplot as plt


def plot_csv(file_path):
    """
    Reads a CSV file with format:
        time_ms, voltage_mv
    and plots it on its own graph.
    """

    try:
        # Try with header
        df = pd.read_csv(file_path)
        if df.shape[1] < 2:
            raise ValueError
        time = df.iloc[:, 0]
        voltage = df.iloc[:, 1]
    except:
        # Fallback: no header
        df = pd.read_csv(file_path, header=None)
        time = df.iloc[:, 0]
        voltage = df.iloc[:, 1]

    plt.figure()
    plt.plot(time, voltage)
    plt.xlabel("Time (ms)")
    plt.ylabel("Voltage (mV)")
    plt.title(os.path.basename(file_path))
    plt.grid(True)
    plt.show()


def main():
    csv_files = glob.glob("*.csv")

    if not csv_files:
        print("No CSV files found in this folder.")
        return

    for file in sorted(csv_files):
        print(f"Plotting {file}")
        plot_csv(file)


if __name__ == "__main__":
    main()
