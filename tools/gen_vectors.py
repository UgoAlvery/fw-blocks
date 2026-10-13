#!/usr/bin/env python3
"""Oracle de reference des vecteurs de test de fwb_framing.

Ce script est DELIBEREMENT independant du code du projet : il reimplemente
le CRC, COBS et le format de trame a partir de la specification seule, dans
un autre langage. Des vecteurs produits par le code qu'ils sont censes
valider ne valident rien.

Usage :
    gen_vectors.py --write    reecrit framing/tests/vectors/vectors.txt
    gen_vectors.py --check    verifie que le fichier versionne est a jour

Le mode --check est execute par ctest et par la CI. Il echoue si quelqu'un
edite vectors.txt a la main sans passer par l'oracle, ou si l'oracle est
modifie sans regenerer le fichier.
"""

from __future__ import annotations

import argparse
import difflib
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
VECTORS = ROOT / "framing" / "tests" / "vectors" / "vectors.txt"


# ------------------------------------------------------------------ CRC

def reflect(value: int, bits: int) -> int:
    out = 0
    for i in range(bits):
        if value & (1 << i):
            out |= 1 << (bits - 1 - i)
    return out


def crc_bitwise(data: bytes, width: int, poly: int, init: int,
                refin: bool, refout: bool, xorout: int) -> int:
    """Modele de Rocksoft, implementation bit a bit. Voir ADR-0005."""
    topbit = 1 << (width - 1)
    mask = (1 << width) - 1
    crc = init
    for byte in data:
        b = reflect(byte, 8) if refin else byte
        crc ^= (b << (width - 8)) & mask
        for _ in range(8):
            crc = ((crc << 1) ^ poly) & mask if (crc & topbit) else (crc << 1) & mask
    if refout:
        crc = reflect(crc, width)
    return crc ^ xorout


CRC16_CCITT_FALSE = dict(width=16, poly=0x1021, init=0xFFFF,
                         refin=False, refout=False, xorout=0x0000)
CRC32C = dict(width=32, poly=0x1EDC6F41, init=0xFFFFFFFF,
              refin=True, refout=True, xorout=0xFFFFFFFF)


def crc16(data: bytes) -> int:
    return crc_bitwise(data, **CRC16_CCITT_FALSE)


# ------------------------------------------------------------------ COBS

def cobs_encode(data: bytes) -> bytes:
    """COBS, delimiteur 0x00. Voir ADR-0006 et wire-format.md §6."""
    out = bytearray()
    idx = 0
    while True:
        run = bytearray()
        while idx < len(data) and data[idx] != 0 and len(run) < 254:
            run.append(data[idx])
            idx += 1
        out.append(len(run) + 1)
        out.extend(run)
        if idx < len(data) and data[idx] == 0:
            idx += 1
            if idx == len(data):
                out.append(0x01)   # zero final : un bloc vide le represente
                break
            continue
        if len(run) == 254 and idx < len(data):
            continue
        break
    return bytes(out)


def cobs_decode(data: bytes) -> bytes:
    out = bytearray()
    idx = 0
    while idx < len(data):
        code = data[idx]
        if code == 0:
            raise ValueError("octet nul interdit dans une zone encodee")
        idx += 1
        end = idx + code - 1
        if end > len(data):
            raise ValueError("code COBS depassant la fin de la zone")
        out.extend(data[idx:end])
        idx = end
        if code < 255 and idx < len(data):
            out.append(0)
    return bytes(out)


# ------------------------------------------------------------------ Framing

VER = 0x01
HEADER_LEN = 7
CRC_LEN = 2
MIN_FRAME = 9
MAX_PAYLOAD = 256


def build_frame(type_: int, seq: int, payload: bytes, link_id: int = 0x0000,
                ver: int = VER, flags: int = 0x00,
                len_override: int | None = None) -> bytes:
    """Construit une trame DECODEE, conformement a wire-format.md §2."""
    length = len(payload) if len_override is None else len_override
    header = bytes([
        ver,
        flags,
        type_,
        seq & 0xFF, (seq >> 8) & 0xFF,
        length & 0xFF, (length >> 8) & 0xFF,
    ])
    protected = header + payload
    crc_input = bytes([link_id & 0xFF, (link_id >> 8) & 0xFF]) + protected
    crc = crc16(crc_input)
    return protected + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


def on_wire(frame: bytes) -> bytes:
    return cobs_encode(frame) + b"\x00"


def hexs(b: bytes) -> str:
    return " ".join(f"{x:02X}" for x in b)


def wrap(b: bytes, per: int = 16, indent: str = "    ") -> str:
    return "\n".join(indent + hexs(b[i:i + per]) for i in range(0, len(b), per))


# ------------------------------------------------------------------ Auto-tests

def selftest() -> None:
    """Valide l'oracle lui-meme avant de produire quoi que ce soit."""
    check = b"123456789"

    c16 = crc16(check)
    assert c16 == 0x29B1, f"CRC-16/CCITT-FALSE = {c16:#06X}, attendu 0x29B1"
    c32 = crc_bitwise(check, **CRC32C)
    assert c32 == 0xE3069283, f"CRC-32C = {c32:#010X}, attendu 0xE3069283"

    samples = [b"", b"\x00", b"\x00\x00", b"\x01\x02\x00\x03",
               bytes(range(256)), b"\x11" * 300, b"\x00" * 300,
               bytes(254), bytes([1] * 253), bytes([1] * 254), bytes([1] * 255)]
    for s in samples:
        enc = cobs_encode(s)
        assert 0 not in enc, f"octet nul en sortie COBS pour {hexs(s[:8])}…"
        assert cobs_decode(enc) == s, f"aller-retour COBS rompu pour {hexs(s[:8])}…"
        overhead = len(enc) - len(s)
        assert overhead <= 1 + len(s) // 254, (
            f"surcout COBS {overhead} hors borne pour {len(s)} octets")


# ------------------------------------------------------------------ Vecteurs

def vectors() -> tuple[list[dict], list[dict]]:
    """Retourne (vecteurs complets, vecteurs bruts niveau delimitation)."""
    full: list[dict] = []

    def add(ident, title, frame, expect, note=""):
        full.append(dict(id=ident, title=title, frame=frame,
                         wire=on_wire(frame), expect=expect, note=note))

    add("FRM-V-001", "Payload vide",
        build_frame(0x10, 0, b""), "FWB_OK",
        "TYPE=0x10 SEQ=0 LEN=0 link_id=0x0000")
    add("FRM-V-002", "Payload ASCII 'ABC'",
        build_frame(0x20, 1, b"ABC"), "FWB_OK",
        "TYPE=0x20 SEQ=1 LEN=3 link_id=0x0000 — vecteur de reference du format")
    add("FRM-V-003", "Payload de trois octets nuls",
        build_frame(0x20, 2, b"\x00\x00\x00"), "FWB_OK",
        "exerce le decoupage COBS : chaque zero ferme un bloc")
    add("FRM-V-004", "SEQ a sa valeur maximale",
        build_frame(0x30, 0xFFFF, bytes([0, 0, 0x80, 0x3F])), "FWB_OK",
        "SEQ=0xFFFF, la trame suivante repasse a 0 ; payload = motif binaire de 1.0f")
    add("FRM-V-005", "Payload maximal de 256 octets",
        build_frame(0x40, 3, bytes(range(256))), "FWB_OK",
        "payload = 0x00..0xFF ; trame la plus longue possible, COBS a plus d'un bloc")
    add("FRM-V-006", "Meme trame que FRM-V-002 avec link_id=0xBEEF",
        build_frame(0x20, 1, b"ABC", link_id=0xBEEF), "FWB_OK",
        "seul le CRC differe de FRM-V-002 : demontre l'identifiant de lien (ADR-0007)")

    bad_crc = bytearray(build_frame(0x20, 1, b"ABC"))
    bad_crc[-2] ^= 0x01
    add("FRM-I-001", "CRC errone", bytes(bad_crc), "FWB_ERR_BAD_CRC",
        "bit 0 du CRC inverse par rapport a FRM-V-002")

    bad_pl = bytearray(build_frame(0x20, 1, b"ABC"))
    bad_pl[8] ^= 0x80
    add("FRM-I-002", "Payload corrompu", bytes(bad_pl), "FWB_ERR_BAD_CRC",
        "bit 7 du premier octet de payload inverse ('B' -> 0xC2)")

    add("FRM-I-003", "LEN incoherent",
        build_frame(0x20, 1, b"ABC", len_override=4), "FWB_ERR_LENGTH_MISMATCH",
        "LEN=4 annonce, 3 octets presents ; controle 3 avant le CRC")
    add("FRM-I-004", "Version inconnue",
        build_frame(0x20, 1, b"ABC", ver=0x02), "FWB_ERR_BAD_VERSION",
        "VER=0x02, CRC volontairement valide : le controle 5 s'applique apres le CRC")
    add("FRM-I-005", "Drapeau reserve positionne",
        build_frame(0x20, 1, b"ABC", flags=0x01), "FWB_ERR_BAD_FLAGS",
        "FLAGS=0x01 (bit MAC), CRC volontairement valide")
    add("FRM-I-006", "LEN superieur au maximum",
        build_frame(0x20, 1, b"ABC", len_override=257), "FWB_ERR_PAYLOAD_TOO_LARGE",
        "LEN=0x0101=257 > 256 ; controle 2, avant le controle de coherence")
    add("FRM-I-007", "Trame tronquee",
        build_frame(0x20, 1, b"ABC")[:6], "FWB_ERR_FRAME_TOO_SHORT",
        "6 octets, minimum structurel 9")

    raw = [
        dict(id="FRM-I-008", title="Code COBS hors limites",
             wire=bytes([0x05, 0x01, 0x02, 0x00]), expect="FWB_ERR_COBS_MALFORMED",
             note="le code 0x05 designe un bloc depassant la fin de la zone encodee"),
        dict(id="FRM-I-009", title="Trame decodee trop courte",
             wire=cobs_encode(b"\x01\x02\x03") + b"\x00",
             expect="FWB_ERR_FRAME_TOO_SHORT",
             note="COBS valide, 3 octets decodes, minimum 9"),
        dict(id="FRM-I-010", title="Delimiteur isole",
             wire=bytes([0x00]), expect="FWB_OK",
             note="trame vide : ignoree silencieusement, aucune erreur, aucune livraison"),
        dict(id="FRM-I-011", title="Delimiteur premature",
             wire=bytes([0x02, 0x01, 0x00, 0x00]), expect="FWB_ERR_FRAME_TOO_SHORT",
             note="la trame se termine a l'offset 2 : 1 octet decode"),
    ]
    return full, raw


def render() -> str:
    full, raw = vectors()

    # Controles de coherence sur ce qui va etre ecrit.
    for v in full:
        assert v["wire"][-1] == 0x00, f"{v['id']} : pas de delimiteur final"
        assert 0 not in v["wire"][:-1], f"{v['id']} : octet nul avant le delimiteur"
        assert cobs_decode(v["wire"][:-1]) == v["frame"], f"{v['id']} : aller-retour"

    longest = max(len(v["wire"]) for v in full)
    assert longest == 268, f"trame maximale {longest}, attendu 268"

    out = [
        "# Vecteurs de reference fw-blocks / framing",
        "#",
        "# GENERE par tools/gen_vectors.py — ne pas editer a la main.",
        "# Regenerer : python3 tools/gen_vectors.py --write",
        "#",
        "# DECODED = trame apres decodage COBS (wire-format.md §2).",
        "# WIRE    = octets sur le fil, delimiteur 0x00 inclus (§6).",
        "# EXPECT  = code de retour attendu au decodage. Le verifier exactement :",
        "#           un decodeur qui rejette tout passe un test qui n'exige",
        "#           qu'un rejet. Voir ADR-0004.",
        "",
    ]
    for v in full:
        out += [f"[{v['id']}]",
                f"title  = {v['title']}",
                f"expect = {v['expect']}",
                f"note   = {v['note']}",
                f"decoded_len = {len(v['frame'])}", "decoded =",
                wrap(v["frame"]),
                f"wire_len = {len(v['wire'])}", "wire =",
                wrap(v["wire"]), ""]
    for v in raw:
        out += [f"[{v['id']}]",
                f"title  = {v['title']}",
                f"expect = {v['expect']}",
                f"note   = {v['note']}",
                f"wire_len = {len(v['wire'])}", "wire =",
                wrap(v["wire"]), ""]
    return "\n".join(out)


# ------------------------------------------------------------------ Entree

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--write", action="store_true",
                   help="reecrit le fichier de vecteurs")
    g.add_argument("--check", action="store_true",
                   help="verifie que le fichier versionne est a jour")
    args = ap.parse_args()

    selftest()
    content = render()

    if args.write:
        VECTORS.parent.mkdir(parents=True, exist_ok=True)
        VECTORS.write_text(content, encoding="utf-8")
        print(f"ecrit {VECTORS.relative_to(ROOT)}")
        return 0

    if not VECTORS.exists():
        print(f"ERREUR : {VECTORS.relative_to(ROOT)} est absent.", file=sys.stderr)
        print("Lancer : python3 tools/gen_vectors.py --write", file=sys.stderr)
        return 1

    current = VECTORS.read_text(encoding="utf-8")
    if current == content:
        full, raw = vectors()
        print(f"vecteurs a jour : {len(full) + len(raw)} vecteurs, "
              f"auto-tests de l'oracle OK")
        return 0

    print(f"ERREUR : {VECTORS.relative_to(ROOT)} differe de l'oracle.",
          file=sys.stderr)
    diff = difflib.unified_diff(current.splitlines(), content.splitlines(),
                                "versionne", "oracle", lineterm="", n=2)
    for line in list(diff)[:60]:
        print(line, file=sys.stderr)
    print("\nRegenerer : python3 tools/gen_vectors.py --write", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
