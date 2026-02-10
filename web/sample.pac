// Sample PAC file for testing
// This file demonstrates common PAC file patterns

function FindProxyForURL(url, host) {
    // Direct access for localhost
    if (isPlainHostName(host) ||
        dnsDomainIs(host, ".localhost") ||
        isInNet(host, "127.0.0.0", "255.0.0.0")) {
        return "DIRECT";
    }

    // Direct access for internal networks
    if (isInNet(myIpAddress(), "10.0.0.0", "255.0.0.0") ||
        isInNet(myIpAddress(), "172.16.0.0", "255.240.0.0") ||
        isInNet(myIpAddress(), "192.168.0.0", "255.255.0.0")) {

        // Check if destination is also internal
        if (isInNet(dnsResolve(host), "10.0.0.0", "255.0.0.0") ||
            isInNet(dnsResolve(host), "172.16.0.0", "255.240.0.0") ||
            isInNet(dnsResolve(host), "192.168.0.0", "255.255.0.0")) {
            return "DIRECT";
        }
    }

    // Use proxy for external sites
    if (dnsDomainIs(host, ".example.com")) {
        return "PROXY proxy.example.com:8080; DIRECT";
    }

    // Default: use main proxy
    return "PROXY proxy.corporate.com:3128; DIRECT";
}
