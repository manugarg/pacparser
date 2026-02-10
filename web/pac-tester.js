// PAC File Tester - Client-side PAC file evaluator
// Copyright (C) 2025 Manu Garg (based on pacparser library)

// Global state
let dnsMappingsMap = {};

// PAC_UTILS_JS is loaded from pac_utils.js, which is auto-generated from
// src/pac_utils.h via: make -C src pac_utils_js
//
// It is kept as a source string (not executed at load time) so that the
// eval'd sandbox below can define its own dnsResolve() / myIpAddress()
// in the same scope as the utility functions — ensuring DNS mocking
// works correctly via lexical scoping.

// Add a new DNS mapping row
function addDnsMapping() {
    const container = document.getElementById('dnsMappings');
    const mappingDiv = document.createElement('div');
    mappingDiv.className = 'dns-mapping';
    mappingDiv.innerHTML = `
        <input type="text" placeholder="hostname (e.g., example.com)" class="dns-host">
        <span class="arrow">→</span>
        <input type="text" placeholder="IP address" class="dns-ip">
        <button class="btn-remove" onclick="removeDnsMapping(this)">✕</button>
    `;
    container.appendChild(mappingDiv);
}

// Remove a DNS mapping row
function removeDnsMapping(button) {
    const mapping = button.parentElement;
    mapping.remove();
}

// Collect DNS mappings from UI
function collectDnsMappings() {
    const mappings = {};
    const mappingElements = document.querySelectorAll('.dns-mapping');

    mappingElements.forEach(el => {
        const host = el.querySelector('.dns-host').value.trim();
        const ip = el.querySelector('.dns-ip').value.trim();
        if (host && ip) {
            mappings[host] = ip;
        }
    });

    return mappings;
}

// File upload handler
document.addEventListener('DOMContentLoaded', () => {
    const fileInput = document.getElementById('fileInput');
    const fileName = document.getElementById('fileName');
    const pacFile = document.getElementById('pacFile');

    fileInput.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (file) {
            fileName.textContent = file.name;
            const reader = new FileReader();
            reader.onload = (event) => {
                pacFile.value = event.target.result;
            };
            reader.readAsText(file);
        }
    });
});

// Extract hostname from URL
function extractHost(url) {
    try {
        const urlObj = new URL(url);
        return urlObj.hostname;
    } catch (e) {
        // Fallback for invalid URLs
        const match = url.match(/^(?:https?:\/\/)?(?:[^@\n]+@)?(?:www\.)?([^:\/\n?]+)/im);
        return match ? match[1] : url;
    }
}

// Main test function
function testPac() {
    // Get inputs
    const pacFileContent = document.getElementById('pacFile').value.trim();
    const testUrl = document.getElementById('testUrl').value.trim();
    const clientIp = document.getElementById('clientIp').value.trim();

    // Collect DNS mappings
    dnsMappingsMap = collectDnsMappings();

    // Validate inputs
    if (!pacFileContent) {
        showResult('error', 'Please provide a PAC file');
        return;
    }

    if (!testUrl) {
        showResult('error', 'Please provide a test URL');
        return;
    }

    try {
        // Extract host from URL
        const host = extractHost(testUrl);

        // Build the complete self-contained script.
        // PAC_UTILS_JS (from pac_utils.js) and the mock functions are all
        // defined inside the IIFE so they share the same scope — this ensures
        // that isInNet(), isResolvable() etc. call our local dnsResolve() mock
        // rather than any global definition.
        // The script returns [result, dnsLog, myIpCalled] as a tuple.
        const script = `
            (function() {
                ${PAC_UTILS_JS}

                // DNS resolution mock — uses user-provided mappings
                var __dnsLog = [];
                var __mappings = ${JSON.stringify(dnsMappingsMap)};
                function dnsResolve(host) {
                    __dnsLog.push(host);
                    return __mappings.hasOwnProperty(host) ? __mappings[host] : null;
                }

                // myIpAddress mock — returns user-provided client IP
                var __myIpCalled = false;
                function myIpAddress() {
                    __myIpCalled = true;
                    return ${JSON.stringify(clientIp || '127.0.0.1')};
                }

                // User's PAC file
                ${pacFileContent}

                // Call the entry point via the findProxyForURL dispatcher defined
                // in PAC_UTILS_JS, which handles both FindProxyForURL and
                // FindProxyForURLEx. Throw early if neither is defined.
                if (typeof FindProxyForURL !== 'function' && typeof FindProxyForURLEx !== 'function') {
                    throw new Error('FindProxyForURL function not found in PAC file');
                }
                var __result = findProxyForURL(${JSON.stringify(testUrl)}, ${JSON.stringify(host)});

                return [__result, __dnsLog, __myIpCalled];
            })();
        `;

        // Evaluate the script and unpack the returned tuple
        const [result, dnsLog, myIpCalled] = eval(script);

        // Show success result
        showResult('success', result, {
            url: testUrl,
            host: host,
            clientIp: clientIp || '127.0.0.1',
            dnsCallLog: dnsLog,
            myIpCallLog: myIpCalled
        });

    } catch (error) {
        showResult('error', `Error: ${error.message}`);
        console.error('PAC evaluation error:', error);
    }
}

// Display results
function showResult(type, message, debug = null) {
    const resultsSection = document.getElementById('resultsSection');
    const resultsDiv = document.getElementById('results');

    resultsSection.style.display = 'block';

    let className = 'result-success';
    let label = 'Proxy Result';

    if (type === 'error') {
        className = 'result-error';
        label = 'Error';
    } else if (type === 'warning') {
        className = 'result-warning';
        label = 'Warning';
    }

    let html = `
        <div class="${className}">
            <div class="result-label">${label}:</div>
            <div class="result-value">${escapeHtml(message)}</div>
    `;

    if (debug) {
        html += `
            <div class="debug-info">
                <h4>Debug Information:</h4>
                <ul>
                    <li><strong>URL:</strong> ${escapeHtml(debug.url)}</li>
                    <li><strong>Host:</strong> ${escapeHtml(debug.host)}</li>
                    <li><strong>Client IP:</strong> ${escapeHtml(debug.clientIp)}</li>
                    ${debug.myIpCallLog ? '<li><strong>myIpAddress()</strong> was called</li>' : ''}
                    ${debug.dnsCallLog.length > 0 ?
                        `<li><strong>DNS lookups:</strong> ${escapeHtml(debug.dnsCallLog.join(', '))}</li>` :
                        '<li>No DNS lookups performed</li>'}
                </ul>
            </div>
        `;
    }

    html += '</div>';
    resultsDiv.innerHTML = html;

    // Scroll to results
    resultsSection.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

// Escape HTML to prevent XSS
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Make functions globally accessible
window.addDnsMapping = addDnsMapping;
window.removeDnsMapping = removeDnsMapping;
window.testPac = testPac;
