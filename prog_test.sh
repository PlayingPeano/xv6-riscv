#!/usr/bin/env bash

set -e

IMG="ext2.img"
MNT="/mnt"
TEST_FILE="test_file.txt"
PROG="../prog"
TEST_DIR="ext2_test"
TEST_CONTENT="Hello, EXT2 Test!"

mkdir -p "$TEST_DIR" && cd "$TEST_DIR"

truncate -s 128M "$IMG"
mkfs.ext2 "$IMG" >/dev/null
sudo mount -t ext2 "$IMG" "$MNT"

echo "$TEST_CONTENT" | sudo tee "$MNT/$TEST_FILE" >/dev/null

INODE=$(stat -c %i "$MNT/$TEST_FILE")
SHA_E=$(sha512sum "$MNT/$TEST_FILE" | cut -d' ' -f1)

sudo umount "$MNT"

SHA_A=$("$PROG" "$IMG" "$INODE" | sha512sum | cut -d' ' -f1)

echo -n "simple test with '$TEST_CONTENT' content: "
if [[ "$SHA_A" == "$SHA_E" ]]; then
    echo "OK"
else
    echo "FAIL"
fi
echo "  expected: $SHA_E"
echo "  actual:   $SHA_A"
