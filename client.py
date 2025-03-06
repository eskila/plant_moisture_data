#!/usr/bin/env python3

import json
import os
import sys
import csv
import datetime
import urllib.request
import urllib.error

def scale_moisture(raw_value, dry_value, wet_value):
    """
    Scales raw_value (in range [wet_value, dry_value]) to [0, 100].
    wet_value -> 100, dry_value -> 0
    """
    # Clamp values to avoid going beyond the expected range
    if raw_value < wet_value:
        raw_value = wet_value
    if raw_value > dry_value:
        raw_value = dry_value

    # Linear interpolation
    # Example: raw_value == dry_value -> 0, raw_value == wet_value -> 100
    scaled = 100.0 * (dry_value - raw_value) / (dry_value - wet_value)
    return round(scaled, 2)

def fetch_json(url, timeout=5):
    """
    Performs a simple GET request using only standard library urllib.
    Returns parsed JSON (dict) on success, or None on error.
    """
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            # Check HTTP status code (Python 3.8 returns status in response.status)
            if response.status == 200:
                data = response.read()
                try:
                    return json.loads(data)
                except json.JSONDecodeError as e:
                    print(f"JSON parse error: {e}")
                    return None
            else:
                print(f"Error: HTTP {response.status} for URL {url}")
                return None
    except urllib.error.URLError as e:
        print(f"Could not connect to {url}: {e}")
        return None
    except Exception as e:
        print(f"Unexpected error fetching {url}: {e}")
        return None

def main():
    # 1) Load config
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, sys.argv[1])

    if not os.path.exists(config_path):
        print(f"Error: config.json not found at {config_path}")
        sys.exit(1)

    with open(config_path, "r") as f:
        config = json.load(f)

    esp_ip = config["esp_ip"]
    pins = config["pins"]
    csv_path = config["csv_path"]
    dry_value = config.get("dry_value", 2500)
    wet_value = config.get("wet_value", 800)

    # 2) Prepare CSV header
    # We'll store columns in this order: [timestamp, plant1, plant2, ...]
    header = ["timestamp"]
    for p in pins:
        header.append(p["plant_name"] + "_moisture_percent")
        header.append(p["plant_name"] + "_raw_value")

    # 3) Collect readings
    moisture_percent_values = []
    raw_values = []
    for p in pins:
        pin_num = p["pin"]
        plant_name = p["plant_name"]
        url = f"http://{esp_ip}/{pin_num}"

        data = fetch_json(url)
        if data is not None and "reading" in data:
            raw_reading = data["reading"]
            scaled_value = scale_moisture(raw_reading, dry_value, wet_value)
        else:
            # Could not get a valid reading
            scaled_value = -1  # or another sentinel value

        moisture_percent_values.append(scaled_value)
        raw_values.append(raw_reading)

    # 4) Write to CSV
    file_exists = os.path.isfile(csv_path)

    with open(csv_path, mode="a", newline="") as csvfile:
        writer = csv.writer(csvfile)

        # If file didn't exist, write header
        if not file_exists:
            writer.writerow(header)

        # Build the row: [timestamp, moisture_percent_1, raw_value_1, moisture_percent_2, ...]
        timestamp_str = datetime.datetime.now().isoformat()
        values = [e for tuple in zip(moisture_percent_values, raw_values) for e in tuple]
        row = [timestamp_str] + values

        writer.writerow(row)

    print("Data successfully written to CSV.")

if __name__ == "__main__":
    main()
