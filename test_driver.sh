#!/bin/bash

# Test script for memory driver - Lab 5
# This script tests both Part 1 and Part 2 functionality

echo "======================================"
echo "Memory Driver Test Script - Lab 5"
echo "======================================"

# Part 1 Tests - Device file creation
echo ""
echo "PART 1: Testing device file creation"
echo "--------------------------------------"

# Check if driver is already loaded
if lsmod | grep -q "memory"; then
    echo "Driver already loaded. Removing..."
    sudo rmmod memory
    sleep 1
fi

# Check if /dev/memory exists
if [ -e /dev/memory ]; then
    echo "Warning: /dev/memory already exists"
    ls -l /dev/memory
else
    echo "/dev/memory does not exist (good)"
fi

# Load the driver
echo ""
echo "Loading driver..."
sudo insmod memory.ko

# Check if driver loaded
if lsmod | grep -q "memory"; then
    echo "✓ Driver loaded successfully"
else
    echo "✗ Driver failed to load"
    exit 1
fi

# Check if /dev/memory was created
echo ""
echo "Checking for /dev/memory:"
if [ -e /dev/memory ]; then
    echo "✓ /dev/memory created successfully"
    ls -l /dev/memory
    MAJOR=$(ls -l /dev/memory | awk '{print $5}' | tr -d ',')
    echo "  Major number: $MAJOR"
else
    echo "✗ /dev/memory not found"
    exit 1
fi

# Check dmesg for init messages
echo ""
echo "Recent kernel messages:"
sudo dmesg | tail -10 | grep "memory:" || echo "No memory driver messages found"

# Part 2 Tests - 5-byte storage
echo ""
echo "PART 2: Testing 5-byte storage"
echo "--------------------------------------"

# Set permissions
echo "Setting permissions..."
sudo chmod 666 /dev/memory
echo "✓ Permissions set: $(ls -l /dev/memory | awk '{print $1}')"

# Test 1: Write exactly 5 bytes
echo ""
echo "Test 1: Writing '98765' (5 bytes)"
echo -n "98765" > /dev/memory
if [ $? -eq 0 ]; then
    echo "✓ Write successful"
else
    echo "✗ Write failed"
fi

# Read back the data
echo "Reading data back:"
RESULT=$(dd if=/dev/memory bs=5 count=1 2>/dev/null)
echo "  Result: '$RESULT'"
if [ "$RESULT" = "98765" ]; then
    echo "✓ Read successful - data matches"
else
    echo "✗ Read failed - expected '98765', got '$RESULT'"
fi

# Test 2: Write more than 5 bytes (should truncate)
echo ""
echo "Test 2: Writing '1234567890' (10 bytes - should truncate to 5)"
echo -n "1234567890" > /dev/memory
RESULT=$(dd if=/dev/memory bs=5 count=1 2>/dev/null)
echo "  Result: '$RESULT'"
if [ "$RESULT" = "12345" ]; then
    echo "✓ Truncation working correctly"
else
    echo "✗ Truncation failed - expected '12345', got '$RESULT'"
fi

# Test 3: Write less than 5 bytes
echo ""
echo "Test 3: Writing 'AB' (2 bytes)"
echo -n "AB" > /dev/memory
RESULT=$(dd if=/dev/memory bs=5 count=1 2>/dev/null)
echo "  Result: '$RESULT'"
if [ "$RESULT" = "AB" ]; then
    echo "✓ Partial write working correctly"
else
    echo "✗ Partial write failed - expected 'AB', got '$RESULT'"
fi

# Test 4: Multiple writes (should overwrite)
echo ""
echo "Test 4: Multiple writes - should overwrite"
echo -n "FIRST" > /dev/memory
echo -n "SECOND" > /dev/memory
RESULT=$(dd if=/dev/memory bs=5 count=1 2>/dev/null)
echo "  Result: '$RESULT'"
if [ "$RESULT" = "SECOND" ]; then
    echo "✓ Overwrite working correctly"
else
    echo "✗ Overwrite failed - expected 'SECOND', got '$RESULT'"
fi

# Check kernel messages for debugging info
echo ""
echo "Kernel debug messages from operations:"
sudo dmesg | tail -20 | grep "memory:" || echo "No recent memory driver messages"

# Clean up
echo ""
echo "Cleaning up..."
sudo rmmod memory
sleep 1

if [ -e /dev/memory ]; then
    echo "✗ /dev/memory still exists after driver removal"
else
    echo "✓ /dev/memory removed automatically (Part 1 working)"
fi

echo ""
echo "======================================"
echo "All tests completed"
echo "======================================"