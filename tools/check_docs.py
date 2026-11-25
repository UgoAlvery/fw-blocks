#!/usr/bin/env python3
"""Controles de coherence de la documentation de fw-blocks.

Ce que ce script verifie, et qu'aucune relecture ne fait de maniere fiable :

  1. tout lien relatif entre documents pointe vers un fichier existant ;
  2. tout identifiant d'exigence cite quelque part est defini quelque part ;
  3. tout ADR cite existe ;
  4. tout code d'erreur cite est declare dans docs/conventions.md ;
  5. toute exigence definie apparait dans la matrice de tracabilite ;
  6. le compte annonce par la matrice est exact ;
  7. la couverture : toute exigence dont la verification n'est pas purement
     documentaire porte son identifiant dans un fichier de test.

Les controles 1 a 6 font toujours echouer le script. Le controle 7 ne
rapporte qu'un taux de couverture par brique, sauf avec --strict-tests.

Ce choix est deliberе : une couverture bloquante des le premier test ecrit
rendrait la CI rouge pendant tout le developpement d'une brique, et la
reaction naturelle serait de desactiver le controle. La couverture est donc
un indicateur en continu et un critere de sortie ponctuel. --strict-tests
correspond au critere « brique terminee » de docs/test-plan.md §7 ; il est
lance avant un marquage de version, pas a chaque commit.

Usage : check_docs.py [--strict-tests] [--brick <nom>]
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

REQ_FILES = ("crc/docs/requirements.md",
             "framing/docs/requirements.md",
             "antireplay/docs/requirements.md")

LINK = re.compile(r"\[[^\]]*\]\(([^)#]+?)\)")
REQ_DEF = re.compile(r"^\|\s*((?:CRC|FRM|AR)-REQ-\d{3})\s*\|")
REQ_ANY = re.compile(r"(?:CRC|FRM|AR)-REQ-\d{3}")
REQ_ROW = re.compile(r"^\|\s*((?:CRC|FRM|AR)-REQ-\d{3})\s*\|[^|]*\|[^|]*\|\s*([A-Z, ]+?)\s*\|")
ADR_USE = re.compile(r"ADR-(\d{4})")
CODE_USE = re.compile(r"FWB_(?:OK|ERR_[A-Z_]+)\b")
CODE_DEF = re.compile(r"^(FWB_[A-Z_]+)\s+=", re.M)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--strict-tests", action="store_true",
                    help="rend la couverture bloquante (critere de sortie)")
    ap.add_argument("--brick", metavar="NOM", action="append",
                    help="limite --strict-tests a cette brique, repetable")
    args = ap.parse_args()

    docs = sorted(p for p in ROOT.rglob("*.md")
                  if "build" not in p.parts and "_deps" not in p.parts)
    errors: list[str] = []
    text = {p: p.read_text(encoding="utf-8") for p in docs}

    def rel(p: pathlib.Path) -> str:
        return str(p.relative_to(ROOT))

    # 1. liens relatifs
    for p in docs:
        for target in LINK.findall(text[p]):
            if target.startswith(("http://", "https://", "mailto:")):
                continue
            if not (p.parent / target).resolve().exists():
                errors.append(f"lien mort        {rel(p)} -> {target}")

    # 2. identifiants d'exigence
    defined: set[str] = set()
    verif: dict[str, str] = {}
    for name in REQ_FILES:
        for line in (ROOT / name).read_text(encoding="utf-8").splitlines():
            m = REQ_DEF.match(line)
            if m:
                defined.add(m.group(1))
            m = REQ_ROW.match(line)
            if m:
                verif[m.group(1)] = m.group(2)

    for p in docs:
        for rid in sorted(set(REQ_ANY.findall(text[p]))):
            if rid not in defined:
                errors.append(f"exigence inconnue {rel(p)} cite {rid}")

    # 3. ADR
    adrs = {q.name.split("-")[0] for q in (ROOT / "docs/adr").glob("*.md")}
    for p in docs:
        for num in sorted(set(ADR_USE.findall(text[p]))):
            if num not in adrs:
                errors.append(f"ADR inconnu       {rel(p)} cite ADR-{num}")

    # 4. codes d'erreur
    conv = ROOT / "docs/conventions.md"
    codes = set(CODE_DEF.findall(text[conv]))
    for p in docs:
        for c in sorted(set(CODE_USE.findall(text[p]))):
            if c not in codes:
                errors.append(f"code inconnu      {rel(p)} cite {c}")

    # 5 et 6. matrice de tracabilite
    trace = ROOT / "docs/traceability.md"
    traced = set(REQ_ANY.findall(text[trace]))
    for rid in sorted(defined - traced):
        errors.append(f"non tracee        {rid} absente de la matrice")

    claimed = re.search(r"Exigences spécifiées \| (\d+)", text[trace])
    if not claimed:
        errors.append("matrice           compte d'exigences introuvable")
    elif int(claimed.group(1)) != len(defined):
        errors.append(f"matrice           annonce {claimed.group(1)} exigences, "
                      f"{len(defined)} definies")

    # 7. couverture par les tests, par brique
    test_src = [p for p in ROOT.rglob("*.c")
                if any(d in p.parts for d in ("tests", "fuzz"))
                and "build" not in p.parts and "_deps" not in p.parts]
    joined = "\n".join(p.read_text(encoding="utf-8", errors="replace")
                       for p in test_src)
    cited = set(REQ_ANY.findall(joined))

    # Une exigence verifiee par la seule inspection n'a pas de test a porter
    # son identifiant : l'exiger produirait un commentaire decoratif.
    testable = {r for r in defined if verif.get(r, "").strip() != "INSP"}

    prefix_to_brick = {"CRC": "crc", "FRM": "framing", "AR": "antireplay"}
    by_brick: dict[str, list[str]] = {}
    for rid in sorted(testable):
        by_brick.setdefault(prefix_to_brick[rid.rsplit("-REQ-")[0]], []).append(rid)

    print(f"documents markdown     : {len(docs)}")
    print(f"exigences definies     : {len(defined)}")
    print(f"exigences tracees      : {len(traced & defined)}")
    print(f"ADR presents           : {len(adrs)}")
    print(f"codes d'erreur declares: {len(codes)}")
    print(f"fichiers de test       : {len(test_src)}")
    print()
    print("Couverture par brique (exigences non documentaires) :")
    for brick, rids in sorted(by_brick.items()):
        done = sum(1 for r in rids if r in cited)
        pct = 100 * done / len(rids)
        mark = "complet" if done == len(rids) else ""
        print(f"  {brick:<12} {done:>3}/{len(rids):<3} {pct:5.1f} %  {mark}")
    insp = len(defined) - len(testable)
    print(f"  {'(inspection)':<12} {insp:>3} exigence(s) hors couverture de test")
    print()

    if args.strict_tests:
        targets = set(args.brick) if args.brick else set(by_brick)
        unknown = targets - set(by_brick)
        if unknown:
            errors.append(f"brique inconnue   {sorted(unknown)}")
        for brick in sorted(targets & set(by_brick)):
            for rid in by_brick[brick]:
                if rid not in cited:
                    errors.append(
                        f"sans test         {rid} n'apparait dans aucun test")

    if errors:
        print(f"{len(errors)} probleme(s) :", file=sys.stderr)
        for e in sorted(set(errors)):
            print("  -", e, file=sys.stderr)
        return 1

    print("aucun probleme detecte")
    return 0


if __name__ == "__main__":
    sys.exit(main())
