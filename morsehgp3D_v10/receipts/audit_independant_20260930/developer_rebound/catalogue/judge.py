import json
from pathlib import Path
import sys


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def run(path):
    rows=Path(path).read_text().splitlines()
    require(len(rows)==1,'one native output required')
    row=json.loads(rows[0]);model=row['abstract_model']
    require(row['status']=='PASS' and row['checks']>250,'native boundary panel missing')
    require(model['balls']==20000000 and model['atlas_cells']==200000000 and
            model['ext_cells']==200000000 and model['ext_join_cells']==60000000,
            'virtual cardinal model changed')
    counts=[20000000*x for x in (58,74,92)]
    total=sum(counts);first=(20000000-1)*224+58+74
    require(model['perK_representatives']==counts and all(c<2**32-1 for c in counts),
            'per-K domain model invalid')
    require(model['global_representatives']==total and total==4480000000,'global count mismatch')
    require(model['last_span_first']==first and model['old_first']==first%(2**32),
            'ball-major prefix truncation mismatch')
    require(model['old_last_index']==(first+91)%(2**32) and
            model['exact_last_index']==total-1 and model['old_last_index']!=total-1,
            'legacy alias mismatch')
    require(row['add_wrap_old']==9,'addition-only wrapping mismatch')
    require(row['actual_geometry_realized'] is False and row['product_tower_executed'] is False and
            row['GCP'] is False,'scope must remain a virtual cardinal model')
    return dict(status='PASS',native_checks=row['checks'],
                exact_last_index=total-1,old_alias=(first+91)%(2**32),
                scope='scalar representation/guard implication only; no realized geometric catalogue')


if __name__=='__main__':
    print(json.dumps(run(sys.argv[1]),sort_keys=True))
