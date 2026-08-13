# Audit de `BallFormToBallEvent-v0` au pin `2b89ea1`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

GCP non utilisé. Aucun fichier logiciel n'a été modifié par l'auditeur.

## Verdict

Le pin construit enfin un objet après les formes : clé primitive, supports,
census `I_B/U_B`, owner et disposition. L'ordre architectural est bon et les
huit CTests passent en `0,08 s` sur le domaine du probe `coord<=64`.

L'étape `0A` n'est cependant **pas reçue pour le profil u16**. Deux overflows
certains corrompent les clés sur des coordonnées valides, et plusieurs verts
ne jugent pas la propriété qu'ils annoncent. Le bon statut est :

```text
VerticalBallEventSlice-v0 = implemented_bounded_coord64_candidate
quantized_u16_input_only = blocked_by_exact_width_and_abi
BenchmarkOutputContract-v1 = absent
SLO_50k_G4 = not_measured
```

## 1. Deux erreurs arithmétiques P0

### 1.1 Réduction de numérateurs larges vers `long long`

`prototype/ball_event.hpp:125` et `:156` convertissent en `long long` des
numérateurs calculés en `i128`. Cette conversion est hors plage sur des entrées
u16 ordinaires.

Poser `M=65535`. Pour le triangle
`(M,0,0),(0,M,0),(0,0,M)`, la construction q3 produit
`N_i=2M^5=2417667177417934889418750`, soit 81 bits. La clé primitive attendue
est :

```text
(1,-43690,-43690,-43690,-1431612075)
```

Pour le tétraèdre
`(0,0,0),(M,M,0),(M,0,M),(0,M,M)`, chaque numérateur du centre vaut
`8M^4=147564945596578005000`, soit 67 bits. La clé attendue est :

```text
(1,-65535,-65535,-65535,0)
```

Le commentaire « borne vérifiée par l'appelant » n'est appuyé par aucun
preflight ; le probe limite seulement ses générateurs à 64.

### 1.2 Overflow `i128` avant le pgcd

`ball_event.hpp:75-83` construit `den^2`, `N^2` et
`||den*p-N||^2`, puis réduit le résultat. La réduction arrive trop tard. Dans la
fixture q3 ci-dessus, `den=6M^4` a 67 bits, `den^2` 134 bits et `N^2` environ
162 bits. La fixture q4 atteint déjà environ 134 bits.

La clé doit être formée avant cette explosion : pour un centre `N/den` et un
point `p` de la sphère, employer directement

```text
A=den
B=-2*N
C=2*N dot p-den*||p||^2
```

puis normaliser le signe et le pgcd. Pour q3, la forme encore plus courte déjà
reçue est :

```text
A=G
B=-(2G*a+W)
C=G*||a||^2+a dot W
```

### 1.3 La positivité q3 déborde aussi

Élargir seulement `N` ne suffit pas. `ball_event.hpp:191-195` remultiplie les
numérateurs barycentriques par le dénominateur du centre ; la même fixture q3
atteint un terme `6M^8`, au-delà de 128 bits.

Pour q3, tester directement les trois numérateurs :

```text
E*(D-F) > 0
D*(E-F) > 0
2G-E*(D-F)-D*(E-F) > 0
```

Pour q4, conserver de même les numérateurs Gram--Cramer des quatre poids, sans
passer par un centre surdimensionné. Chaque branche reçoit une preuve de
largeur sur tout `[0,65535]^3` avant d'entrer dans l'ABI.

## 2. Les huit verts ne ferment pas encore l'indépendance

- Le juge rationnel classe dépendance affine et côté de sphère, mais ne juge
  pas indépendamment `be_positive`. Un mutant de positivité peut donc changer
  silencieusement la liste des supports émis.
- Il ne reconstruit pas une clé primitive canonique indépendante ; il compare
  seulement les signes sur les points présents. Deux polynômes distincts qui
  ont les mêmes signes sur ce petit nuage peuvent passer.
- Le mutant `cle-non-reduite` ajoute artificiellement `runs.size()` à son
  compteur de fautes. Il se déclare donc tué sans comparer la clé ni la
  partition de runs à une autorité.
- Le test de refus `--coord=128` est rejeté par le parseur dont la borne vaut
  64. Il n'atteint jamais un preflight arithmétique de `BallEvent`.
- Le probe emploie l'indice du vecteur comme `PointId` partout. Son mutant
  owner ne teste pas une permutation réelle stockage/identité, car le sujet et
  la référence rappellent `be_owner` sur les mêmes indices.

Le minimum recevable est un juge multiprécision ou rationnel indépendant qui
reconstruit : dépendance, positivité, clé primitive, niveau exact, `I_B/U_B`,
`SupportKey` et owner à partir de vrais `PointId` séparés des indices.

## 3. ABI à compléter avant le fold

La `PrimitiveSphereKey` est une bonne clé géométrique, mais la `BallKey`
contractuelle ajoute au moins `CloudEpoch` ou `CloudDigest`,
`GeometryProfileId` et `ExactKeySchemaId`. Deux nuages ne partagent jamais un
run par accident.

Le fold exige aussi :

- un `ExactLevelToken` et son comparateur exact ; reconstruire naïvement le
  rayon depuis `(A,B,C)` crée des produits plus larges que `i128` ;
- un vrai `PointId` distinct de l'index dense, avec codec versionné ;
- un `SupportRecord` atomique qui réunit `SupportKey`, owner, lane, positivité
  et provenance, au lieu de deux vecteurs parallèles ;
- `cloud_size`, compte extérieur et `census_complete`, avec partition exacte
  du nuage ; `rank` redondant ne doit pas pouvoir diverger ;
- `supports_complete` ou `SupportStreamRef` reçu ; une liste explicite sans
  marqueur peut être un préfixe silencieux ;
- un statut initial `unclassified/pending`, jamais `regular` par défaut ;
- des statuts distincts `unsupported_degeneracy`, `resource_exhausted`,
  `numeric_failure`, support dépendant et support non positif ;
- `count -> preflight -> fill -> validate -> publish`, avec zéro événement
  publié après toute erreur ou cap moins un.

`__int128` et `std::vector` peuvent rester des détails de l'oracle hôte. Ils ne
sont ni la sérialisation persistante ni l'ABI device.

## 4. Portée exacte de `RelevantGP`

Le code assimile actuellement tout `U_B!=S` à une violation. La règle reçue
porte sur un support propre **pertinent**, donc sur la lane et la condition
`p+q<=smax`. Un support hors fenêtre ne doit pas faire refuser un run entier.
Inversement, `rank=|I_B|+|U_B|` ne remplace pas `p+q` : une paire antipodale sur
un shell de douze points garde `p+q=2` et peut être précisément la violation
pertinente.

La politique candidate reste fail-closed : un extra-shell utile donne
`unsupported_degeneracy` atomique tant que le quotient n'est pas reçu. Cette
politique exige une fermeture explicite du domaine `RelevantGP`; elle ne découle
pas du seul encodage u16.

## 5. Fixtures bloquantes

1. triangle u16 maximal ci-dessus, clé attendue et trois poids `1/3` ;
2. tétraèdre u16 maximal ci-dessus, clé attendue et quatre poids `1/4` ;
3. stockage permuté d'un équilatéral portant les IDs non denses `{40,7,99}` :
   owner attendu `(7,40)` ;
4. paire `(0,0,0),(2,0,0)` avec trois témoins qui donnent exactement les signes
   `-1/0/+1` ;
5. même sphère produite par deux supports : une clé, un census, deux records de
   support sans écrasement ;
6. triangle rectangle : q3 non positif, jugé par une route indépendante ;
7. shell de douze points avec paire antipodale : rang fermé douze mais
   `p+q=2` ;
8. cosphère 384 : refus atomique après clé, census et témoin de violation, sans
   exiger l'expansion des `2 322 560` supports ;
9. même équation primitive sous deux epochs : deux `BallKey` ;
10. caps exacts puis moins un sur runs, supports, `I_B`, `U_B` et octets : zéro
    payload au moins un.

## 6. Ordre remis à Claude

1. réparer les formes q3/q4 sans centre réduit en `int64` et recevoir leurs
   largeurs sur u16 ;
2. rendre indépendants positivité, clé et owner avec les fixtures ci-dessus ;
3. versionner `BallKey`, `SupportRecord`, census et statuts transactionnels ;
4. seulement alors déclarer `0A` reçu et commencer `0B`, le fold borné jusqu'au
   `BenchmarkOutputContract-v1` ;
5. raccorder ensuite `SOC64`, le LP projectif ou les cages à ce même sink.

Le contrat `50k/1s` reste entièrement ouvert. Aucun de ces CTests CPU n'est une
mesure G4 ni une réception du payload complet.
