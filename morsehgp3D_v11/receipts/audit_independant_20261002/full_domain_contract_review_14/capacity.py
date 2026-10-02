def capacity():
 maximum=(1<<32)-2
 cases=set(range(2049))|{10_000_000,30_000_000,50_000_000,maximum,maximum+1}
 for power in range(33):
  for offset in (-1,0,1):
   n=(1<<power)+offset
   if 0<=n<=maximum+1:cases.add(n)
 checked=0
 for n in sorted(cases):
  c=0 if n==0 else 1<<(2*n-1).bit_length()
  if n==0:
   if c!=0:raise ValueError('empty capacity')
  else:
   if not (c>=2*n and c<4*n and c&(c-1)==0 and 4*c<16*n and c<=1<<33):raise ValueError('pow2 capacity bound')
   if not (4*c<=1<<35 and 16*n>4*c):raise ValueError('bytes and upstream guard')
  checked+=1
 samples=[]
 for n in (0,1,2,3,127,128,129,10_000_000,30_000_000,50_000_000,maximum):
  c=0 if n==0 else 1<<(2*n-1).bit_length()
  samples.append({'catalogue_balls':n,'lookup_slots':c,'lookup_bytes':4*c,'temporary_emissions_proven_lower_bytes':16*n,'lookup_fits_below_released_emission_bytes':n==0 or 4*c<16*n})
 return {'status':'PASS','scalar_cases':checked,'max_real_catalogue_balls':maximum,'max_lookup_slots':1<<33,'max_lookup_bytes':1<<35,'scope':'Independent scalar powers/cardinalities, no arrays indexed by giant sizes and no product execution','samples':samples,'success_peak_proof':'For B>0: C=pow2ceil(2B)<4B; 4C<16B<=sizeof(Emission)*B. Successful catalogue assembly coexists with final Rcat and temporary Emission[B]+SiteIdx[P], so Fcat>=Rcat+16B+4P>Rcat+4C. For B=0 lookup has zero bytes. Under stable U/single-pilot budget, FullDomain success peak equals catalogue success peak; table system allocation can still fail.'}
if __name__=='__main__':
 import json
 print(json.dumps(capacity(),indent=2,sort_keys=True))
