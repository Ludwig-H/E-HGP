from math import comb

def capacity():
 rows=[]
 for n in range(1,13):
  count=sum(comb(n,q) for q in range(1,min(n,4)+1))
  rows.append({'local_sites':n,'presentations':count,'point_tests_upper':n*count,'comparisons_upper':count-1})
 nmax=(1<<32)-2
 return {'scope':'Analytical combinatorial bounds, no product geometry or allocations', 'local':rows,'maximum_local':rows[-1], 'global_site_domain_max':nmax,'one_global_response_ids_bytes_upper':4*nmax,'global_outputs_formula':'4*sum(|I_j|+|U_j|); each <=4*n, all retained answers included', 'global_shell_not_bounded_by_local12':True,'layout_sizeof_not_measured':True}
if __name__=='__main__':
 import json
 print(json.dumps(capacity(),indent=2,sort_keys=True))
