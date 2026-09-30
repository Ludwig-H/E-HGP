"""Capture one existing native u18 export with explicit dependencies; never builds."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys


def main():
    p = argparse.ArgumentParser()
    for name in ('exporter','library','source-tree','build-receipt','closure-helper'):
        p.add_argument('--'+name, type=Path, required=True)
    args = p.parse_args()
    root = Path(__file__).resolve().parent
    module = importlib.util.spec_from_file_location('inherited_closure', args.closure_helper)
    helper = importlib.util.module_from_spec(module)
    module.loader.exec_module(helper)
    before = helper.closure(args, args.closure_helper.parent)
    before[str(Path(__file__).resolve())] = helper.sha(Path(__file__).resolve())
    sys.path.insert(0, str(root/'source_snapshot'))
    import frontier_core as fc
    cloud, export = root/'fixtures/triangle_default.u32le', root/'fixtures/triangle_default.json'
    if cloud.exists() or export.exists():
        raise RuntimeError('refuse to overwrite a capture')
    fc.write_cloud([(0,0,0),(8,0,0),(0,3,0)], cloud)
    code, summary, stdout, stderr, argv = fc.run_exporter(
        str(args.exporter), str(cloud), str(export), 2, threads=1, timeout=10)
    after = helper.closure(args, args.closure_helper.parent)
    after[str(Path(__file__).resolve())] = helper.sha(Path(__file__).resolve())
    if before != after or code != 0:
        raise RuntimeError('native capture failed or dependencies changed')
    report = {'status':'PASS','profile':'quantized_u18_input_only','native_argv':argv,
              'native_returncode':code,'native_stdout':stdout,'native_stderr':stderr,
              'cloud_sha256':helper.sha(cloud),'export_sha256':helper.sha(export),
              'closure_before':before,'closure_after':after,'GCP_used':False,
              'limits':'one three-site export; inherited existing build, no compilation'}
    (root/'triangle_default_capture.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'status':'PASS','sites':3,'dependencies':len(before)},sort_keys=True))


if __name__ == '__main__':
    main()
