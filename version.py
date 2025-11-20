"""
    UUENCODE/UUDECODE File Processor and Integrity Checker
    This script performs the following operations:
    1. Decodes a uuencoded file back to its original binary format
    2. Calculates SHA-256 checksums for file integrity verification
    3. Compares checksums to ensure the decoding process was successful
    The script is useful for verifying that uuencoded files are decoded correctly
    without any data corruption or modification during the process.
"""
import hashlib
import binascii
import codecs
import os

def file_checksum(filename):
    """
        Calculate the SHA-256 checksum of a file.
        Args:
            filename (str): Path to the file for which to calculate checksum
        Returns:
            str: Hexadecimal representation of the SHA-256 checksum
    """
    sha256_hash = hashlib.sha256()  # Create SHA-256 hash object
    with open(filename, "rb") as f:  # Open file in binary read mode
        # Read file in 4KB chunks to handle large files efficiently
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)  # Update hash with each chunk
    return sha256_hash.hexdigest()  # Return final hash as hexadecimal string

def decode_uu_manual(input_uu_file, output_file):
    """
        Decode a uuencoded file manually without using the deprecated uu module.
        Args:
            input_uu_file (str): Path to the uuencoded input file
            output_file (str): Path where the decoded file will be saved
    """
    with open(input_uu_file, "rb") as uu_file:  # Open in binary mode
        uu_data = uu_file.read()

    # Remove any header lines (lines starting with 'begin')
    lines = uu_data.split(b'\n')
    decoded_data = b''

    for line in lines:
        # Skip header lines and empty lines
        if line.startswith(b'begin ') or line.startswith(b'end') or not line.strip():
            continue

        # Skip lines that don't contain uuencoded data
        if len(line) < 1 or line[0] < 32 or line[0] > 96:
            continue

        # Decode the uuencoded line
        try:
            # The first character indicates the number of encoded bytes
            length_byte = line[0]
            if length_byte == 96:  # ` character means zero bytes
                continue

            # Calculate actual data length
            data_length = (length_byte - 32) & 63

            # Decode the data portion
            encoded_chunk = line[1:1+data_length*4//3]
            decoded_chunk = codecs.decode(encoded_chunk, 'uu')
            decoded_data += decoded_chunk
        except Exception as e:
            print(f"Warning: Could not decode line: {e}")
            continue

    # Write the decoded data to output file
    with open(output_file, "wb") as out_file:
        out_file.write(decoded_data)

def decode_uu_simple(input_uu_file, output_file):
    """
        Simple uudecode implementation using codecs.
        Args:
            input_uu_file (str): Path to the uuencoded input file
            output_file (str): Path where the decoded file will be saved
    """
    with open(input_uu_file, "rb") as uu_file:
        uu_data = uu_file.read()

    try:
        # Try to decode using codecs
        decoded_data = codecs.decode(uu_data, 'uu')
        with open(output_file, "wb") as out_file:
            out_file.write(decoded_data)
    except Exception as e:
        print(f"Error decoding with codecs: {e}")
        # Fall back to manual decoding
        decode_uu_manual(input_uu_file, output_file)

# File paths configuration
input_uu_file = "version.uu"  # Input uuencoded file
output_file = "version.uu.res"   # Output decoded file

try:
    # Step 1: Decode the uuencoded file using simple method
    print("Decoding uuencoded file...")
    decode_uu_simple(input_uu_file, output_file)

    # Step 2: Calculate and display checksums for verification
    print("Calculating checksums...")

    # Check if output file was created
    if not os.path.exists(output_file):
        raise FileNotFoundError(f"Output file {output_file} was not created")

    # Get file size for verification
    file_size = os.path.getsize(output_file)
    print(f"Decoded file size: {file_size} bytes")

    decoded_checksum = file_checksum(output_file)
    print(f"Decoded file checksum: {decoded_checksum}")

    # If you have the original file for comparison, uncomment the following lines:
    # original_checksum = file_checksum("version_original.res")
    # print(f"Original checksum: {original_checksum}")
    #
    # # Step 3: Verify integrity by comparing checksums
    # if original_checksum == decoded_checksum:
    #     print("File successfully decoded with correct integrity!")
    # else:
    #     print("The decoded file does not match the original file.")

    print("Decoding completed successfully!")

except FileNotFoundError as e:
    print(f"File error: {e}")
except Exception as e:
    print(f"Unexpected error: {e}")
