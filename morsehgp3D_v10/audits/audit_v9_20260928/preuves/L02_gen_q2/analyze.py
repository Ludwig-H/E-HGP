import json,sys,os
D=os.path.dirname(os.path.abspath(__file__))
rows=[json.loads(l) for l in open(os.path.join(D,'campaign.jsonl'))]
def tag(r):
    f=os.path.basename(r['file']); return f.split('_')[0] if f.startswith('s0') else f.split('_')[0]
print('scene n K config | rect leaf% maxF | cand acc c/a | frontProd hTests | cnt_visits vis/cand struct | sibRej poolSel poolFilt | worker_ms_sum payload% | us/site(cpu)')
for r in rows:
    f=r['front']; c=r['census']
    print(tag(r), r['n'], r['K'], r['config'].ljust(6), '|', r['rectangles'], round(100*f['leaf_pairs']/max(1,r['rectangles'])), f['max_factor'], '|',
          r['candidates'], r['accepted'], round(r['candidates']/max(1,r['accepted']),1), '|', f['products'], f['h_tests'], '|',
          c['node_visits'], round(c['node_visits']/max(1,r['candidates']),1), r['order']['structural_splits'], '|',
          r['sibling']['rejected_pairs'], r['pool']['selected_rect'], r['pool']['filtered'], '|',
          round(r['worker_ms_sum']), round(100*r['payload_ms_sum']/max(1e-9,r['worker_ms_sum'])), '|', round(1000*r['worker_ms_sum']/r['n'],1))
