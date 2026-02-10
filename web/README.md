# PAC File Tester - Web Application

A client-side web application for testing Proxy Auto-Config (PAC) files without installing any software.

## Features

- **100% Client-Side**: All processing happens in your browser - no data is sent to any server
- **Privacy First**: Your PAC files and configuration never leave your machine
- **Full PAC Support**: Implements all standard PAC helper functions
- **Custom DNS Mappings**: Mock DNS resolution for testing different scenarios
- **Debug Information**: See which DNS lookups were performed and what functions were called
- **Simple UI**: Clean, intuitive interface for quick testing

## Usage

1. **Open `index.html`** in your web browser
   - You can open it directly from your file system
   - Or serve it with any web server (e.g., `python -m http.server 8000`)

2. **Provide Your PAC File**
   - Paste the PAC file content directly into the text area, OR
   - Upload a `.pac` file using the file picker

3. **Configure Test Parameters**
   - **Test URL**: The URL you want to test (e.g., `https://example.com`)
   - **Client IP** (optional): The IP address returned by `myIpAddress()` function
   - **DNS Mappings** (optional): Hostname to IP mappings for `dnsResolve()` function

4. **Click "Test Proxy"** to see the result

## Example

### Sample PAC File
A sample PAC file is provided in `sample.pac` for testing.

### Sample Test Configuration
- **URL**: `https://www.google.com`
- **Client IP**: `192.168.1.100`
- **DNS Mappings**:
  - `www.google.com` → `142.250.185.46`
  - `proxy.corporate.com` → `10.0.0.1`

## Supported PAC Functions

The tester implements all standard PAC helper functions:

### Network Functions
- `dnsResolve(host)` - Resolves hostname to IP (uses custom mappings)
- `myIpAddress()` - Returns client IP (uses custom IP or 127.0.0.1)
- `isResolvable(host)` - Checks if hostname can be resolved

### Matching Functions
- `isPlainHostName(host)` - True if hostname has no dots
- `dnsDomainIs(host, domain)` - True if host is in domain
- `localHostOrDomainIs(host, hostdom)` - True if host matches
- `shExpMatch(str, pattern)` - Shell expression matching
- `isInNet(host, pattern, mask)` - IP address range checking
- `dnsDomainLevels(host)` - Number of DNS domain levels

### Time Functions
- `weekdayRange([wd1, wd2, "GMT"])` - Weekday matching
- `dateRange([day1, month1, year1, day2, month2, year2, "GMT"])` - Date range matching
- `timeRange([hour1, min1, sec1, hour2, min2, sec2, "GMT"])` - Time range matching

## How It Works

1. The tester loads the PAC utility functions (ported from Mozilla's implementation)
2. It creates mock implementations of `dnsResolve()` and `myIpAddress()` using your custom mappings
3. Your PAC file is evaluated in a sandboxed context
4. The `FindProxyForURL()` function is called with your test URL
5. Results and debug information are displayed

## Deployment

### GitHub Pages
1. Push the `web` directory to your repository
2. Enable GitHub Pages in repository settings
3. Set the source to the `web` directory
4. Access at: `https://yourusername.github.io/pacparser/`

### Netlify
1. Drag and drop the `web` folder to Netlify
2. Or connect your GitHub repository

### Any Static Host
Since this is a pure client-side application, you can host it on:
- AWS S3 + CloudFront
- Google Cloud Storage
- Azure Static Web Apps
- Any web server (Apache, Nginx, etc.)

## Browser Compatibility

Works in all modern browsers:
- Chrome/Edge 90+
- Firefox 88+
- Safari 14+

## Privacy & Security

- **No Server Processing**: Everything runs in your browser using JavaScript
- **No Analytics**: No tracking or data collection
- **No External Requests**: Doesn't make any network requests
- **Safe Evaluation**: PAC files are evaluated in isolated JavaScript contexts

## License

This web application is part of the Pacparser project and is licensed under LGPL.

## Contributing

Found a bug or have a feature request? Please open an issue on the [Pacparser GitHub repository](https://github.com/manugarg/pacparser).
