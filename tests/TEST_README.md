# resolve_host() Test Suite

This directory contains comprehensive tests for the `resolve_host()` function in `pacparser.c`.

## Test Files

### 1. test_resolve_host.c
Basic functional testing with various hostnames and configurations.

**Compile & Run:**
```bash
gcc -g -Wall -o test_resolve_host test_resolve_host.c
./test_resolve_host
```

**Tests:**
- IPv4/IPv6 resolution
- localhost resolution
- Invalid/empty hostnames
- Numeric IP addresses
- max_results limiting

### 2. test_multi_ip.c
Specific tests for multiple IP address handling.

**Compile & Run:**
```bash
gcc -g -Wall -o test_multi_ip test_multi_ip.c
./test_multi_ip
```

**Tests:**
- Multiple IP resolution with semicolon separation
- Format validation (no leading/trailing/double semicolons)
- max_results limiting with multiple IPs

### 3. test_buffer_safety.c
Memory safety and buffer overflow detection.

**Compile & Run:**
```bash
gcc -g -Wall -o test_buffer_safety test_buffer_safety.c
./test_buffer_safety
```

**Tests:**
- Canary-based overflow detection
- Edge case: max_results = 0
- Long IPv6 addresses
- Buffer utilization metrics

### 4. test_edge_cases.c
Comprehensive edge case and boundary condition testing.

**Compile & Run:**
```bash
gcc -g -Wall -o test_edge_cases test_edge_cases.c
./test_edge_cases
```

**Tests:**
- max_results boundaries (0, 1, MAX)
- IPv4-only, IPv6-only, and mixed resolution
- Numeric IPs (including IPv4-mapped IPv6)
- Null termination verification
- Write-beyond-buffer detection

## Running All Tests

```bash
# Compile all
for test in test_resolve_host test_multi_ip test_buffer_safety test_edge_cases; do
    gcc -g -Wall -o $test ${test}.c
done

# Run all
for test in test_resolve_host test_multi_ip test_buffer_safety test_edge_cases; do
    echo "=== Running $test ==="
    ./$test
    echo
done
```

## Clean Up

```bash
rm -f test_resolve_host test_multi_ip test_buffer_safety test_edge_cases
```

## Test Coverage

These tests verify:
- ✅ Single and multiple IP resolution
- ✅ IPv4 and IPv6 handling
- ✅ Semicolon separation formatting
- ✅ Buffer safety (no overruns)
- ✅ Null termination
- ✅ Error handling for invalid hostnames
- ✅ max_results limiting
- ✅ Edge cases (empty results, max boundaries)
- ✅ All address families (AF_INET, AF_INET6, AF_UNSPEC)

## Notes

- Some tests (google.com, IPv6) require network connectivity
- IPv6 tests may fail on IPv4-only systems (this is expected)
- The tests use the same `resolve_host()` implementation from `pacparser.c`
- All tests include validation to detect memory corruption and format errors
