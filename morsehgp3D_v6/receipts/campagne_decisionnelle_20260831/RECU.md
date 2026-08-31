# Reçu — statut `replication_complete` (jamais une confirmation)

Statut : **replication_complete** — la requalification de l'auditeur
(« réplication de reproductibilité, pas un échantillon confirmatoire
indépendant ») est acceptée et fait foi : cette campagne rejoue le MÊME
binaire privé, les MÊMES familles, tailles et graines que la capture
exploratoire dont `T_lourde` et l'octave 10 ont été dérivés. Son nom de
dossier (« decisionnelle ») est CADUC — le présent reçu prime. AUCUN
`E6_active` n'est produit sur ces 36 tuples ; la PORTE E6 BORNÉE ne sera
appliquée qu'à la campagne de CONFIRMATION hors échantillon
(`bench/profils/locale_confirmation_v1.txt`, graines 6/7/8 disjointes,
tailles 10000/20000/40000 décalées, préenregistrée au pin `99bf6723` AVANT
toute lecture).

## Ce que cette réplication établit

- **Reproductibilité prouvée** : 36/36 `digest_all` IDENTIQUES à la capture
  exploratoire (`campagne_sonde_octaves_20260831`) — l'isolation par copie
  privée rend les compteurs et l'objet bit-reproductibles sur cette
  machine ;
- 36/36 codes 0, `DONE` terminal, `HASHES.txt` homogène au binaire privé
  (`4bbb257c…3359`), stderr vides, hashes des 36 sorties au META ;
- protocole ANTÉRIEUR aux données : commit `a30c3a98` (lanceur committé,
  hashes du lanceur/validateur/agrégateur et autorité de profil gravés
  avant le premier tuple — constat d'antériorité fait par l'auditeur) ;
- `PENTES.txt` : sortie exacte du validateur ÉPINGLÉ du pin (`git show
  a30c3a98:bench/pentes.py` + profil du même pin), rc=0.

## Limites (gravées par l'audit, acceptées)

Prospective vis-à-vis des scripts, PAS des données ; le lien binaire↔commit
est corrélé (même sha256 que le reçu de portes), pas prouvé par
construction — la campagne de confirmation utilisera le mode
`BIN_SOURCE=auto` du lanceur durci (build depuis `git archive <pin>`,
liaison prouvée). Les défauts résiduels du validateur relevés au troisième
tour (liaison des hashes de protocole) sont fermés au pin `99bf6723`,
postérieur à cette capture : ils ne s'appliquent pas rétroactivement ici.

## Provenance

- pin `a30c3a98` ; binaire privé `bin/mhgp6` sha256
  `4bbb257cd31413f2c1058ee7b873f2ffe84158e3ce299a76d1230e6ab3053359` ;
- matrice : 4 familles × {8000, 16000, 32000} × graines {3, 4, 5} ;
- machine : codespace 8 vCPU (charges gravées au META) — les `secs=` ne
  sont pas des mesures ; compteurs déterministes seulement.
