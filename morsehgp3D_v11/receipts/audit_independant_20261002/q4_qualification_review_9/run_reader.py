#!/usr/bin/env python3
"""Replay frozen developer reader; redirect only two original-receipt paths into this capsule."""
from pathlib import Path
import importlib.util,json
ROOT=Path(__file__).resolve().parent
RECEIPTS=ROOT/'copies/morsehgp3D_v11/receipts'
spec=importlib.util.spec_from_file_location('q4_reader',RECEIPTS/'catalogue_q4_20261002/check.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
redirect={}
for folder,name in [('catalogue_q4_20261002/q4levels1','q4levels1'),('catalogue_profiles_20261002/profiles1','profiles1')]:
    compact=json.loads((RECEIPTS/folder/'receipt.json').read_text())
    redirect[compact['raw_receipt_local']]=ROOT/'raw'/(name+'_receipt.json')
class ReceiptPath(type(Path())):
    def read_bytes(self):
        source=redirect.get(str(self))
        return source.read_bytes() if source is not None else super().read_bytes()
reader.old.Path=ReceiptPath
if __name__=='__main__':reader.main()
