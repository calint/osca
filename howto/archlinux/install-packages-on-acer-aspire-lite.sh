#!/bin/bash
set -e
cd $(dirname "$0")

# Package list file
package_file="packages-acer-aspire-lite.txt"

# Check if the package list file exists
if [[ ! -f "$package_file" ]]; then
  echo "Error: Package list file '$package_file' not found."
  exit 1
fi

# Read packages from the file into an array
packages=()
while IFS= read -r line; do
  # Skip empty lines and comments
  if [[ -n "$line" && ! "$line" =~ ^# ]]; then
    packages+=("$line")
  fi
done < "$package_file"

# Install packages using yay
yay -S --needed --noconfirm "${packages[@]}"

echo "Packages installed successfully."