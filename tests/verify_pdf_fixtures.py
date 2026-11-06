"""Strict structural checks for fixture_builder.h output, not a general PDF parser.

Run: python tests/verify_pdf_fixtures.py --fixture-dir build-msvc/tests/runtime
Only original, uncompressed, single-xref generated fixtures are accepted.
No PDF rendering or preservation claim is made by these checks.
"""

import argparse
from pathlib import Path
import re


def require(condition, message):
    if not condition:
        raise ValueError(message)


def replace_fixture_bytes(data, start, end, replacement):
    """Rebase the generated xref after an in-memory dictionary mutation."""
    tail = re.search(rb"startxref\n(\d+)\n%%EOF\n\Z", data)
    xref = int(tail[1])
    require(0 <= start <= end < xref, "control mutation outside objects")
    delta = len(replacement) - (end - start)
    suffix = data[xref:]

    def rebase(entry):
        offset = int(entry[1])
        require(not start < offset < end, "control crosses object boundary")
        return f"{offset + (delta if offset >= end else 0):010d} 00000 n ".encode()

    suffix = re.sub(rb"(?m)^(\d{10}) 00000 n ", rebase, suffix)
    suffix = re.sub(rb"startxref\n\d+", f"startxref\n{xref + delta}".encode(), suffix)
    return data[:start] + replacement + data[end:xref] + suffix


def verify(data, pages, resources=False, mask=False):
    require(data.startswith(b"%PDF-1.7\n"), "header")
    tail = re.search(rb"trailer\n<< /Size (\d+) /Root 1 0 R >>\n"
                     rb"startxref\n(\d+)\n%%EOF\n\Z", data)
    require(tail is not None, "trailer")
    size, xref = map(int, tail.groups())
    require(size == 8 + (pages == 2) + 2 * mask, "object count")
    require(data[xref:xref + 5] == b"xref\n", "startxref offset")
    table = data[xref:tail.start()].splitlines()
    require(table[:3] == [b"xref", f"0 {size}".encode(),
                           b"0000000000 65535 f "], "xref header")
    require(len(table) == size + 2, "xref entry count")
    offsets = []
    for entry in table[3:]:
        require(re.fullmatch(rb"\d{10} 00000 n ", entry), "xref entry")
        offsets.append(int(entry[:10]))
    require(offsets == sorted(set(offsets)), "xref ordering")
    objects = {}
    for number, (start, end) in enumerate(zip(offsets, offsets[1:] + [xref]), 1):
        block = data[start:end]
        prefix = f"{number} 0 obj\n".encode()
        require(block.startswith(prefix) and block.endswith(b"\nendobj\n"),
                "object offset/boundary")
        body = block[len(prefix):-len(b"\nendobj\n")]
        objects[number] = body
        if b"\nstream\n" in body:
            dictionary, payload = body.split(b"\nstream\n", 1)
            lengths = re.findall(rb"/Length\b", dictionary)
            length = re.search(rb"/Length (\d+) >>\Z", dictionary)
            require(len(lengths) == 1 and length is not None, "stream length missing")
            count = int(length[1])
            # fixture_builder includes the final LF in text stream lengths,
            # but emits a separate LF after binary image bytes. Accepting both
            # suffixes hides a text length that is one byte too short.
            image_stream = number not in (4, 7)
            suffix = b"\nendstream" if image_stream else b"endstream"
            require(payload[count:] == suffix and
                    (image_stream or payload[:count].endswith(b"\n")),
                    "stream length")
    for body in objects.values():
        dictionary = body.split(b"\nstream\n", 1)[0]
        for target in re.findall(rb"(\d+) 0 R\b", dictionary):
            require(int(target) in objects, "dangling reference")
    kids = b"3 0 R 8 0 R" if pages == 2 else b"3 0 R"
    require(objects[1] == b"<< /Type /Catalog /Pages 2 0 R >>", "catalog")
    require(objects[2] == b"<< /Type /Pages /Kids [" + kids +
            f"] /Count {pages} >>".encode(), "page tree")
    for page in ([3, 8] if pages == 2 else [3]):
        body = objects[page]
        require(b"/Type /Page /Parent 2 0 R" in body and
                b"/Contents 4 0 R" in body, "page references")
        require(b"/Font << /F1 5 0 R >>" in body and
                b"/XObject << /Im0 6 0 R /Fm0 7 0 R " in body,
                "page resource references")
        require((b"/GSalpha" in body) == resources, "alpha resources")
        if resources:
            require(b"/ExtGState << /GSalpha << /Type /ExtGState "
                    b"/ca 0.5 /CA 0.5 >> >>" in body, "alpha values")
        require((b"/Masked" in body) == mask, "mask resources")
        if mask:
            image = 9 if pages == 2 else 8
            require(f"/Masked {image} 0 R".encode() in body, "mask image reference")
    require(objects[5] == b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
            "font resource")
    require(objects[6].startswith(b"<< /Type /XObject /Subtype /Image /Width 8 /Height 8 "),
            "image resource")
    require(objects[7].startswith(b"<< /Type /XObject /Subtype /Form /BBox [0 0 100 60] "),
            "form resource")
    content = objects[4].split(b"\nstream\n", 1)[1]
    require((b"/GSalpha gs" in content) == resources and
            (b"re W n" in content) == resources, "alpha/clip content")
    require((b"/Masked Do" in content) == mask, "mask content")
    if mask:
        image = 9 if pages == 2 else 8
        require(f"/SMask {image + 1} 0 R".encode() in objects[image], "SMask reference")
        for number, color, pixels in (
            (image, b"DeviceRGB", b"\xff\0\0" * 64),
            (image + 1, b"DeviceGray", (b"\0" * 4 + b"\x80" * 4) * 8),
        ):
            dictionary, payload = objects[number].split(b"\nstream\n", 1)
            require(b"/Subtype /Image /Width 8 /Height 8" in dictionary and
                    b"/BitsPerComponent 8" in dictionary and
                    b"/ColorSpace /" + color in dictionary, "mask image dictionary")
            require(payload == pixels + b"\nendstream", "mask image samples")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture-dir", type=Path, required=True)
    args = parser.parse_args()
    cases = [
        ("artplus_edit_fixture.pdf", 1, False, False),
        ("artplus_edit_multipage_fixture.pdf", 2, False, False),
        ("artplus_edit_resources_fixture.pdf", 1, True, False),
        ("artplus_edit_softmask_fixture.pdf", 1, False, True),
    ]
    for name, pages, resources, mask in cases:
        data = (args.fixture_dir / name).read_bytes()
        verify(data, pages, resources, mask)
        print(f"PASS {name}")
    data = (args.fixture_dir / cases[-1][0]).read_bytes()
    mutations = [
        ("startxref offset", re.sub(rb"startxref\n\d+", b"startxref\n0", data)),
        ("stream length", data.replace(b"/Length 192", b"/Length 191", 1)),
        ("dangling reference", data.replace(b"/SMask 9 0 R", b"/SMask 0 0 R")),
        ("SMask reference", data.replace(b"/SMask 9 0 R", b"/SMask 7 0 R")),
        ("mask image samples", data.replace((b"\0" * 4 + b"\x80" * 4) * 8,
                                             (b"\0" * 4 + b"\x81" * 4) * 8, 1)),
        ("object offset/boundary", data.replace(b"5 0 obj", b"0 0 obj", 1)),
    ]
    controls = [(expected, data, corrupted, 1, False, True)
                for expected, corrupted in mutations]
    # Equal-length substitutions keep xref offsets intact so each control
    # reaches the intended semantic check instead of failing on PDF framing.
    for name, pages, resources, mask in cases:
        original = (args.fixture_dir / name).read_bytes()
        # Corrupt the single-xref envelope independently of object contents.
        # Tail-only edits cannot shift any object offset or startxref value.
        tail = re.search(rb"trailer\n<< /Size (\d+) /Root 1 0 R >>\n"
                         rb"startxref\n(\d+)\n%%EOF\n\Z", original)
        size, xref = map(int, tail.groups())
        table_end = tail.start()
        table = original[xref:table_end]
        entries = table.splitlines(keepends=True)
        envelope_controls = [
            ("header", b"%PDF-1.7\n", b"%PDF-1.6\n"),
            ("xref header", f"0 {size}\n".encode(), f"1 {size}\n".encode()),
            ("xref header", f"0 {size}\n".encode(), f"0 {size - 1}\n".encode()),
            ("xref header", b"0000000000 65535 f ", b"0000000000 00000 f "),
            ("xref header", b"0000000000 65535 f ", b"0000000000 65535 n "),
            ("xref entry", entries[3], entries[3].replace(b"00000 n", b"00001 n")),
            ("xref entry", entries[3], entries[3].replace(b"00000 n", b"00000 f")),
            ("xref entry", entries[3], b"X" + entries[3][1:]),
            ("object count", f"/Size {size}".encode(), f"/Size {size + 1}".encode()),
            ("trailer", b"/Root 1 0 R", b"/Root 2 0 R"),
            ("trailer", b"/Root 1 0 R", b"/Root 1 1 R"),
            ("trailer", b"%%EOF", b"%%EOX"),
        ]
        for expected, before, after in envelope_controls:
            require(original.count(before) == 1, "envelope control target ambiguous")
            controls.append((expected, original, original.replace(before, after, 1),
                             pages, resources, mask))
        tables = [
            ("xref entry count", b"".join(entries[:-1])),
            ("xref entry count", table + entries[-1]),
            ("xref ordering", b"".join(entries[:3] + [entries[4], entries[3]] + entries[5:])),
            ("xref ordering", b"".join(entries[:4] + [entries[3]] + entries[5:])),
        ]
        for expected, replacement in tables:
            controls.append((expected, original,
                             original[:xref] + replacement + original[table_end:],
                             pages, resources, mask))
        for suffix in (b"X", b"\n", b"%%EOF\n"):
            controls.append(("trailer", original, original + suffix,
                             pages, resources, mask))
        # Same-size mutations isolate framing errors without shifting xrefs.
        # Exercise every content/form/image/mask stream independently.
        for match in re.finditer(rb"/Length (\d+) >>\nstream\n", original):
            start, end = match.span(1)
            count = int(match[1])
            payload_start = match.end()
            payload_end = payload_start + count
            # Exercise missing, duplicate and malformed declarations separately.
            # Duplicate keeps a valid terminal declaration: only the uniqueness
            # guard can reject it. Rebase xrefs so insertions reach that guard.
            declaration_start = match.start()
            declaration_end = end
            declaration = original[declaration_start:declaration_end]
            require(replace_fixture_bytes(original, declaration_start,
                                          declaration_end, declaration) == original,
                    "control xref rebase changed original")
            # Leading zero preserves the integer value while growing the PDF;
            # this valid control exercises every rebased object offset.
            grown = replace_fixture_bytes(original, start, end, b"0" + match[1])
            verify(grown, pages, resources, mask)
            declarations = [
                declaration.replace(b"/Length", b"/LengXX"),
                b"/Length 0 " + declaration,
                b"/Length -1",
                b"/Length +1",
                b"/Length 1.0",
                b"/Length (1)",
                b"/Length 1 0 R",
                b"/Length",
            ]
            for replacement in declarations:
                corrupted = replace_fixture_bytes(original, declaration_start,
                                                  declaration_end, replacement)
                controls.append(("stream length missing", original, corrupted,
                                 pages, resources, mask))
            for delta in (-1, 1):
                replacement = str(count + delta).encode()
                require(len(replacement) == end - start, "control changes xref offsets")
                corrupted = original[:start] + replacement + original[end:]
                controls.append(("stream length", original, corrupted, pages, resources, mask))
            # Damage the final payload byte, endstream token and any separate
            # image separator. No corrupt files are written to disk.
            positions = [original.index(b"endstream", payload_end)]
            if original[payload_end:payload_end + 1] == b"\n":
                positions.append(payload_end)
            else:
                # Binary samples are validated separately; only text streams
                # require their last payload byte to be a line feed.
                require(original[payload_end - 1:payload_end] == b"\n", "control payload boundary")
                positions.append(payload_end - 1)
            for position in positions:
                corrupted = original[:position] + b"X" + original[position + 1:]
                controls.append(("stream length", original, corrupted, pages, resources, mask))
        replacements = [
            ("page tree", b"/Count " + str(pages).encode(), b"/Count 0"),
            ("page tree", b"/Kids [3 0 R", b"/Kids [5 0 R"),
            ("page references", b"/Parent 2 0 R", b"/Parent 1 0 R"),
            ("page references", b"/Contents 4 0 R", b"/Contents 6 0 R"),
            ("page resource references", b"/F1 5 0 R", b"/F1 6 0 R"),
            ("page resource references", b"/Im0 6 0 R", b"/Im0 7 0 R"),
            ("page resource references", b"/Fm0 7 0 R", b"/Fm0 6 0 R"),
            ("font resource", b"/BaseFont /Helvetica", b"/BaseFont /CourierXX"),
            ("image resource", b"/Subtype /Image", b"/Subtype /ImagX"),
            ("form resource", b"/Subtype /Form", b"/Subtype /ForX"),
        ]
        if pages == 2:
            replacements.append(("page tree", b"/Kids [3 0 R 8 0 R]",
                                 b"/Kids [3 0 R 3 0 R]"))
        if resources:
            replacements.extend([
                ("alpha values", b"/ca 0.5", b"/ca 1.0"),
                ("alpha values", b"/CA 0.5", b"/CA 1.0"),
                ("alpha resources", b"/GSalpha <<", b"/GSother <<"),
            ])
        if mask:
            replacements.append(("mask image reference", b"/Masked 8 0 R",
                                 b"/Masked 6 0 R"))
        for expected, before, after in replacements:
            require(len(before) == len(after), "control changes xref offsets")
            controls.append((expected, original, original.replace(before, after),
                             pages, resources, mask))
            if pages == 2 and expected in ("page references", "page resource references"):
                # Corrupt only page two: validating page one must not hide it.
                position = original.rfind(before)
                require(position >= 0, "second-page control target missing")
                corrupted = original[:position] + after + original[position + len(before):]
                controls.append((expected, original, corrupted, pages, resources, mask))
    for expected, original, corrupted, pages, resources, mask in controls:
        require(corrupted != original, "negative control did not mutate input")
        try:
            verify(corrupted, pages, resources, mask)
        except ValueError as error:
            require(str(error) == expected, f"wrong rejection: {error}; expected {expected}")
        else:
            raise ValueError(f"negative control accepted: {expected}")
        print(f"PASS rejected {expected}")
    print(f"{len(cases)} fixtures and {len(controls)} negative controls passed (structure only)")


if __name__ == "__main__":
    main()
