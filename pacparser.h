// Initializes pac parser
// It initializes JavaScript engine and does few basic initializations specific
// to pacparser.
// returns 0 on failure and 1 on success.
int pacparser_init();

// Parse pac file
// parses and evaulates PAC file in JavaScript context created by
// pacparser_init.
// returns 0 on failure and 1 on success.
int pacparser_parse_pac(const char *pacfile           // PAC file to parse
                        );

// Finds proxy for a given URL and Host.
// This function should be called only after pacparser engine has been
// initialized (using pacparser_init) and pac file has been parsed (using
// pacparser_parse_pac). It determines "right" proxy (based on pac file) for
// url and host.
// returns proxy string on sucess and NULL on error.
char *pacparser_find_proxy(const char *url,           // URL to find proxy for
                           const char *host           // Host part of the URL
                           );

// Finds proxy for a given PAC file, url and host.
// This function is a wrapper around functions pacparser_init,
// pacparser_parse_pac, pacparser_find_proxy and pacparser_cleanup. If you just
// want to find out proxy a given set of pac file, url and host, this is the
// function to call. This function takes care of all the initialization and
// cleanup.
// returns proxy string on success and NULL on error.
char *pacparser_just_find_proxy(const char *pacfile,  // PAC file
                           const char *url,           // URL to find proxy for
                           const char *host           // Host part of the URL
                           );

// Destroys JavaSctipt context.
// This function should be called once you're done with using pacparser engine.
void pacparser_cleanup();
