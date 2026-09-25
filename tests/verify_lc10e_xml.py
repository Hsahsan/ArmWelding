"""Verify compiled profile against the user-supplied ESI, without any hardware."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
esi = ET.parse(root / "LC10E V1.04.xml").getroot()
header = (root / "src/lc10e_esi.h").read_text()


def number(text):
    return int(text.replace("#x", "0x"), 0)


def constant(name):
    return int(re.search(r"\b" + name + r"\s*=\s*(0x[0-9a-f]+|\d+)", header)[1], 0)


devices = esi.findall("./Descriptions/Devices/Device")
assert len(devices) == 1
device = devices[0]
kind = device.find("Type")
assert number(esi.findtext("./Vendor/Id")) == constant("vendor")
assert number(kind.get("ProductCode")) == constant("product")
assert number(kind.get("RevisionNo")) == constant("revision")
for tag, name, size in [("RxPdo", "rx", "output"), ("TxPdo", "tx", "input")]:
    pdo = next(p for p in device.findall(tag) if number(p.findtext("Index")) == constant(name + "_index"))
    assert pdo.get("Fixed") == "1"
    descriptors = [(number(e.findtext("Index")) << 16) |
                   (number(e.findtext("SubIndex")) << 8) | number(e.findtext("BitLen"))
                   for e in pdo.findall("Entry")]
    array = re.search(r"\b" + name + r"\[\]\s*=\s*\{([^}]+)\}", header)[1]
    assert descriptors == [int(n, 16) for n in re.findall(r"0x[0-9a-f]+", array)]
    assert sum(e & 0xff for e in descriptors) == constant(size + "_bytes") * 8
for sm, name in zip(device.findall("Sm")[2:4], ["output", "input"]):
    assert number(sm.get("StartAddress")) == constant(name + "_address")
    assert number(sm.get("ControlByte")) == (constant(name + "_flags") & 0xff)
    assert number(sm.get("Enable")) == ((constant(name + "_flags") >> 16) & 1)
print("PASS: profile identity, fixed descriptors, 15/28-byte lengths and SM settings match supplied XML.")
