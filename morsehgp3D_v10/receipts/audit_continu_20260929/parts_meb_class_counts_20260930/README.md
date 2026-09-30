# K-parties : comptage par classe MEB et limite du catalogue FULL

Petit oracle autonome Fraction, sans moteur natif, source partagée, GCP ou performance.

Pour une boule B(c,r), avec I strict/p=|I| et U coquille, définir h_B(j) comme le nombre de S⊂U de cardinal j tels que c∈conv(S). h_B,x(j) ajoute x∈S. Toute K-partie T de MEB exactement B se décompose UNIQUEMENT en T∩I et T∩U. Son intersection de coquille doit contenir c dans son enveloppe convexe. Ainsi le nombre contenant x vaut Σ_j h_B(j) C(p−1,K−j−1) si x∈I, et Σ_j h_B,x(j) C(p,K−j) si x∈U. Un binôme hors domaine vaut zéro.

Le critère sur S n’exige pas que S soit minimal ni que tous ses poids soient strictement positifs. Sur une coquille régulière U de q points affinement indépendants, avec c strictement intérieur au simplexe, seule S=U convient, donc seul j=q contribue. Sur un carré cosphérique, h(2)=2,h(3)=4,h(4)=1 : compter uniquement les supports minimaux omet les parties avec trois/quatre points de coquille.

Qualification petite :113 comparaisons par point et K sur quatre classes : segment, triangle aigu, tétraèdre réguliers, carré non régulier ; deux sites strictement intérieurs par classe. Les MEB sont calculés par supports critiques strictement positifs ; h utilise une reconstruction affine indépendante avec poids non négatifs. Aucun échantillon de LiDAR ni garantie de coût industriel.

Contre-exemple K=Kmax=2, x=0, sites colinéaires 0,19/10,39/20,2. α_x=19/20, η=1/8, donc borne de bande au carré=29241/25600>1. Les trois paires contenant x sont dans la bande. La boule de {0,2} a c=1,r=1,p=2,qmin=2 : p+qmin=4>Kmax+1=3 et p≥K. Elle est exclue du catalogue FULL borné décrit par cette condition, mais représente un vote de la référence K-parties. La masse totale de référence de x vaut3, le filtrage en garde2. Ceci démontre une perte de vote/masse, PAS une hauteur majoritaire différente ni un défaut FULL de connexité.

Conséquence : le comptage combinatoire local est correct, mais ne prouve pas que les classes nécessaires soient dans le catalogue actuel. Ni énumérer C(n,K), ni élargir aveuglément le catalogue en production n’est proposé.

Acquisition : un préflight réussi (sortie outil tronquée), deux contrôles directs normal/−O réussis, puis deux captures finales à stderr séparé dans receipt.json (17:28:13–17:28:14 UTC). Les deux captures finales sont seules l’autorité. Les horaires de préflight ne sont pas inventés. Les lectures ultérieures ne sont pas des nouvelles mesures.

Lecteur : python3 -B verify.py puis python3 -B -O verify.py. Inventaire/hash vérifiés avant le reçu et le rejeu. Archives relocalisables, aucune dépendance LIVE.
