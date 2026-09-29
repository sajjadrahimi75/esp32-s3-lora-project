
import serial
from pathlib import Path

# Serial port settings
PORT = "COM10"
BAUD = 115200

# Folder where photos will be saved
SAVE_FOLDER = Path(r"C:\Users\SAJJAD\Downloads\esp\photo test")
SAVE_FOLDER.mkdir(parents=True, exist_ok=True)


def get_next_filename():
    number = 1

    while (SAVE_FOLDER / f"image{number}.jpg").exists():
        number += 1

    return SAVE_FOLDER / f"image{number}.jpg"


def receive_photos():
    print(f"Opening {PORT} at {BAUD} baud...")
    print(f"Saving photos to: {SAVE_FOLDER}")
    print("Waiting for JPEG images...")
    print("Press Ctrl+C to stop.\n")

    with serial.Serial(PORT, BAUD, timeout=None) as ser:

        while True:
            line = ser.readline().decode("ascii", errors="ignore").strip()

            # Wait for the image header from ESP32
            if not line.startswith("IMAGE_HEX_START:"):
                if line:
                    print(line[:150])
                continue

            try:
                expected_size = int(line.split(":", 1)[1])
            except ValueError:
                print("Invalid image header")
                continue

            print(f"\nReceiving image: {expected_size} bytes")

            # Read HEX-encoded JPEG data
            hex_data = ser.readline().decode("ascii", errors="ignore").strip()

            try:
                photo = bytes.fromhex(hex_data)
            except ValueError:
                print("ERROR: Invalid HEX data")
                continue

            # Check received size
            if len(photo) != expected_size:
                print(
                    f"ERROR: Incomplete image. "
                    f"Received {len(photo)} / {expected_size} bytes"
                )
                continue

            # Check JPEG start and end markers
            if not photo.startswith(b"\xFF\xD8"):
                print("ERROR: JPEG start marker missing")
                continue

            print("PC JPEG first:", photo[:4].hex(" ").upper())
            print("PC JPEG last :", photo[-4:].hex(" ").upper())
            print("PC image size:", len(photo))

            if not photo.endswith(b"\xFF\xD9"):
                print("ERROR: JPEG end marker missing")
                continue

            # Save the JPEG
            filename = get_next_filename()
            filename.write_bytes(photo)

            print(f"PHOTO SAVED: {filename}")
            print(f"File size: {len(photo)} bytes")
            print("Waiting for next photo...\n")


if __name__ == "__main__":
    try:
        receive_photos()

    except KeyboardInterrupt:
        print("\nReceiver stopped.")

    except serial.SerialException as e:
        print(f"\nSerial port error: {e}")
        print("Check COM10 and close the ESP-IDF Serial Monitor.")
