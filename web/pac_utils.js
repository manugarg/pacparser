// Auto-generated from src/pac_utils.h — do not edit manually.
// Regenerate with: make -C src pac_utils_js
//
// Exported as a source string (PAC_UTILS_JS) rather than executed
// directly, so that the eval'd PAC sandbox can define its own
// dnsResolve() / myIpAddress() in the same scope as the utility
// functions — keeping DNS mocking correct via lexical scoping.
const PAC_UTILS_JS = `
function dnsDomainIs(host, domain) {
    return (host.length >= domain.length &&
            host.substring(host.length - domain.length) == domain);
}
function dnsDomainLevels(host) {
    return host.split('.').length-1;
}
function convert_addr(ipchars) {
    var bytes = ipchars.split('.');
    var result = ((bytes[0] & 0xff) << 24) |
                 ((bytes[1] & 0xff) << 16) |
                 ((bytes[2] & 0xff) <<  8) |
                  (bytes[3] & 0xff);
    return result;
}
function isInNet(ipaddr, pattern, maskstr) {
    var test = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/.exec(ipaddr);
    if (test == null) {
        ipaddr = dnsResolve(ipaddr);
        if (ipaddr == null)
            return false;
    } else if (test[1] > 255 || test[2] > 255 || 
               test[3] > 255 || test[4] > 255) {
        return false;    // not an IP address
    }
    var host = convert_addr(ipaddr);
    var pat  = convert_addr(pattern);
    var mask = convert_addr(maskstr);
    return ((host & mask) == (pat & mask));
    
}
function convert_addr6(ipchars) {
    ipchars = ipchars.replace(/(^:|:$)/, '');
    var fields = ipchars.split(':');
    var diff = 8 - fields.length;
    for (var i = 0; i < fields.length; i++) {
        if (fields[i] == '') {
            fields[i] = '0';
            // inject 'diff' number of '0' elements here.
            for (var j = 0; j < diff; j++) {
                fields.splice(i++, 0, '0');
            }
            break;
        }
    }
    var result = [];
    for (var i = 0; i < fields.length; i++) {
        result.push(parseInt(fields[i], 16));
    }
    return result;
}
function isInNetEx6(ipaddr, prefix, prefix_len) {
    if (prefix_len > 128) {
        return false;
    }
    prefix = convert_addr6(prefix);
    ip = convert_addr6(ipaddr);
    // Prefix match strategy:
    //   Compare only prefix length bits between 'ipaddr' and 'prefix'
    //   Match in the batches of 16-bit fields 
    prefix_rem = prefix_len % 16;
    prefix_nfields = (prefix_len - prefix_rem) / 16;

    for (var i = 0; i < prefix_nfields; i++) {
        if (ip[i] != prefix[i]) {
            return false;
        }
    }
    if (prefix_rem > 0) {
        // Compare remaining bits
        prefix_bits = prefix[prefix_nfields] >> (16 - prefix_rem);
        ip_bits = ip[prefix_nfields] >> (16 - prefix_rem);
        if (ip_bits != prefix_bits) {
            return false;
        }
    }
    return true;
}
function isInNetEx4(ipaddr, prefix, prefix_len) {
    if (prefix_len > 32) {
        return false;
    }
    var netmask = [];
    for (var i = 1; i < 5; i++) {
        var shift_len = 8 * i - prefix_len;
        if (shift_len <= 0) {
            netmask.push(255)
        } else {
            netmask.push((0xff >> shift_len) << shift_len);
        }
    }
    return isInNet(ipaddr, prefix, netmask.join('.'));
}
function isInNetEx(ipaddr, prefix) {
    prefix_a = prefix.split('/');
    if (prefix_a.length != 2) {
        return false;
    }
    var test = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(ipaddr);
    if (!test) {
        return isInNetEx6(ipaddr, prefix_a[0], prefix_a[1]);
    } else {
        return isInNetEx4(ipaddr, prefix_a[0], prefix_a[1]);
    }
}
function isPlainHostName(host) {
    return (host.search('\\.') == -1);
}
function isResolvable(host) {
    var ip = dnsResolve(host);
    return (ip != null);
}
if (typeof(dnsResolveEx) == "function") {
function isResolvableEx(host) {
    var ip = dnsResolveEx(host);
    return (ip != null);
}
}
function localHostOrDomainIs(host, hostdom) {
    return (host == hostdom) ||
           (hostdom.lastIndexOf(host + '.', 0) == 0);
}
function shExpMatch(url, pattern) {
   pattern = pattern.replace(/\./g, '\\.');
   pattern = pattern.replace(/\*/g, '.*');
   pattern = pattern.replace(/\?/g, '.');
   var newRe = new RegExp('^'+pattern+'$');
   return newRe.test(url);
}
var wdays = {SUN: 0, MON: 1, TUE: 2, WED: 3, THU: 4, FRI: 5, SAT: 6};
var months = {JAN: 0, FEB: 1, MAR: 2, APR: 3, MAY: 4, JUN: 5, JUL: 6, AUG: 7, SEP: 8, OCT: 9, NOV: 10, DEC: 11};
function weekdayRange() {
    function getDay(weekday) {
        if (weekday in wdays) {
            return wdays[weekday];
        }
        return -1;
    }
    var date = new Date();
    var argc = arguments.length;
    var wday;
    if (argc < 1)
        return false;
    if (arguments[argc - 1] == 'GMT') {
        argc--;
        wday = date.getUTCDay();
    } else {
        wday = date.getDay();
    }
    var wd1 = getDay(arguments[0]);
    var wd2 = (argc == 2) ? getDay(arguments[1]) : wd1;
    return (wd1 == -1 || wd2 == -1) ? false
                                    : (wd1 <= wday && wday <= wd2);
}
function dateRange() {
    function getMonth(name) {
        if (name in months) {
            return months[name];
        }
        return -1;
    }
    var date = new Date();
    var argc = arguments.length;
    if (argc < 1) {
        return false;
    }
    var isGMT = (arguments[argc - 1] == 'GMT');

    if (isGMT) {
        argc--;
    }
    // function will work even without explicit handling of this case
    if (argc == 1) {
        var tmp = parseInt(arguments[0]);
        if (isNaN(tmp)) {
            return ((isGMT ? date.getUTCMonth() : date.getMonth()) ==
getMonth(arguments[0]));
        } else if (tmp < 32) {
            return ((isGMT ? date.getUTCDate() : date.getDate()) == tmp);
        } else { 
            return ((isGMT ? date.getUTCFullYear() : date.getFullYear()) ==
tmp);
        }
    }
    var year = date.getFullYear();
    var date1, date2;
    date1 = new Date(year,  0,  1,  0,  0,  0);
    date2 = new Date(year, 11, 31, 23, 59, 59);
    var adjustMonth = false;
    for (var i = 0; i < (argc >> 1); i++) {
        var tmp = parseInt(arguments[i]);
        if (isNaN(tmp)) {
            var mon = getMonth(arguments[i]);
            date1.setMonth(mon);
        } else if (tmp < 32) {
            adjustMonth = (argc <= 2);
            date1.setDate(tmp);
        } else {
            date1.setFullYear(tmp);
        }
    }
    for (var i = (argc >> 1); i < argc; i++) {
        var tmp = parseInt(arguments[i]);
        if (isNaN(tmp)) {
            var mon = getMonth(arguments[i]);
            date2.setMonth(mon);
        } else if (tmp < 32) {
            date2.setDate(tmp);
        } else {
            date2.setFullYear(tmp);
        }
    }
    if (adjustMonth) {
        date1.setMonth(date.getMonth());
        date2.setMonth(date.getMonth());
    }
    if (isGMT) {
    var tmp = date;
        tmp.setFullYear(date.getUTCFullYear());
        tmp.setMonth(date.getUTCMonth());
        tmp.setDate(date.getUTCDate());
        tmp.setHours(date.getUTCHours());
        tmp.setMinutes(date.getUTCMinutes());
        tmp.setSeconds(date.getUTCSeconds());
        date = tmp;
    }
    return ((date1 <= date) && (date <= date2));
}
function timeRange() {
    var argc = arguments.length;
    var date = new Date();
    var isGMT= false;

    if (argc < 1) {
        return false;
    }
    if (arguments[argc - 1] == 'GMT') {
        isGMT = true;
        argc--;
    }

    var hour = isGMT ? date.getUTCHours() : date.getHours();
    var date1, date2;
    date1 = new Date();
    date2 = new Date();

    if (argc == 1) {
        return (hour == arguments[0]);
    } else if (argc == 2) {
        return ((arguments[0] <= hour) && (hour <= arguments[1]));
    } else {
        switch (argc) {
        case 6:
            date1.setSeconds(arguments[2]);
            date2.setSeconds(arguments[5]);
        case 4:
            var middle = argc >> 1;
            date1.setHours(arguments[0]);
            date1.setMinutes(arguments[1]);
            date2.setHours(arguments[middle]);
            date2.setMinutes(arguments[middle + 1]);
            if (middle == 2) {
                date2.setSeconds(59);
            }
            break;
        default:
          throw 'timeRange: bad number of arguments'
        }
    }

    if (isGMT) {
        date.setFullYear(date.getUTCFullYear());
        date.setMonth(date.getUTCMonth());
        date.setDate(date.getUTCDate());
        date.setHours(date.getUTCHours());
        date.setMinutes(date.getUTCMinutes());
        date.setSeconds(date.getUTCSeconds());
    }
    return ((date1 <= date) && (date <= date2));
}
function findProxyForURL(url, host) {
    if (typeof FindProxyForURLEx == 'function') {
        return FindProxyForURLEx(url, host);
    } else {
        return FindProxyForURL(url, host);
    }
}
`;
