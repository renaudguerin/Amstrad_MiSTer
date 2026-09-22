#!/usr/bin/env python3
"""ACCC section lookup for agents: find the Compendium sections that answer a
question without reading dozens of grep hits.

Pipeline: parse the checked-in pdf-inspector extractions into numbered sections
(section numbers are shared by the FR and EN editions), shortlist with BM25,
then optionally rerank the shortlist with TypeSafe Jev (one Noul per
query/section pair). Only the top few sections are printed in full; the rest of
the shortlist is printed as one-line headings so the caller can pull more.

The text layers flatten chronograms and tables. Every section that sits on a
page pdf-inspector flagged is marked "visual check": read the rendered page
before trusting a timing rule taken from it (docs/classic/extract/README.md).

Status: proof of concept, advisory only. Never cite its output as verification.

  scripts/accc/lookup.py "what does a type 1 CRTC do when R3 changes during HSYNC?"
  scripts/accc/lookup.py --no-rerank "..."        # BM25 only, no API call
  scripts/accc/lookup.py --section 14.5.2         # print one section
  scripts/accc/lookup.py --check-claim "..." --section 14.5.2

TypeSafe key: $TYPESAFE_API_KEY, else macOS Keychain item "typesafe-api".
Without a key or network (sandboxed workers, other hosts) a lookup falls back
to BM25 ranking and says so; --check-claim needs Jev and exits instead.

Technical information sourced from the "Amstrad CPC CRTC Compendium" by
Longshot (CC BY-NC-ND); section text printed by this tool is quoted from it.
"""

import argparse
import concurrent.futures
import hashlib
import json
import math
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
EXTRACT = REPO / "docs/classic/extract"
EDITIONS = {
    "en": EXTRACT / "inspector-v1.11-en/ACCC1.11-EN",
    "fr": EXTRACT / "inspector-v1.11-fr/ACCC1.11-FR",
}
PAGES_DIR = EXTRACT / "pages"
API_URL = "https://api.typesafe.ai/v1/systemone"
MODEL = os.environ.get("ACCC_LOOKUP_MODEL", "jev-1.13.0")
CACHE = Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache")) / "accc-lookup/jev.json"

HEADING = re.compile(r"^#{1,6}\s+(\d+(?:\.\d+)*)\.?\s*(.*)$")
# Page footers, in both orders the extraction produces, sometimes glued to the
# end of a body line: "V1.11 – 08.2026 – Page **N** / **296**" (FR "sur **295**")
# and "**N** / **296** V1.11 – 08.2026 – Page".
FOOTER = re.compile(
    r"V1\.11 – 08[./]2026 – Page \*\*(\d+)\*\* (?:/|sur) \*\*\d+\*\*"
    r"|\*\*(\d+)\*\* (?:/|sur) \*\*\d+\*\* V1\.11 – 08[./]2026 – Page")
MAX_STATE_CHARS = 60000  # well under Jev's 32k-token state budget


# ---------------------------------------------------------------- sections

def parse_edition(ed):
    """Map section number -> {title, path, text, pages} for one edition.

    Footers close a page, so text after footer N is on page N+1; body text
    glued to a footer line is kept. The table of
    contents (before section 1) is skipped. A numbered heading must move
    forward in document order (num_ok) and have a letter in its title;
    anything else, such as a table row promoted to a heading, stays body
    text. Unnumbered headings stay inside the enclosing section.
    """
    lines = Path(str(EDITIONS[ed]) + ".md").read_text().splitlines()
    sections, cur, page, last = {}, None, 1, None
    for line in lines:
        m = FOOTER.search(line)
        if m:
            # Text before a glued footer belongs to the page the footer closes.
            rest = (line[:m.start()] + line[m.end():]).strip()
            if cur is not None and rest:
                cur["text"].append(rest)
            page = int(m.group(1) or m.group(2)) + 1
            continue
        h = HEADING.match(line)
        if h:
            t = tuple(int(x) for x in h.group(1).split("."))
            title = h.group(2).strip()
            if (last is None and t == (1,)) or (
                    last and num_ok(last, t) and re.search(r"[A-Za-z]", title)
                    and h.group(1) not in sections):
                last = t
                num = h.group(1)
                parents = [".".join(map(str, t[:i])) for i in range(1, len(t))]
                path = [f"{p} {sections[p]['title']}" for p in parents if p in sections]
                cur = sections[num] = {"num": num, "title": title,
                                       "path": " > ".join(path), "text": [],
                                       "pages": [page]}
                continue
        if cur is None:
            continue
        cur["text"].append(line)
        if cur["pages"][-1] != page and line.strip():
            cur["pages"].append(page)
    for s in sections.values():
        s["text"] = re.sub(r"\n{3,}", "\n\n", "\n".join(s["text"])).strip()
    return sections


def num_ok(prev, t):
    """Section numbers only move forward and by small steps. Some top-level
    headings are missing from the extraction (9 appears only as 9.1), so
    gaps are allowed; table cells promoted to headings (8.4 -> 3) are not."""
    return t > prev and t[0] - prev[0] <= 2


def flagged_pages(ed):
    data = json.loads(Path(str(EDITIONS[ed]) + ".inspection.json").read_text())
    flags = {p: "table/figure" for p in data.get("pages_with_tables", [])}
    for entry in data.get("ocr_reasons_by_page", []):
        flags[entry["page"]] = ",".join(entry.get("reasons", [])) or "ocr"
    return flags


def load():
    en, fr = parse_edition("en"), parse_edition("fr")
    en_flags, fr_flags = flagged_pages("en"), flagged_pages("fr")
    merged = {}
    for num, s in en.items():
        f = fr.get(num, {"title": "", "text": "", "pages": []})
        visual = sorted({p for p in s["pages"] if p in en_flags})
        merged[num] = {
            "num": num,
            "title_en": s["title"], "title_fr": f["title"],
            "path_en": s["path"], "path_fr": f.get("path", ""),
            "text_en": s["text"], "text_fr": f["text"],
            "pages_en": s["pages"], "pages_fr": f["pages"],
            "visual_en": visual,
            "visual_fr": sorted({p for p in f["pages"] if p in fr_flags}),
        }
    return merged


# ---------------------------------------------------------------- BM25

STOP = set("""a an the of to in on at is are be by for and or with what when how
does do that this it as from which if while its there than then into during
de la le les des du un une et en est au aux sur pour par que qui dans""".split())


def tokens(text):
    text = re.sub(r"\btype[- ]?([0-4])\b", r"crtc \1", text, flags=re.I)
    return [t for t in re.findall(r"[a-z0-9]+(?:\.[0-9]+)?", text.lower())
            if t not in STOP and len(t) > 1 or t.isdigit()]


class BM25:
    def __init__(self, docs, k1=1.4, b=0.75):
        self.docs = [Counter(d) for d in docs]
        self.lens = [len(d) for d in docs]
        self.avg = sum(self.lens) / len(self.lens)
        df = Counter(t for d in self.docs for t in d)
        n = len(docs)
        self.idf = {t: math.log(1 + (n - c + 0.5) / (c + 0.5)) for t, c in df.items()}
        self.k1, self.b = k1, b

    def scores(self, query):
        q = tokens(query)
        out = []
        for d, ln in zip(self.docs, self.lens):
            s = 0.0
            for t in q:
                tf = d.get(t)
                if tf:
                    s += self.idf[t] * tf * (self.k1 + 1) / (
                        tf + self.k1 * (1 - self.b + self.b * ln / self.avg))
            out.append(s)
        return out


def shortlist(sections, query, k):
    nums = list(sections)
    docs = []
    for n in nums:
        s = sections[n]
        head = f"{n} {s['path_en']} {s['title_en']} {s['path_fr']} {s['title_fr']} "
        docs.append(tokens(head * 3 + s["text_en"] + " " + s["text_fr"]))
    sc = BM25(docs).scores(query)
    ranked = sorted(zip(nums, sc), key=lambda x: -x[1])
    return [(n, s) for n, s in ranked[:k] if s > 0]


# ---------------------------------------------------------------- TypeSafe

class JevUnavailable(Exception):
    """No key, no network, or an API error: callers fall back or exit."""


def api_key():
    key = os.environ.get("TYPESAFE_API_KEY")
    if key:
        return key
    try:
        return subprocess.run(
            ["security", "find-generic-password", "-s", "typesafe-api", "-w"],
            capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        raise JevUnavailable(
            "no TypeSafe key: set TYPESAFE_API_KEY or add Keychain item 'typesafe-api' "
            "(security add-generic-password -a \"$USER\" -s typesafe-api -w)")


def _cache():
    try:
        return json.loads(CACHE.read_text())
    except (OSError, ValueError):
        return {}


def ask(state, questions, cache):
    body = {"model": MODEL, "state": state, "questions": questions}
    key = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()
    if key in cache:
        return cache[key]
    req = urllib.request.Request(
        API_URL, data=json.dumps(body).encode(), method="POST",
        headers={"Authorization": f"Bearer {api_key()}",
                 "Content-Type": "application/json"})
    for attempt in range(4):
        try:
            with urllib.request.urlopen(req, timeout=120) as r:
                resp = json.load(r)
            break
        except urllib.error.HTTPError as e:
            if e.code != 429 or attempt == 3:
                raise JevUnavailable(f"TypeSafe HTTP {e.code}: {e.read()[:300]!r}")
            import time
            time.sleep(2 ** attempt)
        except (urllib.error.URLError, OSError, ValueError) as e:
            raise JevUnavailable(f"TypeSafe unreachable: {e}")
    cache[key] = resp
    return resp


def save_cache(cache):
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    CACHE.write_text(json.dumps(cache))


RELEVANCE = {
    "type": "noul",
    "instructions": (
        "`question` was asked by an engineer implementing the Amstrad CPC CRTC "
        "(6845 variants: CRTC 0 HD6845S/UM6845, CRTC 1 UM6845R, CRTC 2 MC6845, "
        "CRTC 3/4 ASIC) in an FPGA. Does `section.text`, an excerpt of the "
        "CRTC Compendium, contain the rule or behaviour that answers "
        "`question`, for the CRTC type(s) the question is about?"),
    "criteria": {
        "true": ("The section states the specific rule, condition, value or "
                 "timing asked about (in prose or in a table), so an engineer "
                 "could answer the question from it."),
        "false": ("The section is only on a related topic, only mentions the "
                  "same registers or counters, covers other CRTC types only, "
                  "or answers a different question."),
    },
}


def section_state(s, lang="en"):
    return {"number": s["num"], "title": s[f"title_{lang}"],
            "within": s[f"path_{lang}"],
            "text": s[f"text_{lang}"][:MAX_STATE_CHARS]}


def rerank(sections, query, cands, lang="en"):
    cache = _cache()

    def one(num):
        state = {"question": query, "section": section_state(sections[num], lang)}
        r = ask(state, {"relevant": RELEVANCE}, cache)
        return num, r["answers"]["relevant"]["noul"], r.get("usage", {})

    with concurrent.futures.ThreadPoolExecutor(8) as ex:
        results = list(ex.map(one, [n for n, _ in cands]))
    save_cache(cache)
    return sorted(results, key=lambda x: -x[1])


SCAN = {
    "type": "noul",
    "instructions": (
        "`question` was asked by an engineer implementing the Amstrad CPC CRTC "
        "in an FPGA. `section` is the heading path and opening of one "
        "CRTC Compendium section. Is this section likely to contain the "
        "answer to `question`?"),
    "criteria": {
        "true": "The section's topic matches what the question asks about.",
        "false": "The section is about a different topic or mechanism.",
    },
}


def scan(sections, query, k, workers=16):
    """Recall pass: one cheap Noul per section on its heading path and first
    ~600 characters, so sections worded unlike the query still surface."""
    cache = _cache()

    def one(num):
        s = sections[num]
        state = {"question": query, "section": {
            "number": num, "within": s["path_en"], "title": s["title_en"],
            "opening": s["text_en"][:600]}}
        return num, ask(state, {"likely": SCAN}, cache)["answers"]["likely"]["noul"]

    with concurrent.futures.ThreadPoolExecutor(workers) as ex:
        res = list(ex.map(one, list(sections)))
    save_cache(cache)
    return sorted(res, key=lambda x: -x[1])[:k]


def candidates(sections, query, k, mode):
    """Shortlist for the rerank: BM25, Jev scan, or the union of both."""
    bm = shortlist(sections, query, k)
    if mode == "bm25":
        return bm
    sc = scan(sections, query, k)
    if mode == "scan":
        return sc
    seen, out = set(), []
    for n, v in [x for pair in zip(bm, sc) for x in pair]:
        if n not in seen:
            seen.add(n)
            out.append((n, v))
    return out


CLAIM = {
    "type": "choice",
    "instructions": (
        "`claim` was written by an engineer about the Amstrad CPC CRTC, citing "
        "`section`, an excerpt of the CRTC Compendium. Judge only from "
        "`section.text`, without outside knowledge. How does the section "
        "relate to the claim? Details matter: a different CRTC type, a "
        "different register value, an off-by-one count or a different "
        "condition makes the claim not supported."),
    "criteria": {
        "supports": "The section states what the claim says, including its conditions and the CRTC type(s) it names.",
        "contradicts": "The section states something incompatible with the claim, such as a different value, count, timing, condition or CRTC type.",
        "silent": "The section neither states nor contradicts the claim; it would need inference or another section.",
    },
}


def check_claim(sections, claim, num, lang="en"):
    cache = _cache()
    r = ask({"claim": claim, "section": section_state(sections[num], lang)},
            {"verdict": CLAIM}, cache)
    save_cache(cache)
    return r["answers"]["verdict"]


# ---------------------------------------------------------------- output

def visual_note(s):
    if not s["visual_en"] and not s["visual_fr"]:
        return ""
    pngs = [str(p.relative_to(REPO)) for pg in s["visual_en"]
            for p in [PAGES_DIR / f"p{pg:03d}.png"] if p.exists()]
    pngs += [str(p.relative_to(REPO)) for pg in s["visual_fr"]
             for p in [PAGES_DIR / f"fr_p{pg:03d}.png"] if p.exists()]
    out = (f"VISUAL CHECK: flagged pages EN {s['visual_en']} FR {s['visual_fr']}; "
           "tables/chronograms are flattened in this text")
    return out + (f"; rendered: {', '.join(pngs)}" if pngs else "")


def headline(s, score=None):
    sc = f"[{score:.2f}] " if score is not None else ""
    vis = " (visual)" if s["visual_en"] or s["visual_fr"] else ""
    path = f"{s['path_en']} > " if s["path_en"] else ""
    return (f"{sc}§{s['num']} {path}{s['title_en']} / {s['title_fr']} "
            f"— EN p{s['pages_en'][0]}, FR p{s['pages_fr'][0] if s['pages_fr'] else '?'}{vis}")


def print_full(s, score=None):
    print(f"=== {headline(s, score)}")
    note = visual_note(s)
    if note:
        print(note)
    print(s["text_en"])
    print()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("query", nargs="?")
    ap.add_argument("--section")
    ap.add_argument("--check-claim", metavar="CLAIM")
    ap.add_argument("--lang", choices=["en", "fr", "both"], default="en",
                    help="edition sent to Jev (EN is Jev's strongest; 'both' checks EN and FR for translation traps)")
    ap.add_argument("--bilingual", action="store_true",
                    help="shortcut for --lang both when checking claims")
    ap.add_argument("--no-rerank", action="store_true")
    ap.add_argument("--shortlist", type=int, default=30)
    ap.add_argument("--recall", choices=["bm25", "scan", "hybrid"], default="hybrid")
    ap.add_argument("--top", type=int, default=3)
    ap.add_argument("--json", action="store_true", help="machine output, no section text")
    a = ap.parse_args()
    if a.bilingual:
        a.lang = "both"
    sections = load()

    if a.check_claim:
        if a.section not in sections:
            sys.exit(f"unknown section {a.section}")
        s = sections[a.section]
        if a.lang == "both":
            try:
                v_en = check_claim(sections, a.check_claim, a.section, "en")
                v_fr = check_claim(sections, a.check_claim, a.section, "fr")
            except JevUnavailable as e:
                sys.exit(f"claim check needs Jev: {e}")
            disagree = v_en["choice"] != v_fr["choice"]
            trap = (f"DISAGREEMENT: EN evaluates '{v_en['choice']}' ({v_en['confidence']:.2f}) "
                    f"while FR evaluates '{v_fr['choice']}' ({v_fr['confidence']:.2f}). "
                    "Often Jev noise, not a translation difference: compare the two editions' text "
                    "yourself; French takes precedence unless hardware supersedes it.") if disagree else None
            print(json.dumps({
                "section": a.section,
                "disagreement": disagree,
                "possible_translation_trap": trap,
                "en": {"verdict": v_en["choice"], "confidence": round(v_en["confidence"], 3),
                       "probabilities": v_en["probabilities"]},
                "fr": {"verdict": v_fr["choice"], "confidence": round(v_fr["confidence"], 3),
                       "probabilities": v_fr["probabilities"]},
                "visual_check": visual_note(s) or None,
                "advisory": "text-layer signal only; never cite as verification"
            }, indent=2, ensure_ascii=False))
            return
        try:
            v = check_claim(sections, a.check_claim, a.section, a.lang)
        except JevUnavailable as e:
            sys.exit(f"claim check needs Jev: {e}")
        print(json.dumps({"section": a.section, "verdict": v["choice"],
                          "confidence": round(v["confidence"], 3),
                          "probabilities": v["probabilities"],
                          "visual_check": visual_note(s) or None,
                          "advisory": "text-layer judgment only; not verification"},
                         indent=2, ensure_ascii=False))
        return
    if a.section:
        if a.section not in sections:
            sys.exit(f"unknown section {a.section}")
        print_full(sections[a.section])
        return
    if not a.query:
        ap.error("query, --section or --check-claim required")

    lang = "en" if a.lang == "both" else a.lang  # "both" only means something for claims
    try:
        cands = candidates(sections, a.query, a.shortlist, a.recall)
        if a.no_rerank:
            ranked = [(n, None) for n, _ in cands]
        else:
            ranked = [(n, p) for n, p, _ in rerank(sections, a.query, cands, lang)]
    except JevUnavailable as e:
        print(f"NOTE: Jev unavailable ({e}); showing BM25 ranking only, which is much "
              "weaker: check the headings below before trusting the top sections.\n")
        ranked = [(n, None) for n, _ in shortlist(sections, a.query, a.shortlist)]
    if a.json:
        print(json.dumps([{"section": n, "score": p} for n, p in ranked]))
        return
    for n, p in ranked[:a.top]:
        print_full(sections[n], p)
    if len(ranked) > a.top:
        print("--- other candidates (lookup.py --section N to read):")
        for n, p in ranked[a.top:]:
            print(headline(sections[n], p))


if __name__ == "__main__":
    main()
