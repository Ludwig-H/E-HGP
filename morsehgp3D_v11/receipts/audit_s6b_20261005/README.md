# Assemblage S6b — contrelecture du 5 octobre 2026

`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.

Source : commit `9e7428995e3b359301d58d882610d9d4ee720fad`,
`build/v11-impl-l0`. Le produit S6b est identique à celui du commit local
`ce66efc4a` de `build/v11-impl-l2`. La ligne S6a u18 de `tests.cmake`
garde sa correction propre dans l0 ; aucune qualification n'est transférée
entre ces deux contextes. S3 est publiée sur `main` en `165def5ab` ;
`9e7428995` a également été publié pendant cette contrelecture.

**Verdict : aucun nouveau défaut important établi.** L'assemblage conserve
tous les supports, même sans coface. La pré-passe refuse une coquille de
plus de 24 sites avant allocation, sur l'appel entier. Les deux passes
gardent le même `BallIdx` à chaque position ; sorties disjointes, tampons
privés par worker, jointure avant publication ou libération. Le produit
possède ses tampons et emprunte explicitement l'OrderTree, qui doit
survivre aux lectures de ses identifiants.

## Preuves et portée

- [Lecture du produit et mémoire](native/README.md), avec sources figées,
  périmètre exact et limites dans `native/source_manifest.json` et
  `native/review.json`.
- [Calcul indépendant des bornes](native/bounds.py) : 3 896 formes du
  domaine préparé ; comptes par boule/support dans u32, décalages et
  admission dans u64. Les tailles ABI utilisées sont explicitement
  déclarées, sans prétendre exécuter `sizeof`. Rejeu normal et `-O`
  identique à l'octet, puis même résultat après copie dans ce reçu.
- [Contre-épreuve mathématique et du juge complet](math/REPORT.md) :
  postordre, seaux stables, instantanés datés, supports sans coface et
  comptes exacts sur huit ordres ; six altérations ciblées sont rejetées.
  Le modèle Python s'appuie sur S1 et ne lance pas le produit C++.
  Résultats normal et `-O` identiques, y compris après reconstruction
  des sources depuis le pin Git.

Depuis la racine du dépôt, rejeux Python bornés :

```sh
python3 -S -B morsehgp3D_v11/receipts/audit_s6b_20261005/native/bounds.py
python3 morsehgp3D_v11/receipts/audit_s6b_20261005/math/replay.py
python3 morsehgp3D_v11/receipts/audit_s6b_20261005/math/replay.py --optimized
```

Le refus global à 25 sites est câblé à K1/K2, sans Pool et à W4 : raison,
pic nul, budget rendu et diagnostics intacts. L'admission à 24 sites
vérifie les 828 supports, leurs arités et les comptes K1..3. Les six
mutations nouvelles visent le tri, trois omissions d'admission et le
retrait de la pré-passe. Ce sont des constatations de source : ces portes
natives n'ont pas été exécutées par cet audit.

Les rapports locaux `impl_s6b.md` et `verif_s6b.md` annoncent des résultats
u21. Ils ne sont pas des reçus G4. Restent à qualifier sur la source
assemblée : u18/u24 natifs, sanitizers, TSan, campagne complète de mutants
et K10. Aucun build, test natif, benchmark ou GCP lancé ici.

## Suite

S7 n'est pas encore livrée aux pins relus : écrivain/lecteur MHGP11SP,
raccord API/CLI, identité de signature avec FULL et mesure des étages
complets. Les remarques déjà connues, dont l'allocation de scratch pour
tout le Pool, ne sont pas relancées comme de nouvelles alertes.
