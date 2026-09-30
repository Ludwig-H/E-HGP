from fractions import Fraction
from decimal import Decimal
import json
rows=[]
for text in ('10','1','0.1','0.001'):
 h=Fraction(text)/1000
 for b in (18,21,24):
  span=((1<<b)-1)*h
  rows.append(dict(precision_mm_text=text,bits=b,maximum_grid_index=(1<<b)-1,step_m=dict(num=h.numerator,den=h.denominator),maximum_span_m_exact=str(span),maximum_span_m_decimal=str(Decimal(span.numerator)/Decimal(span.denominator)),current_product_profile=('u18' if b==18 else 'non_qualifie'),input_preparation=('Cloud Morton accepte le domaine de compte jusqu’à 21 bits' if b<=21 else 'Morton actuel 63 bits insuffisant pour 24 bits par axe')))
m=(1<<21)-1
D=4*m**3
tetra=dict(points=[[0,0,0],[m,m,0],[m,0,m],[0,m,m]],center_num=2*m**4,center_den=D,level_num=12*m**8,level_den=D*D,level_num_bits=(12*m**8).bit_length(),level_den_bits=(D*D).bit_length(),level_den_modulo_u128=(D*D)%(1<<128),true_beta=str(Fraction(3*m*m,4)),scope='Dimensionnement symbolique seulement : centre strictement intérieur, quatre poids 1/4 ; produit courant refuse B21. Ce n’est ni un appel natif B21 ni une qualification.')
print(json.dumps(dict(rows=rows,q4_b21_width_example=tetra),indent=2,ensure_ascii=False))
