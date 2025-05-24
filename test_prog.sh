#!/usr/bin/env bash

set -e

IMG="ext2.img"
MNT="/mnt"
TESTS_DIR="ext2_tests"
FILE_NAME="prog"
PROG="../$FILE_NAME"
HELLO_FILE="hello.txt"
SPARSE_FILE="sparse.bin"
BIG_FILE="big.bin"

cleanup() {
    echo "cleaning..."
    if mountpoint -q "$MNT"; then
        sudo umount "$MNT" || true
    fi
    rm -rfv "../$TESTS_DIR"
    rm -fv "$PROG"
}
trap cleanup EXIT INT TERM

echo "compiling..."
echo "compiled"
echo
gcc "$FILE_NAME.c" -o "$FILE_NAME"

mkdir -p "$TESTS_DIR" && cd "$TESTS_DIR"

truncate -s 128M "$IMG"
mkfs.ext2 "$IMG"
sudo mount -t ext2 "$IMG" "$MNT"

echo "Hello, EXT2 Test!" | sudo tee "$MNT/$HELLO_FILE" > "/dev/null"
HELLO_INODE=$(stat -c %i "$MNT/$HELLO_FILE")
HELLO_SHA_E=$(sha512sum "$MNT/$HELLO_FILE" | cut -d' ' -f1)

sudo dd if=/dev/zero of="$MNT/$SPARSE_FILE" bs=1 count=128K seek=0 conv=notrunc status=none
sudo dd if=/dev/zero of="$MNT/$SPARSE_FILE" bs=1 count=64K seek=2M conv=notrunc status=none
sudo dd if=/dev/zero of="$MNT/$SPARSE_FILE" bs=1 count=256K seek=3M conv=notrunc status=none
SPARSE_INODE=$(stat -c %i "$MNT/$SPARSE_FILE")
SPARSE_SHA_E=$(sha512sum "$MNT/$SPARSE_FILE" | cut -d' ' -f1)

sudo dd if=/dev/zero of="$MNT/$BIG_FILE" bs=1 count=4M seek=0 conv=notrunc status=none
BIG_INODE=$(stat -c %i "$MNT/$BIG_FILE")
BIG_SHA_E=$(sha512sum "$MNT/$BIG_FILE" | cut -d' ' -f1)

sudo umount "$MNT"

echo "[1] Testing on image file $IMG:"
echo
for test in HELLO SPARSE BIG; do
    eval "INODE=\$${test}_INODE"
    eval "SHA_E=\$${test}_SHA_E"
    SHA_A=$("$PROG" "$IMG" "$INODE" | sha512sum | cut -d' ' -f1)
    test_name=$(echo "$test" | tr '[:upper:]' '[:lower:]')
    echo -n "$test_name test: "
    if [[ "$SHA_A" == "$SHA_E" ]]; then
        echo "OK"
    else
        echo "FAIL"
    fi
    echo "  expected: $SHA_E"
    echo "  actual:   $SHA_A"
    echo
done

echo "[2] Testing on loop device:"
echo
LOOP=$(sudo losetup -f)
sudo losetup "$LOOP" "$IMG"

for test in HELLO SPARSE BIG; do
    eval "INODE=\$${test}_INODE"
    eval "SHA_E=\$${test}_SHA_E"
    SHA_A=$(sudo "$PROG" "$LOOP" "$INODE" | sha512sum | cut -d' ' -f1)
    test_name=$(echo "$test" | tr '[:upper:]' '[:lower:]')
    echo -n "$test_name test: "
    if [[ "$SHA_A" == "$SHA_E" ]]; then
        echo "OK"
    else
        echo "FAIL"
    fi
    echo "  expected: $SHA_E"
    echo "  actual:   $SHA_A"
    echo
done

sudo losetup -d "$LOOP"
