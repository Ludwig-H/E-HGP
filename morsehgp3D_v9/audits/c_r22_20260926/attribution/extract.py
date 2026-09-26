import json, sys
ROOT = '/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/'
R = {'r21': ROOT + 'g4_tower_r21_20260925/vm/', 'r22': ROOT + 'g4_tower_r22_20260926/vm/'}


def load(r, i):
    return json.loads(open(R[r] + f'probe_{i}.stdout').read())


def g(dct, k, default=0.0):
    v = dct.get(k, default)
    return default if v is None else v


def phases(d):
    t = d['times_ms']; b = d.get('q34_batch', {}); tp = d['tower_phases_ms']; td = d.get('tower_detail', {})
    lv = d['options']['levers']
    P = {}
    P['prepare'] = t['prepare']; P['gen_index'] = t['gen_index']
    P['q2_own'] = t['q2']; P['q2_wait'] = t['q2_wait']
    P['q2_census(bg)'] = g(t, 'q2_census'); P['q2_census_index(bg)'] = g(t, 'q2_census_index'); P['q2_census_wait'] = g(t, 'q2_census_wait')
    P['q34'] = t['q34']
    if b.get('used'):
        P['front'] = b['front_ms']
        P['filter'] = b['filter_ms']; P['filter_dev'] = b['device_ms']; P['filter_kern'] = b['filter_kernel_ms']; P['filter_xfer'] = b['filter_transfer_ms']
        P['filter_host'] = b['filter_ms'] - b['device_ms']
        P['cert'] = b['certificate_ms']; P['cert_dev'] = b['certificate_device_ms']; P['cert_kern'] = b['certificate_kernel_ms']; P['cert_xfer'] = b['certificate_transfer_ms']
        P['cert_host'] = b['certificate_ms'] - b['certificate_device_ms']
        P['lanes'] = b['lanes_ms']; P['lanes_dev'] = b['lanes_device_ms']; P['lanes_kern'] = b['lanes_kernel_ms']; P['lanes_xfer'] = b['lanes_transfer_ms']
        P['lanes_host'] = b['lanes_ms'] - b['lanes_device_ms']
        P['lanes_convert'] = b['lanes_convert_ms']
        P['lanes_plan'] = b['lanes_plan_ms']; P['lanes_task'] = b['lanes_task_ms']; P['lanes_compact'] = b['lanes_compact_ms']
        P['lanes_setup'] = b['lanes_setup_ms']; P['lanes_finish'] = b['lanes_finish_ms']
        P['lanes_upload'] = g(b, 'lanes_upload_ms'); P['lanes_download'] = g(b, 'lanes_download_ms'); P['lanes_download_copy'] = g(b, 'lanes_download_copy_ms'); P['lanes_host_alloc'] = g(b, 'lanes_host_alloc_ms')
        P['tail'] = b['tail_ms']
        P['glue_untimed'] = t['q34'] - (b['front_ms'] + b['filter_ms'] + b['certificate_ms'] + b['lanes_ms'] + b['tail_ms'])
        P['dev_total'] = b['device_ms'] + b['certificate_device_ms'] + b['lanes_device_ms']
        P['kern_total'] = b['filter_kernel_ms'] + b['certificate_kernel_ms'] + b['lanes_kernel_ms']
        P['xfer_total'] = b['filter_transfer_ms'] + b['certificate_transfer_ms'] + b['lanes_transfer_ms']
        P['host_in_calls'] = P['filter_host'] + P['cert_host'] + P['lanes_host']
        P['gpu_prepare(bg)'] = b['gpu_prepare_ms']
        P['front_max_job'] = d['q34_occupancy']['max_job_ms']
        P['front_job_sum/48'] = d['q34_occupancy']['job_sum_s'] * 1000 / 48
    P['merge'] = t['merge']; P['tower_index'] = t['tower_index']; P['census'] = t['census']
    P['tower'] = t['tower']
    for k in ['validate', 'static', 'lots', 'populations', 'images', 'bank', 'encode']:
        P['tower.' + k] = tp[k]
    P['tower.pool'] = td.get('pool_ms', 0)
    P['tower.residual'] = t['tower'] - sum(tp[k] for k in ['validate', 'static', 'lots', 'populations', 'images', 'bank', 'encode']) - td.get('pool_ms', 0)
    P['tower.tail'] = tp['populations'] + tp['images'] + tp['bank'] + tp['encode']
    P['tower.window'] = tp['static'] + tp['lots']
    # window model: phase 0 top-down; A(1) starts at 0
    sk = tp['static_by_k']; lk = tp['lots_by_k']; K = len(sk)
    ends = []
    for k in range(1, K + 1):
        ready = sum(sk[j - 1] for j in range(k, K + 1)) if k > 1 else 0.0
        ends.append(ready + lk[k - 1])
    P['_ends'] = ends
    P['_bind'] = 1 + max(range(K), key=lambda i: ends[i])
    P['window_model'] = max(ends)
    P['static(K)'] = sk[-1]; P['A(K)'] = lk[-1]
    P['static_resolve(K)'] = tp['static_resolve_by_k'][-1]
    crit = t['prepare'] + t['gen_index'] + t['q34'] + t['merge'] + t['tower_index'] + t['census'] + t['tower']
    crit += t['q2_wait'] if b.get('used') and lv.get('q2_during_device') else t['q2']
    P['chain_residual'] = t['chain_total'] - crit
    P['chain_total'] = t['chain_total']
    return P


def work(d):
    c = d['catalogue']; b = d.get('q34_batch', {})
    W = {}
    W['sites'] = d['input']['sites']
    W['balls'] = c['balls']; W['by_qmin'] = c['by_qmin']; W['by_shell'] = c['by_shell'][:7]
    W['census_nodes'] = c['census_nodes']; W['census_leaf_tests'] = c['census_leaf_tests']
    W['early_census_keys'] = c.get('early_census_keys', 0)
    W['max_interior'] = c['max_interior']; W['extra_shell'] = c['extra_shell_balls']
    W['cat_bytes'] = c['bytes']
    W['survivors'] = b.get('survivors', 0); W['lanes_records'] = b.get('lanes_records', 0)
    W['rss_GiB'] = d['peak_rss_kb'] / 1048576
    return W


if __name__ == '__main__':
    r, i = sys.argv[1], int(sys.argv[2])
    d = load(r, i)
    for k, v in phases(d).items():
        print(k, v)
