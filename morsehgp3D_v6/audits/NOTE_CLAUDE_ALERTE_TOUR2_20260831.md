# Note Claude — suivi « campagnes CPU » deuxième tour : les cinq corrections exécutées, campagne décisionnelle lancée depuis le lanceur committé

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Cette note remplace la précédente
(`NOTE_CLAUDE_ALERTE_CAMPAGNE_MIXTE_20260831.md`) comme état de réponse.

## 1. Capture en cours → `exploratory_complete` : FAIT

`campagne_sonde_octaves_20260831` est reçue **exploratoire** (RECU.md) :
36/36 hashes homogènes, `PENTES.txt` du pin (rc=0), AUCUN `E6_active`
produit. Votre réfutation du « préenregistré » est acceptée telle quelle et
gravée au reçu (l'agrégateur committé six minutes après le début, le
lanceur réel = script de scratch). Vos réductions de mes formulations sont
également actées : je retiens « amplification superquadratique du bin o12
scanline observée dans les trois graines sur le seul pas 16000→32000 », la
stabilité d'une queue AGRÉGÉE restant à tester — c'est exactement ce que
`T_lourde` (somme o≥10) mesure désormais, comme hypothèse dérivée déclarée.
La lecture neuve des issues (`LECTURE_ISSUES_OCTAVES.txt`) reste
exploratoire : 42–67 % des seeds lourdes meurent par cellules, 31–54 % par
cœur, passe 2 < 0,3 % — orientation de la sonde contrefactuelle E6.

## 2. `pentes.py` : autorité externe et provenance : FAIT

Profil d'autorité EXTERNE obligatoire (2ᵉ argument) : familles/n/graines
non vides du profil, `PROFIL.txt` de la campagne identique OCTET POUR OCTET,
ligne de matrice du META UNIQUE et égale au profil ; `pin`/`commande`/
`sha256_binaire_prive` uniques ; `bin/mhgp6` présent, non symbolique,
recoupé au hash ; `HASHES.txt` bijectif avec la matrice et HOMOGÈNE au hash
du binaire ; sha256 de CHAQUE sortie recoupé contre le META ; liens
symboliques et sous-répertoires refusés dans `out/` ;
`STATUT_TERMINAL.txt` présent ⟹ refus (l'ancienne campagne mixte est morte
pour tout consommateur, DONE ou pas). `tests/pentes_gate.py` : nominal + 31
falsifications (avec REHASH CAUSAL : chaque falsification échoue sur SA
cause déclarée, pas sur le recoupement de hash) + zéro légitime avec `-`.

## 3. Campagne décisionnelle depuis le lanceur committé : LANCÉE

`campagne_decisionnelle_20260831` court au pin `a30c3a98`, qui contient à la
fois `bench/campagne_locale.sh` (le lanceur EXÉCUTÉ), `bench/pentes.py`,
`bench/agregateur.py` et le profil canonique
`bench/profils/locale_decision_v1.txt`. Gravés AVANT le premier tuple :
sha256 du lanceur, du validateur, de l'agrégateur, de l'autorité de profil
(copiée), du binaire privé (= celui des reçus précédents,
`4bbb257c…3359`), `worktree_sources=0`.

## 4. Égalité copie/source : FAIT

`campagne_locale.sh` refuse AVANT le premier run si le sha256 de la copie
privée diffère de la source relue après la copie ; le worktree des sources
doit être propre (opt-out `CAMPAGNE_ALLOW_DIRTY=1` réservé à l'exploratoire
et gravé). `tests/campagne_gate.py` ajoute votre scénario réellement
annoncé : la SOURCE est REMPLACÉE pendant le premier tuple — les deux
tuples exécutent la MÊME copie et finissent `DONE` homogène (isolation
causale), plus le refus de copie discordante (faux `sha256sum`) et le
profil obligatoire.

## 5. Agrégateur → PORTE E6 BORNÉE : FAIT

Renommée « porte E6 bornée — trois termes, pas 16000→32000 seulement » ;
`garde_fou_borne_viole` remplace l'ancien nom (jamais assimilé au § 3
entier) ; `T_lourde` documenté hypothèse dérivée de la première capture, à
éprouver sur campagne indépendante ; transition 0→positif = **EMERGENCE**
(indéterminée, listée `emergences=`, ni déclencheur ni preuve négative) ;
un refus SUPPRIME l'`AGREGAT.txt` préexistant. Fixtures gravées
(`tests/agregateur_gate.py`) : majorité 1/3 → non, 2/3 → oui, seuil exact
(médiane == 2,0 déclenche), émergence indéterminée, violation du seul
pas 1 → non (périmètre documenté), refus + suppression.

L'alerte peut être absorbée selon vos termes : la campagne postérieure au
profil committé court, et le validateur tue vos contre-fixtures. Son
`AGREGAT.txt` sera produit par la porte E6 bornée préenregistrée — cette
fois réellement antérieure à la capture.
