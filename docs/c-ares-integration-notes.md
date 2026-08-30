# c-ares integration notes (extracted from PR #75)

PR [#75 "Google backporting"](https://github.com/manugarg/pacparser/pull/75)
backports work from Google's fork of pacparser (c-ares integration authored by
Stefano Lattarini, commits spanning 2012–2016). The PR was never merged and is
based on the old SpiderMonkey-era layout (flat tree, `jsapi.h`), while current
`main` uses QuickJS with sources under `src/`. Nothing here is directly
applicable as-is, but the design patterns, API shape, test approach, and the
list of rough edges are worth keeping.

Why c-ares in the first place:

- resolution independent of `/etc/resolv.conf` (custom nameservers),
- custom DNS *search domains* (no equivalent with `getaddrinfo()`),
- deterministic behavior for tests (pin a known upstream server).

All code references below are to the PR head (`ef0f7b9`), fetchable via
`git fetch origin pull/75/head:pr75`.

## 1. Resolver abstraction (the core pattern)

Resolution is dispatched through a single function pointer, with three
swappable backends (`pr75:pacparser.c:73`):

```c
typedef char *(*pacparser_resolve_host_func)(const char *, int all_ips);
```

| variant string | macro (in `pacparser.h`) | implementation |
|---|---|---|
| `"none"` | `DNS_NONE` | `pacparser_resolve_host_literal_ips` — only literal IPs "resolve", to themselves |
| `"getaddrinfo"` | `DNS_GETADDRINFO` | `pacparser_resolve_host_getaddrinfo` — default |
| `"c-ares"` | `DNS_C_ARES` | `pacparser_resolve_host_ares` |

Selected at runtime by `pacparser_set_dns_resolver_variant()`
(`pr75:pacparser.c:598`). Notes:

- String constants instead of an enum on purpose (commit `6e37da6`:
  "Get rid of dns_resolver_t, using strings is simpler & clearer") — strings
  cross the C/Python binding boundary without an extra mapping layer.
- Empty/NULL hostnames short-circuit to `NULL` in the dispatcher
  (`pr75:pacparser.c:622`) *before* any resolver is called, so no resolver
  (including c-ares) ever issues a query for `""`.
- The `"none"` variant doubles as a safe offline mode: literal-IP passthrough
  is implemented with `inet_pton()` only (`is_ip_address`, `:264`), so it
  works with zero network stack.
- If `"c-ares"` is requested but the library was built without it, the
  function fails at setup time with a clear error (rather than mis-resolving).

## 2. Compile-time gating and fallbacks

Everything c-ares-related lives behind `#ifdef HAVE_C_ARES`. The
`#else` branch (`pr75:pacparser.c:541-590`) provides **dummy fallbacks so the
public API is identical in both builds**:

- `pacparser_set_dns_servers()` / `pacparser_set_dns_domains()` return `0`
  and print "requires c-ares integration to be compiled-in." (a `NULL`
  argument is still a successful no-op, so callers can pass unparsed
  `optarg` unconditionally),
- `pacparser_ares_init()` succeeds as a no-op,
- `pacparser_resolve_host_ares()` returns `NULL` (unreachable in practice
  since the variant selector already rejects it).

Build system (`pr75:Makefile:45-63`):

```make
ENABLE_C_ARES ?= auto
ifeq "$(ENABLE_C_ARES)" "auto"
  ENABLE_C_ARES := $(shell pkg-config --exists libcares && echo yes || echo no)
  ...
endif
ifeq "$(ENABLE_C_ARES)" "yes"
  C_ARES_CFLAGS += -DHAVE_C_ARES
  ...
else
  $(warning c-ares (http://c-ares.haxx.se) library not found or disabled)
  $(warning Some DNS-related features will be unavailable)
endif
```

- `auto`/`yes`/`no` tri-state with pkg-config auto-detection; degrades to a
  build *warning*, never a failure.
- Adds an rpath for the c-ares libdir
  (`-Wl,-rpath -Wl,$(shell pkg-config --variable=libdir libcares)`) so
  `pactester` runs from the build tree.
- The Python extension build propagates the link flags as an *environment
  variable* into setuptools (`C_ARES_LDFLAGS='$(C_ARES_LDFLAGS)' ... setup.py
  build`, `pr75:Makefile:173`) — a handy pattern when Makefile and
  setup.py both need the same flags.

## 3. Lifecycle and ordering

Strict init/cleanup pairing around `pacparser_init()` /
`pacparser_cleanup()`:

```
pacparser_set_dns_resolver_variant()   ┐
pacparser_set_dns_servers()            ├─ must happen BEFORE pacparser_init()
pacparser_set_dns_domains()            ┘
        │
pacparser_init()  →  pacparser_ares_init()  (before JS engine init)
        │
pacparser_cleanup()  →  JS teardown, then pacparser_ares_cleanup() last
```

- A process-global `ares_initialized` flag guards the two setters: after
  init they refuse with "this function should be called before c-ares is
  initialized" (`pr75:pacparser.c:354-358`). This keeps the ordering rule
  enforceable instead of merely documented.
- `pacparser_ares_init()` (`pr75:pacparser.c:432`) sequence:
  1. `ares_library_init(ARES_LIB_INIT_ALL)`
  2. split comma-separated `dns_domains` into a `char **` list
     (`strtok_r`) and pass via `ARES_OPT_DOMAINS`,
  3. `ares_init_options(&global_channel, &options, optmask)` with
     `ARES_FLAG_NOCHECKRESP`,
  4. optional `ares_set_servers_csv(global_channel, dns_servers)`.
- **Partial-failure cleanup is the pattern worth copying.** A `CLEAN_AND_FAIL`
  macro unwinds only what was actually done, tracked with explicit
  `done_ares_library_init` / `done_ares_init_options` flags
  (`pr75:pacparser.c:440-451`):

  ```c
  #define CLEAN_AND_FAIL() \
    do { \
      free(domains_list); \
      if (done_ares_init_options) \
        ares_destroy(global_channel); \
      if (done_ares_library_init) \
        ares_library_cleanup(); \
      return (0); \
    } while(0)
  ```

  This was hardened in two successive commits (`d85a9e17` "try harder not to
  leave things half-initialized", `4178d2af` "try *even* harder") — the
  original `SAFE_RETURN(retval, deinit_ares)` two-value scheme was not robust
  enough. Rule of thumb: with multi-step third-party init, track each step's
  completion separately rather than passing "how far did I get" as a boolean.
- One global `ares_channel` for the whole process; `pacparser_ares_cleanup()`
  (`:500`) destroys it, calls `ares_library_cleanup()`, and frees the
  strdup'd `dns_servers`/`dns_domains`.

## 4. Synchronous wrapper over an async API

c-ares is asynchronous; the PR deliberately wraps it in a blocking
`select()` loop so the rest of the codebase (and PAC scripts) see a plain
synchronous `resolve(host) -> string` contract. `ares_wait_for_all_queries()`
(`pr75:pacparser.c:408`, "shamelessly copied from ares_process(3)"):

```c
while (1) {
  FD_ZERO(&readers);
  FD_ZERO(&writers);
  nfds = ares_fds(channel, &readers, &writers);
  if (nfds == 0)      /* no outstanding queries */
    break;
  tvp = ares_timeout(channel, NULL, &tv);
  count = select(nfds, &readers, &writers, NULL, tvp);
  if (count < 0 && errno != EINVAL) { ... return 0; }
  ares_process(channel, &readers, &writers);
}
```

The query + callback (`pr75:pacparser.c:515`):

```c
ares_gethostbyname(global_channel, hostname, ai_families[i],
                   callback_for_ares, (void *) &dc);
if (!ares_wait_for_all_queries(global_channel)) { ... }
```

Points of note:

- The c-ares callback signature forces `void *arg`, so user data is a
  casted pointer to the shared `struct dns_collector`
  (`callback_for_ares`, `:386`; the author's comment calls it "a little
  abomination" — it's unavoidable).
- The callback filters on `status == ARES_SUCCESS` **and**
  `host->h_addrtype == dc->ai_family` (family mismatches are expected and
  normal, not errors), and formats addresses with `ares_inet_ntop()`.
- Families are queried **sequentially** (AF_INET, then AF_INET6, each fully
  waited on), with early exit once one address is collected and `all_ips` is
  off. This guarantees IPv4-before-IPv6 output ordering in the combined
  string (a commit explicitly guarantees this: `f6ef5204`). It is not
  parallel — a possible future improvement, at the cost of ordering
  guarantees.
- `select()` is fine here: a channel has one fd per configured server, far
  below `FD_SETSIZE`. Don't over-engineer with `poll()`.
- Because the wait loop is synchronous, the callback runs in the caller's
  thread and the collector needs no locking. (That changes if you ever make
  it truly async.)

## 5. Shared result collector

Both resolvers feed the same collector so **output semantics are identical
across backends** (`;`-separated addresses, one-IP vs all-IPs):

```c
struct dns_collector {
  char *mallocd_addresses;
  int all_ips;
  int ai_family;
};

enum collect_status { COLLECT_DONE = 0, COLLECT_MORE = 1 };
```

- `collect_mallocd_address()` (`pr75:pacparser.c:230`): with `all_ips` off it
  *replaces* the buffer (free + strdup) and returns `COLLECT_DONE`; with
  `all_ips` on it appends `;addr` and returns `COLLECT_MORE`.
- The getaddrinfo path walks the `addrinfo` list calling
  `getnameinfo(..., NI_NUMERICHOST)` per entry
  (`collect_getaddrinfo_results`, `:283`); the c-ares path does the same per
  `h_addr_list` entry in the callback.
- This is the payoff of the "reduce duplication and indirection" commit
  (`455be51`): the *collection* logic is backend-agnostic, only the
  *iteration* differs.

## 6. Public API additions

`pr75:pacparser.h` adds (all `0` = failure / `1` = success, call before
`pacparser_init()`):

```c
int pacparser_set_dns_servers(const char *ips);        /* comma-separated IPs */
int pacparser_set_dns_domains(const char *domains);    /* comma-separated, ac9a2339 */
int pacparser_set_dns_resolver_variant(const char *v); /* DNS_NONE/GETADDRINFO/C_ARES */
```

- `NULL` is a no-op success on the first two (easy for callers that parse
  optional CLI options).
- The PR also shipped man pages for these in `docs/man/man3/`
  (`pacparser_set_dns_resolver_variant.3`, `pacparser_set_dns_servers.3`,
  `pacparser_set_dns_domains.3`) and doxygen comments in the header — i.e.
  the integration was treated as a first-class documented API surface.
- Python module mirrors the API 1:1
  (`set_dns_resolver_variant` with client-side validation against the
  `DNS_*` constants, `set_dns_servers`, `set_dns_domains` in
  `pr75:pymod/pacparser/__init__.py`).

## 7. pactester CLI surface

`pr75:pactester.c` adds:

- `-r DNS_RESOLVER_TYPE` (none | getaddrinfo | c-ares),
- `-s DNS_SERVER_IP` (comma-separated; documented as c-ares-only),
- `-d DNS_DOMAIN_LIST` (comma-separated; documented as c-ares-only).

Setup calls the three `pacparser_set_*` functions and reports failure with
`__FILE__`-prefixed diagnostics (commit `3e2dd530` made the setters' return
values actually checked — "pactester: pacparser_set_dns_{server,domains}
might return error").

## 8. Test patterns

`pr75:pactester_dns_test.sh` + `pr75:pactester_test_lib.sh`:

- **c-ares mode pins a fixed resolver**: with `--c-ares` the test appends
  `-r c-ares -s 8.8.8.8` — "the only way to have reasonably predictable
  results".
- Test harness: each test is a JS body (heredoc) wrapped in
  `FindProxyForURL` that must print `OK`; `ok` / `ko` / `js_true` helpers;
  failures dump the generated PAC + stdout + stderr.
- Loose IP regexes, deliberately:
  `ip4_rx='([0-9]{1,3}\.){3}[0-9]{1,3}'`,
  `up_ip4_rx='([0-9]{1,3}\.){3}129'` — with the comment "don't try to be
  more precise than this, all the tests will become overly brittle (been
  there, done that)".
- Coverage: literal IPv4/IPv6 passthrough, NXDOMAIN → `null` (not `""`),
  empty hostname → `null` without querying, `dnsResolve` single-IP vs
  `dnsResolveEx` multi-IP with IPv4-before-IPv6 ordering, IPv4-only and
  IPv6-only hosts.
- **Search-domain tests (c-ares only)**: `-d l.google.com` makes
  `isResolvable('ipv6')` true; `-d ""` (empty) disables search so
  `dnsResolve('teams')` → `null`; multiple domains try in order. This
  behavior has no getaddrinfo equivalent and is only testable through the
  c-ares path.
- The Makefile `test` target runs the DNS suite **three ways**: once plain
  (getaddrinfo), and once more with `--c-ares` when `ENABLE_C_ARES=yes`
  (`pr75:Makefile:133-147`). Same suite, both backends — cheap regression
  protection that the two resolvers stay behaviorally equivalent.

## 9. Rough edges and porting caveats

If this is ever revisited on current `main` (QuickJS, `src/pacparser.c`,
`resolve_host()` at `src/pacparser.c:142`):

- **The PR's code predates modern c-ares.** It uses
  `ares_gethostbyname()` (deprecated in favor of `ares_getaddrinfo()`, added
  in c-ares 1.15.0), `ares_init_options()` (superseded by `ares_init_data()`,
  1.16+), `ares_set_servers_csv()` (prefer `ares_set_servers()`), and
  `ares_inet_ntop()` (prefer the system `inet_ntop()`). Verify against the
  target c-ares version; the *shapes* of the patterns (init sequence, wait
  loop, callback collector) port fine, the exact calls do not.
- **Memory bugs were real and fixed late in the series** — expect the same
  classes when porting: a double-free in the cleanup path (`e428ce15`), a
  missing `freeaddrinfo()` (`a9870c6a`), the `p = realloc(p, ...)`
  anti-pattern leaking on failed realloc (`55db2cd9`), and a misalignment
  caught by tmalloc (`4a379bb1`). The PR's own comments mark the integration
  as "still quite rough around the edges" (`7d8e996a`).
- **SpiderMonkey → QuickJS**: the PR wires resolvers into JS via
  `JS_DefineFunction` on SpiderMonkey; current `main` uses QuickJS
  `JS_NewCFunction` (`src/pacparser.c:343`). The resolver layer itself is
  engine-agnostic and would drop in around the existing `dns_resolve` /
  `dns_resolve_ex` QuickJS wrappers.
- **Global mutable state**: one global channel + `ares_initialized` flag,
  not thread-safe (the library as a whole isn't either). Fine for current
  usage; document it if it ever changes.
- **`ares_library_cleanup()` in `pacparser_cleanup()`**: c-ares library init
  is reference-counted process-wide. Calling cleanup is only correct if
  pacparser is the sole c-ares consumer in the process — an embedder that
  also uses c-ares elsewhere would need a different policy (e.g. never call
  `ares_library_cleanup`).
- **The sync wrapper forfeits c-ares's concurrency**; the practical gains
  are resolv.conf independence, custom servers, and search domains — not
  speed. Keep expectations aligned with that.

## 10. Key commits (pr75 branch)

| sha | what |
|---|---|
| `20eb9e5` | DNS internals moved to their own file (`pacparser_dns.c`) — later merged back in `620e8d8` (flat file won out) |
| `62f6ef8` | runtime resolver selection added |
| `6e37da6` | enum → string constants for resolver variants |
| `455be51` | shared result collector, dedup between backends |
| `ac9a233` | domains accepted as comma-separated string |
| `3b02377` | "prepare the stage for upcoming c-ares integration" |
| `7d8e996` | the c-ares integration itself |
| `d85a9e1`, `4178d2a` | two rounds of partial-init cleanup hardening |
| `f6ef520` | IPv4-before-IPv6 ordering guarantee |
| `e428ce1`, `a9870c6`, `55db2cd`, `4a379bb` | memory bug fixes (double-free, freeaddrinfo leak, realloc, alignment) |
| `c5142d3` | getaddrinfo path simplification |
| `c46245c` | layout flatten + setuptools (build-system side) |
