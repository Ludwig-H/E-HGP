"""Run only the frozen final reader delta against frozen region1 receipts."""
import argparse
import copy
import json
from pathlib import Path
import sys
from types import ModuleType

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
import review

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
reader=ModuleType('region_reader_final_copy')
# Dependency-location adapter only. All code bytes executed come from check.py.source.txt.
reader.__file__=str(review.HERE/'raw/morsehgp3D_v11/receipts/center_region_20261002/check.py')
exec(compile((HERE/'check.py.source.txt').read_bytes(),str(HERE/'check.py.source.txt'),'exec'),reader.__dict__)
result, report=review.qualification(reader)
receipt,worker,data=reader.old.read_capture(review.CAPTURE)
manifest,manifest_hash=reader.old.inputs(review.CAPTURE,receipt)
counts,builds,code=reader.matrices(data,reader.contract())
hashes={name:review.sha(data[reader.BASE+name+'/build_provenance.json']) for name in reader.profiles.PROFILES.values()}
arguments=(manifest,manifest_hash,review.sha(data[reader.BASE+'summary.json']),builds,hashes,
           review.sha(data[reader.q4.SUPPLEMENT+'summary.json']))
tests=[]
def refusal(name, function):
    try:
        function()
    except reader.ERRORS as error:
        tests.append(dict(case=name,refused=True,cause=str(error)))
    else:
        raise ValueError('delta corruption accepted: '+name)
bad=copy.deepcopy(report);bad['not_run'].pop(0)
refusal('older_causal_omission_removed',lambda:reader.report_judge(bad,*arguments))
meta=reader.fields(data['results/cmd/002_profiles/meta.txt'])
bad=copy.deepcopy(meta);bad['exit_code']='0'
refusal('timeout_with_success_exit_code',lambda:reader.command_metadata(bad,820))
bad=copy.deepcopy(meta);bad['streams_truncated']='1'
refusal('truncated_command_streams',lambda:reader.command_metadata(bad,820))
result['delta_checks']=tests
result['reader_sha256']=review.sha((HERE/'check.py.source.txt').read_bytes())
result['scope']='pure Python copied latest reader only; no native/build/GCP; final reader still reports preserved failed campaign'
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
print('delta_review_ok original_corruptions=7 additional_corruptions=3 native_runs=0')
