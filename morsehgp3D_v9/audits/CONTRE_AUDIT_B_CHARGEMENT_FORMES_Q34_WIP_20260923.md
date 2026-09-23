# Contre-audit B — chargement des formes q3/q4 en WIP

23 septembre 2026, lecture seule du diff de
`q34_dead_lanes.{cpp,hpp}` après `684d8fc7`, publié ensuite sous
**`47f8a5da`** avec les mêmes octets (`cpp` SHA-256 `f1b23ac4…`,
`hpp` `474e9b11…`). Il préordonne les points
selon le rang spatial une fois par `Q34DeadLaneProver`, réutilise le
préfixe d'IDs de frontière et écrit les formes sans lookup d'ID dans
chaque cover. Le produit scalaire spécialisé aux deux coordonnées non
nulles de chaque vecteur de base est algébriquement identique à l'ancien
`dot`. Les formes des deux extrémités, désormais incluses, valent
identiquement zéro : elles ne fournissent aucun témoin strict. Les
comptes `form_sites` restent exprimés hors extrémités, tandis que
`site_count` formes sont physiquement **calculées** ; les tests réels
sur ces deux formes supplémentaires doivent, eux aussi, rester payés.
Ajouter `forms_computed` ou distinguer clairement ce compteur logique
dans toute ablation du chargement. Ces
lectures ne prouvent pas de gain de temps.

Deux défauts d'API/exactitude sur ce WIP :

1. `load()` peut allouer/remplir `ordered_` **avant** `loaded_=false`.
   Après un `load` réussi, un échec d'allocation de la préparation d'un
   autre index laisse donc `loaded_==true` ; `prove()` peut encore utiliser
   l'ancienne preuve. Cela contredit le contrat commenté « failed load
   leaves no usable state ». Invalider dès l'entrée, avant toute opération
   qui puisse échouer, puis ne réarmer qu'à la fin.
2. `ordered_owner_` est l'adresse nue de l'index, sans possession.
   Un nouvel index alloué à la même adresse après destruction du précédent
   ne déclenche pas la reconstruction de `ordered_`. À taille différente,
   l'accès par rang peut sortir du buffer ; à taille égale, les formes
   peuvent porter les **mauvais points** et certifier à tort une voie
   morte. Le moteur `Engine` garde son index stable pendant un appel, mais
   l'API publique du prover accepte plusieurs covers successifs sans
   ce contrat. Garder un `Q2CensusIndexPtr` propriétaire du cache, ou
   réinitialiser par une identité de génération non réutilisable.

Le coût du nouvel `ordered_` est **O(n) par worker**, pas O(n) partagé.
`sizeof(Point3)=12` octets a été confirmé sur l'ABI de build ; il ajoute
au minimum `12·n·W` octets : 28,8 Mo à 50k/W48, 576 Mo à 1 M/W48 et
17,28 Go (16,1 Gio) à 30 M/W48, avant surcapacité, formes, index,
catalogue et forêts. Ce poste
peut être raisonnable à 50k mais rédhibitoire dans le contrat des
plusieurs dizaines de millions de points ; mesurer RSS couplé et coût de
préparation, comparer à un tampon spatial immuable partagé entre workers.
`peak_edge_buffer_bytes` ne donne que le maximum d'un worker ; la somme
des maxima privés est publiée séparément mais n'est pas un pic
simultané. Après le chargement, `observe` ne recombine pas toujours
`ordered_` avec les buffers ultérieurs d'atlas/q3 ; le RSS réel est
indispensable.
`forms_.resize(site_count)` peut aussi initialiser une croissance du
vecteur avant de réécrire ses formes : qualifier le gain par ablation
réelle, sans supposer qu'une boucle sans branche est plus rapide. Le
message du commit mentionne 74→46 Gcycles de chargement sur une trame
K5/W8 avec instrumentation de brouillon et même digest : indication
locale, **pas un reçu apparié versionné** ni une mesure de toute la
tour/G4. Aucun test nouveau n'est ajouté par ce commit.

Porte minimale : deux index construits successivement, échec injecté au
début du second `load`, et réutilisation d'adresse contrôlée ; oracle de
formes/résultats, ASan/UBSan/TSan si pertinent, puis W1/W8/W48 sur les
trois trames 1 mm avec même sortie complète, temps de chargement,
`dead_form_sites`, tests, RSS et pics de workers. Ne pas relancer G4
uniquement pour ce WIP avant correction des deux points de sûreté.
