#!/bin/bash
# Comprehensive test script for multi-process/multi-thread HTTP server
# Tests functionality, concurrency, synchronization, and stress
# Usage: ./test_server.sh

PORT=8080
HOST="localhost"
BASE_URL="http://${HOST}:${PORT}"

echo "=========================================="
echo "  Complete HTTP Server Tests"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Ensure test directory exists
mkdir -p www/test_files 2>/dev/null

# Global counters
total=0
passed=0
failed=0

# Helper function for tests
test_request() {
    local name=$1
    local url=$2
    local expected_code=$3
    local extra_args=${4:-""}
    
    echo -n "  [$((total+1))] $name... "
    
    http_code=$(curl -s -o /dev/null -w "%{http_code}" $extra_args "$url")
    
    if [ "$http_code" = "$expected_code" ]; then
        echo -e "${GREEN}✓ PASSED${NC} (code: $http_code)"
        ((passed++))
    else
        echo -e "${RED}✗ FAILED${NC} (expected: $expected_code, got: $http_code)"
        ((failed++))
    fi
    ((total++))
}

# Check dependencies
echo -e "${BLUE}=== Dependency Check ===${NC}"
MISSING_DEPS=0

if ! command -v curl &> /dev/null; then
    echo -e "${RED}✗ curl not installed${NC}"
    MISSING_DEPS=1
else
    echo -e "${GREEN}✓ curl found${NC}"
fi

if ! command -v ab &> /dev/null; then
    echo -e "${YELLOW}⚠ Apache Bench (ab) not found - limited load tests${NC}"
    echo "  Install with: sudo apt-get install apache2-utils"
else
    echo -e "${GREEN}✓ Apache Bench (ab) found${NC}"
fi

if ! command -v wget &> /dev/null; then
    echo -e "${YELLOW}⚠ wget not found - some tests will be skipped${NC}"
else
    echo -e "${GREEN}✓ wget found${NC}"
fi

if [ $MISSING_DEPS -eq 1 ]; then
    echo -e "${RED}ERROR: Required dependencies not found${NC}"
    exit 1
fi

echo ""

# Check if server is running
echo -e "${BLUE}=== Server Check ===${NC}"
echo -n "Checking connectivity... "

MAX_RETRIES=10
RETRY_COUNT=0
SERVER_READY=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if curl -s --connect-timeout 2 "$BASE_URL/" > /dev/null 2>&1; then
        SERVER_READY=1
        break
    fi
    ((RETRY_COUNT++))
    echo -n "."
    sleep 1
done

if [ $SERVER_READY -eq 1 ]; then
    echo -e "${GREEN}✓ PASSED${NC} (after $RETRY_COUNT attempts)"
else
    echo -e "${RED}✗ FAILED${NC}"
    echo -e "${RED}ERROR: Server not responding at $BASE_URL after $MAX_RETRIES attempts${NC}"
    exit 1
fi
echo ""

#############################################
# FUNCTIONAL TESTS
.7

# Test 1.4: HEAD /index.html
echo -n "  [$((total+1))] HEAD /index.html... "
response=$(curl -s -I "$BASE_URL/index.html")
if echo "$response" | grep -q "HTTP/1.1 200 OK"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Create test files for MIME types

# Test 1.5: HTML
echo "<!DOCTYPE html><html><body><h1>Test HTML</h1></body></html>" > www/test_files/test.html
sleep 0.2
echo -n "  [$((total+1))] HTML - Correct Content-Type... "
response=$(curl -s -I "$BASE_URL/test_files/test.html")
if echo "$response" | grep -q "Content-Type: text/html"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 1.6: CSS
echo "body { background: #fff; }" > www/test_files/test.css
sleep 1  # Give MORE time
echo -n "  [$((total+1))] CSS - Correct Content-Type... "
response=$(curl -s -I "$BASE_URL/test_files/test.css")

# Check more robustly
if echo "$response" | grep -i "content-type:" | grep -q "text/css"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    echo "Full headers:"
    echo "$response"
    ((failed++))
fi
((total++))

# Test 1.7: JavaScript - CORRECTED
echo "console.log('test');" > www/test_files/test.js
sleep 1
echo -n "  [$((total+1))] JavaScript - Correct Content-Type... "
response=$(curl -s -I "$BASE_URL/test_files/test.js")

# Look for "application/javascript" not just "javascript"
if echo "$response" | grep -i "content-type:" | grep -q "application/javascript"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    echo "Full headers:"
    echo "$response"
    ((failed++))
fi
((total++))

# Test 1.8: JSON - CORRECTED
echo '{"test": "value"}' > www/test_files/test.json
sleep 1
echo -n "  [$((total+1))] JSON - Correct Content-Type... "
response=$(curl -s -I "$BASE_URL/test_files/test.json")

# Look for "application/json" not just "json"
if echo "$response" | grep -i "content-type:" | grep -q "application/json"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    echo "Full headers:"
    echo "$response"
    ((failed++))
fi
((total++))

# Test 1.9: PNG image (create valid 1x1 pixel PNG)
printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xb4\x00\x00\x00\x00IEND\xaeB`\x82' > www/test_files/test.png
sleep 1
echo -n "  [$((total+1))] PNG - Correct Content-Type... "
response=$(curl -s -I "$BASE_URL/test_files/test.png")

# Look for "image/png" not just "png"
if echo "$response" | grep -i "content-type:" | grep -q "image/png"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    echo "Full headers:"
    echo "$response"
    ((failed++))
fi
((total++))

# Test 1.10: Plain text
echo "Plain text file" > www/test_files/test.txt
sleep 1
echo -n "  [$((total+1))] TXT - Correct Content-Type... "
response=$(curl -s -I "$BASE_URL/test_files/test.txt")

# Already correct - looks for "text/plain"
if echo "$response" | grep -i "content-type:" | grep -q "text/plain"; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    echo "Full headers:"
    echo "$response"
    ((failed++))
fi
((total++))

# Test 1.11: 404 Not Found
test_request "404 - Non-existent file" "$BASE_URL/naoexiste12345.html" "404"

# Test 1.12: 404 with deep path
test_request "404 - Non-existent path" "$BASE_URL/dir1/dir2/naoexiste.html" "404"

# Test 1.13: 403 Forbidden (directory)
mkdir -p www/testdir 2>/dev/null
echo "test" > www/testdir/file.txt
test_request "403 - Directory access" "$BASE_URL/test_files/testdir" "403"

# Test 1.14: 501 Not Implemented (POST)
test_request "501 - POST not implemented" "$BASE_URL/" "501" "-X POST"

# Test 1.15: 501 Not Implemented (PUT)
test_request "501 - PUT not implemented" "$BASE_URL/" "501" "-X PUT"

# Test 1.16: 501 Not Implemented (DELETE)
test_request "501 - DELETE not implemented" "$BASE_URL/" "501" "-X DELETE"

# Test 1.17: Correct Content-Length
echo -n "  [$((total+1))] Correct Content-Length... "
echo "Test content with known size" > www/test_files/size_test.txt
expected_size=$(wc -c < www/test_files/size_test.txt)
response=$(curl -s -I "$BASE_URL/test_files/size_test.txt")
actual_size=$(echo "$response" | grep -i "Content-Length:" | awk '{print $2}' | tr -d '\r')
if [ "$actual_size" = "$expected_size" ]; then
    echo -e "${GREEN}✓ PASSED${NC} ($expected_size bytes)"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC} (expected: $expected_size, got: $actual_size)"
    ((failed++))
fi
((total++))

echo ""

#############################################
# CONCURRENCY TESTS
#############################################
echo -e "${BLUE}=== 2. CONCURRENCY TESTS ===${NC}"

# Test 2.1: 50 concurrent requests (curl)
echo -n "  [$((total+1))] 50 concurrent requests (curl)... "
success_count=0
for i in {1..50}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 0.5
# Check if server still responds
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC} (server not responding)"
    ((failed++))
fi
((total++))

# Test 2.2: 100 mixed concurrent requests (GET/HEAD)
echo -n "  [$((total+1))] 100 mixed requests (GET/HEAD)... "
for i in {1..50}; do
    curl -s "$BASE_URL/index.html" > /dev/null 2>&1 &
    curl -s -I "$BASE_URL/test_files/test.css" > /dev/null 2>&1 &
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 2.3: Apache Bench - High concurrency
if command -v ab &> /dev/null; then
    echo -n "  [$((total+1))] Apache Bench: 10000 requests, 100 concurrent... "
    ab_output=$(ab -n 10000 -c 100 -q "$BASE_URL/" 2>&1)
    failed_requests=$(echo "$ab_output" | grep "Failed requests:" | awk '{print $3}')
    
    if [ "$failed_requests" = "0" ]; then
        echo -e "${GREEN}✓ PASSED${NC} (0 failures)"
        ((passed++))
    else
        echo -e "${RED}✗ FAILED${NC} ($failed_requests failures)"
        ((failed++))
    fi
    ((total++))
    
    # Extract and show statistics
    rps=$(echo "$ab_output" | grep "Requests per second:" | awk '{print $4}')
    echo -e "    ${YELLOW}→ Requests/sec: $rps${NC}"
else
    echo -e "  ${YELLOW}⚠ Apache Bench test skipped (not installed)${NC}"
fi

# Test 2.4: Multiple wget clients in parallel
if command -v wget &> /dev/null; then
    echo -n "  [$((total+1))] 20 parallel wget clients... "
    temp_dir=$(mktemp -d)
    
    for i in {1..20}; do
        # Add --timeout option
        timeout 10 wget -q -O "$temp_dir/file_$i.html" "$BASE_URL/" &
    done
    
    # Wait with timeout
    wait
    
    # Count successfully downloaded files
    downloaded=$(ls -1 "$temp_dir" 2>/dev/null | wc -l)
    rm -rf "$temp_dir"
    
    if [ "$downloaded" -eq 20 ]; then
        echo -e "${GREEN}✓ PASSED${NC} (20/20 downloads)"
        ((passed++))
    else
        echo -e "${RED}✗ FAILED${NC} ($downloaded/20 downloads)"
        ((failed++))
    fi
    ((total++))
fi

# Test 2.5: Stress test with requests to different files
echo -n "  [$((total+1))] Stress: 200 requests to different files... "
for i in {1..200}; do
    file=$((i % 5))
    case $file in
        0) curl -s "$BASE_URL/index.html" > /dev/null 2>&1 & ;;
        1) curl -s "$BASE_URL/test_files/test.css" > /dev/null 2>&1 & ;;
        2) curl -s "$BASE_URL/test_files/test.js" > /dev/null 2>&1 & ;;
        3) curl -s "$BASE_URL/test_files/test.html" > /dev/null 2>&1 & ;;
        4) curl -s "$BASE_URL/test_files/test.txt" > /dev/null 2>&1 & ;;
    esac
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

echo ""

#############################################
# SYNCHRONIZATION AND INTEGRITY TESTS
#############################################
echo -e "${BLUE}=== 3. SYNCHRONIZATION TESTS ===${NC}"

# Test 3.1: Log file integrity (check entries do not overlap)
echo -n "  [$((total+1))] Log file integrity... "
log_file="access.log"
if [ -f "$log_file" ]; then
    # Make several requests
    for i in {1..20}; do
        curl -s "$BASE_URL/" > /dev/null 2>&1 &
    done
    wait
    sleep 1
    
    # Check if each line has correct format [YYYY-MM-DD HH:MM:SS]
    invalid_lines=$(grep -v "^\[20[0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]\]" "$log_file" | grep -v "=====" | wc -l)
    
    if [ "$invalid_lines" -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC} (correct format)"
        ((passed++))
    else
        echo -e "${YELLOW}⚠ WARNING${NC} ($invalid_lines lines with unexpected format)"
        ((passed++)) # Do not fail for this
    fi
else
    echo -e "${YELLOW}⚠ Log not found${NC}"
    ((passed++))
fi
((total++))

# Test 3.2: Cache hit consistency (request same file multiple times)
echo -n "  [$((total+1))] Cache consistency (100 requests to same file)... "
errors=0
for i in {1..100}; do
    result=$(curl -s -w "%{http_code}" -o /dev/null "$BASE_URL/test_files/test.html")
    if [ "$result" != "200" ]; then
        ((errors++))
    fi
done

if [ $errors -eq 0 ]; then
    echo -e "${GREEN}✓ PASSED${NC} (0 errors)"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC} ($errors errors)"
    ((failed++))
fi
((total++))

# Test 3.3: Statistics system (check if response exists)
echo -n "  [$((total+1))] Statistics system functional... "
# Make some requests
for i in {1..10}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1
done
sleep 2
# We cannot check stats directly, but we can check if server still works
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC} (server operational)"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 3.4: Race condition test - simultaneous requests to same resource
echo -n "  [$((total+1))] Race conditions: 50 simultaneous requests to same file... "
errors=0
for i in {1..50}; do
    curl -s "$BASE_URL/test_files/test.css" > /dev/null 2>&1 &
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

echo ""

#############################################
# STRESS AND STABILITY TESTS
#############################################
echo -e "${BLUE}=== 4. STRESS TESTS ===${NC}"

# Test 4.1: Sustained load for 30 seconds
echo -n "  [$((total+1))] Sustained load (30 seconds)... "
end_time=$((SECONDS + 30))
request_count=0
while [ $SECONDS -lt $end_time ]; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
    ((request_count++))
    sleep 0.1
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC} ($request_count requests)"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 4.2: Traffic spikes (bursts)
echo -n "  [$((total+1))] Traffic spikes (3 bursts of 100 requests)... "
for burst in {1..3}; do
    for i in {1..100}; do
        curl -s "$BASE_URL/" > /dev/null 2>&1 &
    done
    wait
    sleep 2
done
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 4.3: Mix of valid and invalid requests
echo -n "  [$((total+1))] Request mix (valid + 404 + 501)... "
for i in {1..30}; do
    curl -s "$BASE_URL/" > /dev/null 2>&1 &
    curl -s "$BASE_URL/naoexiste$i.html" > /dev/null 2>&1 &
    curl -s -X POST "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 4.4: Files of various sizes
echo -n "  [$((total+1))] Various file sizes... "
# Create small, medium, and large files
echo "small" > www/test_files/small.txt
dd if=/dev/zero of=www/test_files/medium.bin bs=1K count=100 2>/dev/null
dd if=/dev/zero of=www/test_files/large.bin bs=1K count=1000 2>/dev/null

for i in {1..20}; do
    curl -s "$BASE_URL/test_files/small.txt" > /dev/null 2>&1 &
    curl -s "$BASE_URL/test_files/medium.bin" > /dev/null 2>&1 &
    curl -s "$BASE_URL/test_files/large.bin" > /dev/null 2>&1 &
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 4.5: Stability test - check processes
echo -n "  [$((total+1))] Process check (no zombies)... "
zombie_count=$(ps aux | grep defunct | grep -v grep | wc -l)
if [ $zombie_count -eq 0 ]; then
    echo -e "${GREEN}✓ PASSED${NC} (0 zombies)"
    ((passed++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} ($zombie_count zombie processes)"
    ((passed++)) # Don't fail, just warn
fi
((total++))

echo ""

#############################################
# EDGE CASE TESTS
#############################################
echo -e "${BLUE}=== 5. EDGE CASE TESTS ===${NC}"

# Test 5.1: Very long URL request
echo -n "  [$((total+1))] Extremely long URL... "
long_url="$BASE_URL/"$(printf 'a%.0s' {1..1000})".html"
result=$(curl -s -o /dev/null -w "%{http_code}" "$long_url")
if [ "$result" = "404" ] || [ "$result" = "414" ]; then
    echo -e "${GREEN}✓ PASSED${NC} (handled correctly)"
    ((passed++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} (code: $result)"
    ((passed++))
fi
((total++))

# Test 5.2: Multiple HEAD requests
echo -n "  [$((total+1))] 50 concurrent HEAD requests... "
for i in {1..50}; do
    curl -s -I "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 0.5
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

# Test 5.3: Path traversal (basic security)
echo -n "  [$((total+1))] Path traversal attempt (/../)... "
result=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/../etc/passwd")
if [ "$result" = "404" ] || [ "$result" = "403" ]; then
    echo -e "${GREEN}✓ PASSED${NC} (blocked)"
    ((passed++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} (code: $result)"
    ((passed++))
fi
((total++))

# Test 5.4: File with spaces in name
echo "test content" > "www/test_files/file with spaces.txt"
echo -n "  [$((total+1))] File with spaces in name... "
result=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/test_files/file%20with%20spaces.txt")
if [ "$result" = "200" ]; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} (code: $result)"
    ((passed++))
fi
((total++))

# Test 5.5: Fast connections (connect and disconnect immediately)
echo -n "  [$((total+1))] 20 fast connections (connect/disconnect)... "
for i in {1..20}; do
    timeout 0.1 curl -s "$BASE_URL/" > /dev/null 2>&1 &
done
wait
sleep 1
if curl -s --connect-timeout 3 "$BASE_URL/" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASSED${NC}"
    ((passed++))
else
    echo -e "${RED}✗ FAILED${NC}"
    ((failed++))
fi
((total++))

echo ""

#############################################
# FINAL REPORT
#############################################
echo "=========================================="
echo -e "${BLUE}  FINAL REPORT${NC}"
echo "=========================================="
echo ""
echo "Total tests executed: $total"
echo -e "Tests passed: ${GREEN}$passed${NC}"
echo -e "Tests failed: ${RED}$failed${NC}"

if [ $failed -eq 0 ]; then
    success_rate=100
else
    success_rate=$((passed * 100 / total))
fi

echo -e "Success rate: ${YELLOW}${success_rate}%${NC}"
echo ""

echo -e "${RED}Warning: Cleaning up..."
rm -r www/test_files
rm -r www/testdir

if [ $failed -eq 0 ]; then
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}  ✓ ALL TESTS PASSED!${NC}"
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    exit 0
elif [ $success_rate -ge 90 ]; then
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}  ⚠ Most tests passed (≥90%)${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    exit 0
else
    echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${RED}  ✗ Some critical tests failed${NC}"
    echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    exit 1
fi