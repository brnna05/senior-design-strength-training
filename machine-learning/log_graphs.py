import os
import glob
import pandas as pd
import matplotlib.pyplot as plt


COLUMNS = [
    "timestamp_us",
    "voltage_mv",
    "accel_x",
    "accel_y",
    "accel_z",
    "gyro_x",
    "gyro_y",
    "gyro_z",
    "ppg_ir",
]


def load_csv(file_path):
    """
    Reads a CSV file produced by the firmware printk:
        timestamp_us, voltage_mv, accel_x, accel_y, accel_z,
        gyro_x, gyro_y, gyro_z, ppg_ir

    Lines that are not 9 comma-separated integers (e.g. heart-rate log
    messages, IMU motion alerts, blank lines) are silently skipped.
    """
    rows = []
    with open(file_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split(",")
            if len(parts) != len(COLUMNS):
                continue           # skip log messages / bad lines
            try:
                rows.append([int(p.strip()) for p in parts])
            except ValueError:
                continue           # skip header rows or anything non-numeric

    if not rows:
        raise ValueError(f"No valid data rows found in {file_path}")

    df = pd.DataFrame(rows, columns=COLUMNS)
    # Convert µs timestamp to seconds for readability
    df["time_s"] = (df["timestamp_us"] - df["timestamp_us"].iloc[0]) / 1e6
    return df


def plot_csv(file_path):
    df = load_csv(file_path)
    base = os.path.basename(file_path)

    has_imu = df[["accel_x", "accel_y", "accel_z",
                  "gyro_x", "gyro_y", "gyro_z"]].any().any()
    has_ppg = df["ppg_ir"].any()

    # Decide subplot layout
    n_plots = 1 + (1 if has_imu else 0) + (1 if has_ppg else 0)
    fig, axes = plt.subplots(n_plots, 1, figsize=(12, 4 * n_plots), sharex=True)
    if n_plots == 1:
        axes = [axes]

    ax_idx = 0

    # ── EMG voltage ──────────────────────────────────────────────────────
    axes[ax_idx].plot(df["time_s"], df["voltage_mv"], linewidth=0.8)
    axes[ax_idx].set_ylabel("Voltage (mV)")
    axes[ax_idx].set_title(f"{base} — EMG")
    axes[ax_idx].grid(True)
    ax_idx += 1

    # ── IMU ──────────────────────────────────────────────────────────────
    if has_imu:
        axes[ax_idx].plot(df["time_s"], df["accel_x"], label="accel_x (mg)")
        axes[ax_idx].plot(df["time_s"], df["accel_y"], label="accel_y (mg)")
        axes[ax_idx].plot(df["time_s"], df["accel_z"], label="accel_z (mg)")
        axes[ax_idx].plot(df["time_s"], df["gyro_x"],  label="gyro_x (mdps)", linestyle="--")
        axes[ax_idx].plot(df["time_s"], df["gyro_y"],  label="gyro_y (mdps)", linestyle="--")
        axes[ax_idx].plot(df["time_s"], df["gyro_z"],  label="gyro_z (mdps)", linestyle="--")
        axes[ax_idx].set_ylabel("IMU")
        axes[ax_idx].set_title("Accelerometer & Gyroscope")
        axes[ax_idx].legend(fontsize=7, ncol=3)
        axes[ax_idx].grid(True)
        ax_idx += 1

    # ── PPG IR ───────────────────────────────────────────────────────────
    if has_ppg:
        axes[ax_idx].plot(df["time_s"], df["ppg_ir"], color="red", linewidth=0.8)
        axes[ax_idx].set_ylabel("IR Count")
        axes[ax_idx].set_title("PPG (MAX30102 IR)")
        axes[ax_idx].grid(True)
        ax_idx += 1

    axes[-1].set_xlabel("Time (s)")
    plt.tight_layout()
    plt.show()


def main():
    csv_files = glob.glob("*.csv")
    if not csv_files:
        print("No CSV files found in this folder.")
        return
    for file in sorted(csv_files):
        print(f"Plotting {file}")
        try:
            plot_csv(file)
        except ValueError as e:
            print(f"  Skipped: {e}")


if __name__ == "__main__":
    main()