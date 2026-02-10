# PAC File Tester - Demo Guide

## Quick Start Demo

### Step 1: Open the Application

The web server is running! Open your browser and navigate to:
```
http://localhost:8000
```

### Step 2: Try These Test Scenarios

#### Test Scenario 1: Plain Hostname
1. **PAC File**: Use the content from `sample.pac` or paste:
   ```javascript
   function FindProxyForURL(url, host) {
       if (isPlainHostName(host)) {
           return "DIRECT";
       }
       return "PROXY proxy.example.com:8080";
   }
   ```

2. **Test URL**: `http://localhost`

3. **Expected Result**: `DIRECT`

---

#### Test Scenario 2: Internal Network Check
1. **PAC File**: Use `sample.pac` (already created)

2. **Configuration**:
   - **Test URL**: `https://internal.company.com`
   - **Client IP**: `192.168.1.100`
   - **DNS Mappings**:
     - `internal.company.com` → `192.168.1.50`

3. **Expected Result**: `DIRECT` (since both client and destination are internal)

---

#### Test Scenario 3: External Site via Proxy
1. **PAC File**: Use `sample.pac`

2. **Configuration**:
   - **Test URL**: `https://www.google.com`
   - **Client IP**: `192.168.1.100`
   - **DNS Mappings**:
     - `www.google.com` → `142.250.185.46`

3. **Expected Result**: `PROXY proxy.corporate.com:3128; DIRECT`

---

#### Test Scenario 4: Domain-Based Routing
1. **PAC File**: Paste:
   ```javascript
   function FindProxyForURL(url, host) {
       // Go direct for plain hostnames
       if (isPlainHostName(host) || dnsDomainIs(host, '.local')) {
           return "DIRECT";
       }

       // Check if host is resolvable (requires DNS mapping)
       if (dnsDomainIs(host, '.google.com') && isResolvable(host)) {
           return "PROXY google-proxy:8080";
       }

       // Default proxy
       return "PROXY proxy.company.com:3128; DIRECT";
   }
   ```

2. **Configuration**:
   - **Test URL**: `https://www.google.com`
   - **DNS Mappings**:
     - `www.google.com` → `142.250.185.46`

3. **Expected Result**: `PROXY google-proxy:8080`

---

#### Test Scenario 5: IP Range Matching
1. **PAC File**: Paste:
   ```javascript
   function FindProxyForURL(url, host) {
       // Check if client is in corporate network
       if (isInNet(myIpAddress(), "10.10.0.0", "255.255.0.0")) {
           return "PROXY internal-proxy:3128";
       }

       // Check if destination is in local network
       var resolved = dnsResolve(host);
       if (resolved && isInNet(resolved, "192.168.0.0", "255.255.0.0")) {
           return "DIRECT";
       }

       return "PROXY external-proxy:8080";
   }
   ```

2. **Configuration**:
   - **Test URL**: `http://intranet.local`
   - **Client IP**: `10.10.100.50`
   - **DNS Mappings**:
     - `intranet.local` → `192.168.10.5`

3. **Expected Result**: `PROXY internal-proxy:3128`

---

#### Test Scenario 6: Time-Based Routing
1. **PAC File**: Paste:
   ```javascript
   function FindProxyForURL(url, host) {
       // Use fast proxy during business hours (9 AM - 5 PM)
       if (timeRange(9, 17)) {
           return "PROXY fast-proxy:8080";
       }

       // Use slower proxy outside business hours
       return "PROXY slow-proxy:8080";
   }
   ```

2. **Test URL**: `https://www.example.com`

3. **Expected Result**: Depends on current time
   - During 9 AM - 5 PM: `PROXY fast-proxy:8080`
   - Outside those hours: `PROXY slow-proxy:8080`

---

## Testing with Real PAC Files

### Example: Google PAC File Pattern
```javascript
function FindProxyForURL(url, host) {
    // Bypass proxy for local addresses
    if (shExpMatch(host, "*.local") ||
        shExpMatch(host, "127.*") ||
        shExpMatch(host, "localhost")) {
        return "DIRECT";
    }

    // Use specific proxy for certain domains
    if (shExpMatch(host, "*.example.com")) {
        return "PROXY proxy1.example.com:8080; PROXY proxy2.example.com:8080; DIRECT";
    }

    // Default
    return "PROXY default-proxy.example.com:8080; DIRECT";
}
```

**Test with**:
- URL: `http://internal.example.com`
- Expected: `PROXY proxy1.example.com:8080; PROXY proxy2.example.com:8080; DIRECT`

---

## Debugging Tips

1. **Check the Debug Information**: After testing, scroll down to see:
   - Which DNS lookups were performed
   - Whether `myIpAddress()` was called
   - The extracted hostname from the URL

2. **Add DNS Mappings Progressively**: Start with no DNS mappings and add them as needed based on errors

3. **Test Multiple URLs**: Use the same PAC file with different URLs to verify routing logic

4. **Common Issues**:
   - If `dnsResolve(host)` returns null, add a DNS mapping for that host
   - If `myIpAddress()` returns unexpected value, set the Client IP field
   - Case sensitivity matters for domain names

## Next Steps

- Try uploading your own PAC file
- Test against your actual proxy configuration
- Share the web app with your team for testing PAC files
- Deploy to a static hosting service (GitHub Pages, Netlify, etc.)

## Feedback

If you find any issues or have suggestions, please report them on the [Pacparser GitHub Issues](https://github.com/manugarg/pacparser/issues).
