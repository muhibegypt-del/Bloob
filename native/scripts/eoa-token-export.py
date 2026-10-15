#!/usr/bin/env python3
"""Export design VALUES (numbers and colours) from the Elements of A UI bundles.

Read-only. Run it on the machine that holds the locally extracted reference
files; it never writes next to them and never touches the network or the TV.

    python3 -I eoa-token-export.py <folder with app.js, ProductPage.js, ...> > eoa-tokens.tsv

Output, one row per value: file, component (the nearest enclosing style or
class name), key, value. Only keys that name a design quantity are kept
(padding, height, radius, spacing, size, duration, alpha, colour...), so the
output holds no code: no functions, strings, identifiers of the app's logic,
artwork or text. Review the TSV, then share it; Bloob copies the values into
native/TOKENS.md and theme.h (owner's rule, 15 October 2026).
"""
import os
import re
import sys

FILES = ["app.js", "ProductPage.js", "SuperBrowsePage.js", "PlayerPage.js", "SearchPageFactory.js", "LibraryPage.js"]
DESIGN_KEY = re.compile(
    r"^(padding\w*|margin\w*|inset\w*|spacing\w*|gap\w*|interitem\w*|line\w*|"
    r"\w*(height|width|radius|size|spacing|padding|margin|offset|inset|gap|leading|tracking|kerning)\w*|"
    r"(min|max)\w*|corner\w*|scale\w*|alpha|opacity\w*|\w*duration\w*|\w*delay\w*|\w*timeout\w*|"
    r"stiffness|damping|mass|velocity|blur\w*|shadow\w*|elevation\w*|weight|font\w*|x|y|top|bottom|left|right|"
    r"leading|trailing|color|\w*Color|tint\w*|r|g|b|a)$",
    re.I,
)
PAIR = re.compile(r"([A-Za-z_$][\w$]*)\s*:\s*(-?\d+(?:\.\d+)?(?:e-?\d+)?)(?![\w.])")
OWNER = re.compile(r"([A-Za-z_$][\w$]{2,})\s*[:=]\s*(?:\{|new\s+[\w$.]+\(\s*\{|[\w$.]+\(\s*\{)")
COLOUR = re.compile(r"(?:rgba?|Color)\s*\(\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)\s*,\s*(\d+(?:\.\d+)?)(?:\s*,\s*(\d*\.?\d+))?\s*\)")


def owners(text):
    """Offsets of names that open an object, to label each value with its component."""
    return [(m.start(), m.group(1)) for m in OWNER.finditer(text)]


def owner_at(table, pos, start):
    lo, hi, best = start, len(table) - 1, ""
    while lo <= hi:
        mid = (lo + hi) // 2
        if table[mid][0] <= pos:
            best, lo = table[mid][1], mid + 1
        else:
            hi = mid - 1
    return best, lo


def main():
    if len(sys.argv) != 2 or not os.path.isdir(sys.argv[1]):
        sys.exit(__doc__)
    root = sys.argv[1]
    print("file\tcomponent\tkey\tvalue")
    for name in FILES:
        path = os.path.join(root, name)
        if not os.path.isfile(path):
            for dirpath, _, files in os.walk(root):
                if name in files:
                    path = os.path.join(dirpath, name)
                    break
        if not os.path.isfile(path):
            print(f"# {name}: not found", file=sys.stderr)
            continue
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        table = owners(text)
        seen = set()
        for m in PAIR.finditer(text):
            key, value = m.group(1), m.group(2)
            if not DESIGN_KEY.match(key):
                continue
            comp, _ = owner_at(table, m.start(), 0)
            row = (name, comp, key, value)
            if row not in seen:
                seen.add(row)
                print("\t".join(row))
        for m in COLOUR.finditer(text):
            comp, _ = owner_at(table, m.start(), 0)
            value = ",".join(v for v in m.groups() if v is not None)
            row = (name, comp, "colour", value)
            if row not in seen:
                seen.add(row)
                print("\t".join(row))


if __name__ == "__main__":
    main()
