# Contre-lecture bornée du numérique local

Sources : `c3de9d73d8999f2f1e31a0f592b829efc6e7a4da` (socle intégré par `6a38f7e4b`).
`cpu_reference`, compilation Release au profil 32, `public_status=not_claimed`.
Une petite sonde appelle directement `Q3Candidate`, `Q4Candidate`, leur matérialisation
et les prédicats du produit. Aucun fichier produit n'est modifié.

L'oracle Python n'importe aucune formule de centre du produit : il résout les
équations d'équidistance par élimination de Gauss sur `Fraction`, puis calcule la
distance au centre, les coordonnées barycentriques et les ordres rationnels.
Les certificats sont comparés séparément à la recherche exhaustive du plus grand
exposant satisfaisant leurs inégalités publiées. Les hashes de toutes les sources
`num/` et `core/` lues sont identiques au pin avant et après ; le binaire est aussi
haché avant/après.

Résultat : **224 présentations, 220 sphères valides, 4 dégénérescences et 880
requêtes de puissance**, toutes conformes. Les mêmes entrées sont lues en mode
normal et `-O` ; leurs reçus sont identiques octet pour octet. Les cas couvrent
q2/q3/q4, les étendues 1, 7, 14, 16, 17, 19, 20, 21, 24, 25, 30, 31 et 32,
des réancrages, des translations près de `UINT32_MAX`, les milieux q3/q4,
48 petits supports pseudo-aléatoires et des candidates obtuses ou presque alignées.
Centres, niveaux, signes d'orientation, positivité stricte de la présentation q4,
milieux et comparaison au centre/niveau précédent sont confrontés à l'oracle.

Les voies exercées par les puissances sont : 408 natives, 180 certifiées,
22 contrôlées et 270 larges. Les compteurs enregistrent la voie aboutie, pas les
opérations internes ; les comparaisons de centres peuvent décider sur la partie
entière avant de compter une comparaison fractionnaire.

- `CST-0201` : le témoin q3 d'étendue 20 porte un certificat de puissance de
  domaine 20. Sa requête gardée dont le terme quadratique dépasse 127 bits passe
  effectivement en exact large, avec la bonne valeur. Les domaines de puissance
  et d'orientation des 220 sphères égalent l'oracle des inégalités. Correctif
  confirmé pour ces fabriques et appels CPU ; aucun mutant ni appel GPU n'est
  rejoué ici.
- `CST-0111` : les paliers de construction 24/25 et les voies des prédicats
  génériques sont confrontés en exact. La garde et son recensement ont leur reçu
  distinct. Les budgets mixtes d'orientation attendent encore les appelants du
  catalogue : garder le constat en cours à cette portée globale.
- `CST-0114` : le milieu local est juste, y compris pour des paires antipodales
  translatées en domaine u32 et des présentations q3/q4 aux grandes étendues.
  La canonicalisation du catalogue et son chemin appareil ne sont pas livrés au
  pin : ne pas transférer cette réussite du socle à leur qualification.

Précision documentaire : `CONTRAT_NUMERIQUE.md` § 4 et le commentaire public de
`compare_centers` annoncent des parties entières de centres sur 64 bits. Cela
demande l'hypothèse d'une boule certifiée, dont le centre reste dans l'enveloppe
des sites. Pour la candidate générique q3
`(0,0,0), (4294967295,4294967294,0), (4294967294,4294967293,0)`, le centre vaut
`(-36893488104469430283/2, 36893488121649299459/2, 0)` : ses planchers dépassent
le domaine signé 64 bits. **Pas de défaut de calcul reproduit** : `centers.cpp`
conserve effectivement la partie entière en i128 ou `Big`, et notre comparaison
exacte passe. Restreindre l'énoncé documentaire ou préciser ces types.

Reproduction depuis la racine du worktree, avec une sortie binaire temporaire :

```sh
sh morsehgp3D_v12/receipts/audit_u32_20261007/numerique/build.sh /tmp/mhgp12-audit-num
python3 -B morsehgp3D_v12/receipts/audit_u32_20261007/numerique/check.py --binary /tmp/mhgp12-audit-num --output /tmp/num-normal.json
python3 -B -O morsehgp3D_v12/receipts/audit_u32_20261007/numerique/check.py --binary /tmp/mhgp12-audit-num --output /tmp/num-optimized.json
cmp /tmp/num-normal.json /tmp/num-optimized.json
```

Limites : test borné de correction au profil de compilation 32, pas une preuve
exhaustive ni une campagne de performances. Aucun rejeu des suites lourdes,
sanitizers, mutants, profils 21/24, données réelles ou GCP. Aucun contrat D6,
catalogue GPU ou FULL/100 ms acquis par cette porte.
