#!/usr/bin/env python3
"""Evaluate scripts/accc/lookup.py against the labelled set in accc-eval.json.

Lookups: does the gold section (or an accepted parent/sibling) reach the top
1 / top 3 after BM25 alone and after the Jev rerank, and how many tokens would
the caller read? Claims: Jev verdict against the expected verdict, with the
EN and FR editions as state.

Gold labels come from the repository's own ACCC citations, not from this tool.
Jev calls are cached (see lookup.CACHE), so reruns are free.

  scripts/accc/eval/run_eval.py [--no-jev] [--shortlist 30] [--top 3]
"""

import argparse
import json
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import lookup  # noqa: E402

SET = Path(__file__).with_name("accc-eval.json")


def top(order, ok, k):
    r = rank_of(order, ok)
    return r is not None and r < k


def rank_of(order, ok):
    for i, n in enumerate(order):
        if n in ok:
            return i
    return None


def cost(sections, order, top):
    """Approximate tokens the caller reads: top sections in full plus one
    headline per remaining candidate (chars / 4)."""
    chars = sum(len(sections[n]["text_en"]) + 200 for n in order[:top])
    chars += sum(len(lookup.headline(sections[n])) for n in order[top:])
    return chars // 4

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", type=Path, default=Path(__file__).with_name("accc-eval.json"),
                    help="path or filename of eval JSON set")
    ap.add_argument("--no-jev", action="store_true")
    ap.add_argument("--shortlist", type=int, default=30)
    ap.add_argument("--top", type=int, default=3)
    ap.add_argument("--recall", choices=["bm25", "scan", "hybrid"], default="hybrid")
    ap.add_argument("--no-claims", action="store_true")
    ap.add_argument("-v", action="store_true", help="per-item lines")
    a = ap.parse_args()
    set_path = a.set
    if not set_path.is_file():
        alt = Path(__file__).with_name(str(set_path))
        if alt.is_file():
            set_path = alt
        else:
            sys.exit(f"Dataset not found: {a.set}")
    data = json.loads(set_path.read_text())
    sections = lookup.load()

    stats = Counter()
    toks = {"bm25": [], "jev": [], "grep_all": []}
    for it in data["lookups"]:
        gold = set(it["gold"])
        ok = gold | set(it.get("also_ok", []))
        missing = [g for g in gold if g not in sections]
        if missing:
            print(f"{it['id']}: gold {missing} not parsed; skipped")
            continue
        stats["n"] += 1
        cands = lookup.candidates(sections, it["query"], a.shortlist, a.recall)
        bm = [n for n, _ in cands]
        r_bm = rank_of(bm, ok)
        stats["in_shortlist"] += r_bm is not None
        stats["bm25@1"] += r_bm == 0
        stats["bm25@3"] += r_bm is not None and r_bm < a.top
        stats["bm25_gold@3"] += top(bm, gold, a.top)
        toks["bm25"].append(cost(sections, bm, a.top))
        toks["grep_all"].append(sum(len(sections[n]["text_en"]) for n in bm[:15]) // 4)
        line = f"{it['id']} bm25={r_bm}"
        if not a.no_jev:
            jv = [n for n, _, _ in lookup.rerank(sections, it["query"], cands)]
            r_j = rank_of(jv, ok)
            stats["jev@1"] += r_j == 0
            stats["jev@3"] += r_j is not None and r_j < a.top
            stats["jev_gold@3"] += top(jv, gold, a.top)
            toks["jev"].append(cost(sections, jv, a.top))
            line += f" jev={r_j} top={jv[:3]}"
        if a.v:
            print(f"{line} gold={sorted(gold)} :: {it['query'][:70]}")

    n = stats["n"]
    print(f"\nlookups: {n} items, recall {a.recall}, shortlist {a.shortlist}, top {a.top}")
    print(f"  gold or accepted in shortlist: {stats['in_shortlist']}/{n}")
    print(f"  BM25 : hit@1 {stats['bm25@1']}/{n}  hit@{a.top} {stats['bm25@3']}/{n}  (strict gold@{a.top} {stats['bm25_gold@3']})")
    if not a.no_jev:
        print(f"  Jev  : hit@1 {stats['jev@1']}/{n}  hit@{a.top} {stats['jev@3']}/{n}  (strict gold@{a.top} {stats['jev_gold@3']})")
    for k, v in toks.items():
        if v:
            print(f"  tokens read, {k:8}: median {sorted(v)[len(v)//2]}, max {max(v)}")

    if a.no_jev or a.no_claims or not data.get("claims"):
        return
    print("\nclaims (expected -> Jev verdict @confidence):")
    for lang in ("en", "fr"):
        conf = Counter()
        for c in data["claims"]:
            if c["section"] not in sections:
                continue
            v = lookup.check_claim(sections, c["claim"], c["section"], lang)
            conf[(c["label"], v["choice"])] += 1
            if a.v:
                mark = "ok " if v["choice"] == c["label"] else "BAD"
                print(f"  [{lang}] {mark} {c['id']} {c['label']:>11} -> {v['choice']:<11} {v['confidence']:.2f}  {c['claim'][:60]}")
        right = sum(v for (e, g), v in conf.items() if e == g)
        print(f"  {lang}: {right}/{sum(conf.values())} correct; " +
              ", ".join(f"{e}->{g}: {v}" for (e, g), v in sorted(conf.items()) if e != g))


if __name__ == "__main__":
    main()
