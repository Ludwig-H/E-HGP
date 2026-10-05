# Contrelecture de la nouvelle porte S3 Fraction — 5 octobre 2026

Source figée : intégration S3 non commise de `build/v11-impl-l1b`, HEAD
`19b2fb218b86411a138e5ea351e1b42e265c1127`. Les empreintes des 30 fichiers lus sont
dans `source_manifest.json` ; copie stable pendant la capture et toujours identique
à la source développeur à la vérification après exécution. Le rejeu se reconstruit
depuis ce pin Git et `source_delta.patch`, sans le worktree du développeur :

```sh
python3 replay.py
python3 replay.py --optimized
```

La nouvelle porte `mhgp11_tower_attach_fraction` est effectivement câblée dans
`tests/tower/tests.cmake` : projection de l'arbre et des boules de la sonde vers
l'oracle S1 Fraction (étage A seul), `Cat_K` étroit, voies W1 et W3 avec permutation
effective. Elle compare le propriétaire fermé, les branches ouvertes, les rôles,
les traces strictes, les niveaux et la numérotation/postordre. Elle contrôle aussi
que S* est un support positif minimal d'arité qmin. C'est une porte S3 ; elle ne
compare pas toute Q_b de S6a ni les verticales FULL.

Aucun nouveau défaut mathématique important établi. Le cœur S3 garde ses SHA déjà
relus ; le delta ajoute ce juge et le témoin natif borné K9..K12. Ses attendus
centraux sont confirmés par l'oracle : K9 interne (4 traces, 1 branche), K10 fusion
(4 traces, 4 branches), K11 et K12 naissance. Les nombres de nœuds 7/5/1/1,
de naissances 6/4/1/1 et les enfants des deux fusions concordent.

## Nouvelle contre-épreuve portable bornée

`check_gate.py` calcule les vrais documents de l'oracle pour D2, E5, le triangle
faible, la cellule passagère et le témoin de 12 sites à K1..12 : cinq nuages,
16 ordres. Le nouveau témoin ajoute exactement 12 ordres, 208 boules, 135 nœuds,
93 boules de naissance, 90 de fusion, 25 internes, six passagères, 86 faibles,
25 fusions à au moins trois enfants et sept ordres K>=6. Ces ajouts donnent
les compteurs u21 annoncés en les additionnant à la suite antérieure déjà close
(951 ordres, 15062 boules, 12441 nœuds). La suite complète de 963 ordres n'est
pas relancée ici.

Le transport de la sonde est remplacé par des lignes JSON synthétiques, sans
lancer aucun exécutable. Le comparateur accepte les projections exactes en W1 et
W3 (304 contrôles par voie) et rejette neuf altérations par code 1 :
événement faible omis, propriétaire ouvert de D2, branche omise D2/E5,
passagère classée interne, K10 confondant parties comprimées et traces strictes,
K12 privé de sa naissance, postordre faux et S* invalide. Les premiers messages
exactement obtenus sont conservés dans les JSON.

Python normal et -O, puis les deux commandes de rejeu depuis le pin et le patch,
rendent code 0 et quatre JSON identiques octet pour octet :
SHA256 `e82f8eed892108a564e21913543c76db173d0f3a836d288395558880151c3d56`.
Les quatre commandes et leurs sorties/codes sont dans `executions.json`.
Aucune tentative en échec dans cette capture.

Phase exploration_v11_hors_registre, backend cpu_reference, profil
quantized_u21_input_only, public_status not_claimed. Aucun build, exécutable
natif, test natif ni GCP exécuté. Ce contrôle valide des attendus exacts et la
capacité du harnais à détecter ces fautes ; il ne qualifie pas le moteur natif.
