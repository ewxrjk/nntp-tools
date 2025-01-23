# Changes to rjk-nntp-tools

Please see git history for more detailed changes.

## Changes in version 34

Track dependency changes.

## Changes in version 33

Improve spoolstats handling of invalid articles.

## Changes in version 32

Update output directory for spoolstats to `/var/www/html`.

## Changes in version 31

More reproducible builds.

## Changes in version 30

Miscellaneous build fixes.

## Changes in version 29

Miscellaneous build fixes and bug fixes.

## Changes in version 28

New program, `news-sources`, for graphically
representing the share of news sent by each peer.

## Changes in version 0.27

In the Debian package, `spoolstats` runs as
user `news` by default now.  This can be changed by
editing `/etc/default/spoolstats`.

## Changes in version 0.26

Less warnings from spoolstats.

## Changes in version 0.25

New user-agent table entries for spoolstats.

## Changes in version 0.24

Trivial fixes plus a new tool,
`find-unhistorical`, to find articles in an INN news
spool that aren’t in the history.

## Changes in version 0.23

### `lj2news`

* Check the HTTP status and content type before assuming
  that the data at the given URL is RSS.  This should produce
  more comprehensible error messages, particularly when using
  unreliable feeds.
  * Limit the protocols supported to HTTP and HTTPS.
* More verbose debug output.

## Changes in version 0.22

### `bzr2news` and `git2news`

* Include MIME headers documenting the encoding used, and
  translate encodings as required to produce coherent
  results.
  * Track posted commits locally rather than relying on the
  news server rejecting duplicate message IDs.
  * `bzr2news` now works properly.
* Improve `--help` output.

### `lj2news`

* Don’t choke on RSS that uses XML namespaces.
* New `--timeout` option to `lj2news`.
* Honor `-4` and `-6` options more
  fully.
  
### Spoolstats

* The Y axes are now set more closely.
* The color and font of the graphs have been changed
  slightly.
  * When the graph covers a small number of years, unlabelled
  ticks are included for each month.
  * The scan and report phases can be individually suppressed;
  the results of the scan phase are written to logfiles.
  
### Miscellaneous

* Build fixes for OSX/Fink.
* Properly initialize gcrypt.
* Disable `O_NONBLOCK` when invoking authinfo
  generic helper.

## Changes in version 0.21

`git2news` now supports using non-default
branches.  For instance you can write:
```
git2news -n example.commits rjk-nntp-tools rjk-nntp-tools:release
```

…to publish commits from both the default (master) and
release branches.
