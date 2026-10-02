#!/usr/bin/env python3
"""Frozen developer index reader; redirect only original-receipt path to the preserved local bytes."""
from pathlib import Path
import importlib.util,json
ROOT=Path(__file__).resolve().parent
RECEIPTS=ROOT/'copies/morsehgp3D_v11/receipts'
spec=importlib.util.spec_from_file_location('index_reader',RECEIPTS/'index_20261002/check.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
compact=json.loads((RECEIPTS/'index_20261002/index1/receipt.json').read_text())
redirect={compact['raw_receipt_local']:ROOT/'raw/index1_receipt.json'}
class ReceiptPath(type(Path())):
    def read_bytes(self):
        original=redirect.get(str(self))
        return original.read_bytes() if original is not None else super().read_bytes()
reader.old.Path=ReceiptPath
if __name__=='__main__':reader.main()
