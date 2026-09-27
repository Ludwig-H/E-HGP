#!/usr/bin/env python3
"""Close the small deterministic gates before running the synthetic campaign."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import traceback

from prepare import need, save, sha
from run import sources, check_pins, validate_qualification, QUALIFICATION, THREAD_ENV
from test_unit_pipeline import proof_paths

HERE=Path(__file__).resolve().parent
TESTS=('test_synthetic_data.py','test_prepare.py','test_unit_pipeline.py','test_run.py')


def qualify(output):
    need(not output.exists(),'NEW qualification directory required')
    output.mkdir(parents=True)
    pins=sources()
    for path in [Path(__file__).resolve()]+[HERE/name for name in TESTS]+proof_paths():
        pins[str(path)]=sha(path)
    receipt=dict(schema='mhgp9_synthetic_clustering_qualification_v1',status='running',
        sources_before=pins,commands=[],argv=sys.argv,python=sys.version,
        native_execution=False,native_fixture_transport='replayed_only',GCP_used=False)
    save(output/'intent.json',receipt)
    start=time.monotonic()
    try:
        receipt['inherited_pins']=validate_qualification(QUALIFICATION)
        check_pins(pins)
        for optimized in (False,True):
            for name in TESTS:
                stem=Path(name).stem+('_optimized' if optimized else '_normal')
                argv=[sys.executable,'-B']+(['-O'] if optimized else [])+[str(HERE/name)]
                began=time.monotonic()
                with (output/(stem+'.stdout')).open('xb') as out, (output/(stem+'.stderr')).open('xb') as err:
                    result=subprocess.run(argv,stdout=out,stderr=err,cwd=HERE,
                        env=dict(os.environ,**THREAD_ENV,PYTHONDONTWRITEBYTECODE='1'),check=False)
                command=dict(argv=argv,returncode=result.returncode,elapsed_seconds=time.monotonic()-began,
                    stdout=str(output/(stem+'.stdout')),stderr=str(output/(stem+'.stderr')))
                save(output/(stem+'.command.json'),command);receipt['commands'].append(command)
                print(stem,result.returncode,flush=True)
                need(result.returncode==0,'qualification gate failed: '+stem)
        check_pins(pins);check_pins(receipt['inherited_pins'])
        receipt['status']='completed'
    except BaseException as error:
        receipt.update(status='failed',error=repr(error),traceback=traceback.format_exc())
        raise
    finally:
        receipt['sources_after']={path:sha(path) for path in pins}
        if receipt['sources_after']!=pins:
            receipt.update(status='failed',closure_error='sources changed')
        receipt['artifacts']={str(path):sha(path) for path in output.iterdir() if path.is_file()}
        receipt['elapsed_seconds']=time.monotonic()-start
        save(output/'receipt.json',receipt)
    need(receipt['status']=='completed','qualification closure')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    qualify(parser.parse_args().output.resolve())
