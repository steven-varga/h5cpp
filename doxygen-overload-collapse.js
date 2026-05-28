// Doxygen 1.9.x renders function-group pages with one <h2 class="memtitle">
// heading per overload — `read() [1/10]`, `read() [2/10]`, ... — producing a
// long vertical stack on overload-heavy free-function groups like the
// `datasets` and `attribute-io` groups in h5cpp.
//
// This script collapses each contiguous run of same-name overloads into a
// single visible heading; the [2..N] overloads' detail blocks become
// <details> sections, collapsed by default and labelled with each overload's
// own [k/N] tag. Anchors are preserved so deep-links from the rest of the
// doc tree still scroll to the exact overload.
//
// Look-and-feel intent: each function name appears as ONE entry on the group
// page, mirroring how a class member page renders overloads under one
// heading. The detail body for [1/N] is shown inline (so the most common
// signature reads at a glance); the rest fold under disclosure widgets.

(function () {
    function nameKey(h2) {
        // h2.memtitle text is "fnname() [k/N]" (or just "fnname()" when not
        // an overload). Strip the [k/N] and trailing whitespace to get the
        // group key.
        const txt = h2.textContent || "";
        // Remove the diamond glyph + non-breaking space the permalink span
        // injects ("◆ "), then drop the [k/N] suffix.
        return txt.replace(/◆\s*/, "").replace(/\s*\[\d+\/\d+\]\s*$/, "").trim();
    }

    function overloadIndex(h2) {
        const m = (h2.textContent || "").match(/\[(\d+)\/(\d+)\]\s*$/);
        return m ? { k: parseInt(m[1], 10), n: parseInt(m[2], 10) } : null;
    }

    function detailBlockAfter(h2) {
        // The detail block for a memtitle is the immediately-following
        // <div class="memitem"> (which itself contains the mlabels table
        // with the rendered signature plus the memdoc). Doxygen always
        // emits exactly one memitem per memtitle.
        let node = h2.nextElementSibling;
        while (node && !(node.tagName === "DIV" && node.classList.contains("memitem"))) {
            node = node.nextElementSibling;
        }
        return node;
    }

    function collapseOverloads(root) {
        const headings = Array.from(root.querySelectorAll("h2.memtitle"));
        if (!headings.length) return;

        // Group consecutive headings sharing the same name key.
        const groups = [];
        let cur = null;
        for (const h of headings) {
            const key = nameKey(h);
            if (cur && cur.key === key) {
                cur.items.push(h);
            } else {
                if (cur) groups.push(cur);
                cur = { key, items: [h] };
            }
        }
        if (cur) groups.push(cur);

        for (const g of groups) {
            if (g.items.length < 2) continue;

            // Strip the [k/N] suffix from the first (visible) heading and
            // append a compact overload index that links to each one.
            const first = g.items[0];
            const firstIdx = overloadIndex(first);
            if (firstIdx) {
                first.innerHTML = first.innerHTML.replace(
                    /<span class="overload">\[\d+\/\d+\]<\/span>/,
                    ""
                );
            }

            // Build the overload jump-strip and insert it right after the
            // first heading, before its detail body.
            const strip = document.createElement("div");
            strip.className = "overload-jump-strip";
            strip.innerHTML =
                '<span class="overload-jump-label">overloads:</span> ' +
                g.items
                    .map((h, i) => {
                        // Each h2 is preceded by an <a id="..."> anchor or
                        // contains one inside the permalink span. We pull the
                        // id from the preceding anchor.
                        let id = null;
                        let prev = h.previousElementSibling;
                        while (prev) {
                            if (prev.tagName === "A" && prev.id) { id = prev.id; break; }
                            if (prev.tagName === "H2") break;
                            prev = prev.previousElementSibling;
                        }
                        return id
                            ? `<a class="overload-jump-link" href="#${id}">[${i + 1}/${g.items.length}]</a>`
                            : `<span class="overload-jump-link">[${i + 1}/${g.items.length}]</span>`;
                    })
                    .join(" ");
            first.insertAdjacentElement("afterend", strip);

            // Fold the [2..N] overloads into <details> elements. The detail
            // body sits inside the <details>; the heading becomes the
            // <summary>. The preceding <a id="..."> anchor moves INTO the
            // <details> so deep-link resolution (openOnHash) can walk up
            // from the anchor's parent chain and open the right <details>.
            for (let i = 1; i < g.items.length; i++) {
                const h = g.items[i];
                const detail = detailBlockAfter(h);
                if (!detail) continue;

                const details = document.createElement("details");
                details.className = "overload-fold";
                const summary = document.createElement("summary");
                summary.className = "overload-summary";
                // Strip the diamond glyph from summary text — the disclosure
                // triangle replaces it visually.
                summary.innerHTML = h.innerHTML.replace(/◆\s*/, "");
                details.appendChild(summary);

                // Collect the preceding <a id="..."> anchor (if any) so it
                // moves into the <details> alongside the detail body.
                const anchor =
                    h.previousElementSibling &&
                    h.previousElementSibling.tagName === "A" &&
                    h.previousElementSibling.id
                        ? h.previousElementSibling
                        : null;

                // Replace the original heading + detail with the <details>
                // wrapper containing the anchor + detail body.
                h.replaceWith(details);
                detail.parentNode.removeChild(detail);
                if (anchor) {
                    anchor.parentNode.removeChild(anchor);
                    details.insertBefore(anchor, summary.nextSibling);
                }
                details.appendChild(detail);
            }
        }
    }

    function openOnHash() {
        // If the URL fragment targets an anchor inside a collapsed
        // <details>, open it so the deep-link actually scrolls into view.
        if (!window.location.hash) return;
        const id = window.location.hash.slice(1);
        const target = document.getElementById(id);
        if (!target) return;
        let node = target.parentElement;
        while (node) {
            if (node.tagName === "DETAILS") node.open = true;
            node = node.parentElement;
        }
        // Re-trigger the scroll after the layout reflows.
        target.scrollIntoView();
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", () => {
            collapseOverloads(document);
            openOnHash();
        });
    } else {
        collapseOverloads(document);
        openOnHash();
    }
    window.addEventListener("hashchange", openOnHash);
})();
