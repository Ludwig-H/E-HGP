# Reçu — campagne de CONFIRMATION hors échantillon : `confirmation_complete`, verdict préenregistré **E6_active=non**

Statut : `confirmation_complete`. Première campagne du dépôt où TOUT le
protocole précède les données : profil d'autorité
(`locale_confirmation_v1`, graines 6/7/8 DISJOINTES de {3,4,5}, tailles
10000/20000/40000 DÉCALÉES de {8000,16000,32000}), lanceur, validateur et
PORTE E6 BORNÉE committés au pin `99bf6723` (15:56Z) — capture démarrée à
17:04Z au pin `320299df` ; binaire CONSTRUIT depuis `git archive` du pin
(mode auto : liaison binaire←commit PROUVÉE, `build_pin.log` conservé) ;
copies exactes du protocole archivées dans `protocole/` et LIÉES par le
validateur ; publication atomique (`.partial` → dossier).

Jugement par le protocole ARCHIVÉ, sorties non touchées (ordre de
l'auditeur) : `protocole/pentes.py` rc=0 (36/36, provenance, identités
fermantes) → `PENTES.txt` ; `protocole/agregateur.py` rc=0 → `AGREGAT.txt`.

## Le verdict (préenregistré, appliqué tel quel)

```text
E6_active=non famille=terrain_stationnaire
E6_active=non famille=scanline_stationnaire
garde_fou_borne_viole=non famille=uniform
garde_fou_borne_viole=non famille=eight_clusters
```

Médianes inter-graines des pentes au pas 20000→40000 : W_sweep1 1,22
(terrain) / 1,16 (scanline) ; m_anchor_q4 1,33 / 1,06 ; T_lourde 1,31 /
1,77 — toutes sous le seuil 2,0.

## Ce que la donnée hors échantillon établit

1. **L'hypothèse « queue superquadratique STABLE » n'est PAS confirmée** :
   la porte préenregistrée a protégé contre la confirmation d'une hypothèse
   ajustée dans l'échantillon — c'est exactement son rôle.
2. **La queue est INTERMITTENTE, pas absente** : excursions mono-graines
   T_lourde jusqu'à 6,41 (pas 1, scanline g7) et 4,19 (pas 2, scanline
   g6) ; et la queue peut DISPARAÎTRE (terrain g8 : T_lourde −0,68 au
   pas 2, W_sweep1 −0,01). C'est une migration/émergence de masse entre
   octaves selon la fenêtre et la graine — cohérent avec la réduction de
   l'auditeur (« pas encore une loi d'échelle stable »).
3. La dispersion inter-graines de W_sweep1 reste ÉNORME sur les surfaces
   stationnaires (étendues pas2 : 1,95 terrain, 0,93 scanline) contre un
   uniform parfaitement régulier (1,06/1,06/1,06 — trois graines
   indistinguables).

## Conséquence pour l'expérimentation E3/G16

Le mécanisme (`--e3-g16`, reçu `e6_grille_appariee_20260831`) reste ce que
sa mesure appariée montre : objet bit-identique et W_sweep1 −33/−40 % là où
la queue est présente — un RÉDUCTEUR DE VARIANCE des excursions, pas la
réponse à un mur systématique (le mur systématique n'est pas confirmé).
Son activation par défaut n'est PAS justifiée par cette porte : il demeure
opt-in, à trancher par l'audit (bras séparés et oracle G8/G16 livrés).

## Provenance

- pin de capture `320299df` ; binaire privé sha256
  `f74a8759c3e5add3cc5865d35b0903e763d2a1f869ddbe18dd7c3814f67ae07e`
  construit depuis l'archive du pin ; `HASHES.txt` homogène 36/36 ;
- hashes du lanceur/validateur/agrégateur/profil gravés AVANT le premier
  tuple et recoupés par `pentes.py` ; `worktree_sources=0` ;
- machine : codespace 8 vCPU (charges gravées) — compteurs déterministes
  seulement, les `secs=` ne sont pas des mesures.
