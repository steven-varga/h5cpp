#!/usr/bin/env bash
# docs-postprocess.sh — post-process the Doxygen HTML output to use canonical
# external URL forms.
#
# Doxygen unconditionally appends `.html` to URLs derived from a tagfile's
# bare <anchorfile> paths, producing e.g.
#   https://en.cppreference.com/cpp/string/basic_string.html
# cppreference's canonical URL is the extension-less form:
#   https://en.cppreference.com/cpp/string/basic_string
# Both resolve, but the canonical is preferred by cppreference and by readers
# scanning generated documentation.
#
# Usage:
#   scripts/docs-postprocess.sh <html-dir>
# Example:
#   scripts/docs-postprocess.sh docs/doxygen/html

set -euo pipefail

html_dir="${1:-docs/doxygen/html}"
test -d "$html_dir" || { echo "no such directory: $html_dir" >&2; exit 1; }

# Strip trailing `.html` from cppreference URLs. Path char class is the
# negation of URL-delimiter characters: anything that's not a quote, angle
# bracket, paren, comma, space, or backtick. This handles operator names
# (`operator=`), destructors (`~list`), and any other special chars
# cppreference uses in its paths, while still preventing greedy matches from
# spanning multiple URLs on the same line (e.g. Doxygen's search/*.js index
# entries that bundle several URLs per record).
url_re='https?://en\.cppreference\.com/[^"<>'"'"'()`, ]+'
count=$(grep -rohE "${url_re}\.html" "$html_dir" 2>/dev/null | sort -u | wc -l)
find "$html_dir" -type f \( -name '*.html' -o -name '*.js' \) -print0 \
  | xargs -0 sed -i -E "s|(${url_re})\.html|\1|g"

echo "docs-postprocess: rewrote $count distinct cppreference URLs in $html_dir"

# ----------------------------------------------------------------------------
# Tighten spacing in function-signature rendering.
#
# Doxygen 1.9.x emits memname/paramtype cells with cosmetic whitespace that
# CSS can't reach because it lives in the cell *text content*:
#
#   <td class="memname">h5::at_t h5::create </td>           <- trailing " " before "("
#   <td class="paramtype">const hid_t &amp;&#160;</td>      <- space between type and "&"
#   <td class="paramtype">args_t &amp;&amp;...&#160;</td>   <- same for "&&"
#   <td class="paramtype">T *&#160;</td>                    <- same for pointer "*"
#
# The template-parameter line gets a similar quirk:
#
#   template&lt;class T , class hid_t , class... args_t&gt;
#
# Replace these in the static HTML so the rendered signature reads as the
# equivalent C++ declaration: `h5::create( const hid_t& parent, ... )` and
# `template<class T, class hid_t, ...>`. Only signature containers are
# touched (memname / paramtype cells, memtemplate lines) — narrative prose
# is untouched.
find "$html_dir" -type f -name '*.html' -print0 | xargs -0 sed -i -E \
  -e 's|(<td class="memname">[^<]*[^ <]) (</td>)|\1\2|g' \
  -e '/<td class="paramtype">/ s| (&amp;)|\1|g' \
  -e '/<td class="paramtype">/ s| (\*)|\1|g'

# Template-parameter line: strip " ," → "," (loop until no more matches).
find "$html_dir" -type f -name '*.html' -print0 | xargs -0 sed -i -E '
  /^template&lt;/{
    :loop
    s/ ,/,/
    tloop
  }'

echo "docs-postprocess: tightened function-signature whitespace"
