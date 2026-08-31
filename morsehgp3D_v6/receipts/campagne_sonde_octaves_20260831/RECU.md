# Reçu — campagne sonde octaves, statut `exploratory_complete`

Statut : **exploratoire, jamais décisionnel** (requalification de l'auditeur,
`ALERTE_CAMPAGNE_CPU_MIXTE` mise à jour, acceptée) : `bench/agregateur.py` a
été committé APRÈS le début de cette capture (13:36:39Z vs 8157c65d à
13:42:23Z) — il n'est pas préenregistré POUR ELLE ; le lanceur réellement
actif était un script de scratch (sémantique identique à
`bench/campagne_locale.sh`, committé lui aussi après le début). AUCUN
`E6_active` n'est produit sur cette capture. La provenance d'exécutable est
en revanche PROPRE : copie privée immuable `bin/mhgp6` (chmod 555), sha256
`4bbb257cd31413f2c1058ee7b873f2ffe84158e3ce299a76d1230e6ab3053359` égal à la
source à la copie, vérifié avant ET après chacun des 36 tuples (36/36 lignes
`HASHES.txt` homogènes), 36 codes 0, `DONE` terminal, stderr vides, hashes
des 36 sorties au META.

Lanceur : script de scratch `camp_oct2.sh` (non versionné, faute gravée) —
sémantique reprise à l'identique par `bench/campagne_locale.sh` ; la
prochaine campagne (décisionnelle) part du lanceur COMMITTÉ, au commit qui
contient lanceur + validateur + agrégateur + profil, hashes gravés avant le
premier tuple (correction 3 de l'alerte).

`PENTES.txt` : sortie exacte de `bench/pentes.py` du pin (rc=0 — matrice,
bijections, identités fermantes des sept vecteurs d'octaves toutes
vérifiées).

## Lecture exploratoire (LECTURE_ISSUES_OCTAVES.txt — aucune décision)

La donnée NEUVE de cette capture (absente de la capture invalide 518e2706) :
les quatre issues des seeds q4 par octave. Sur les ancres lourdes (octave
≥ 10) à n=32000 :

- **42–67 % des seeds lourdes meurent par CELLULES** (grille de centres,
  coût quasi nul par seed après construction) ;
- **31–54 % meurent par CŒUR** (scan Jung, ~30–80 évaluations chacune) ;
- corde : 0,5–3,6 % ; **passe 2 : 0,1–0,3 %** (le sweep aval est déjà
  marginal sur ces ancres) ;
- `w1/seed_lourde` = 22–77 : le scan PAR seed est court (les arrêts précoces
  fonctionnent) — la masse `W_sweep1` des octaves ≥ 10 (19–67 % du total)
  est le PRODUIT du nombre de seeds lourdes (4–20 M) par ce petit facteur.

Orientation E6 qui en découle (hypothèse à éprouver, pas un verdict) : la
grille tue déjà la moitié des seeds lourdes pour rien ; le mur restant est
la population « cœur » — transformer ces kills de cœur en décisions de
grille (comptes de témoins de cœur par cellule, grille hiérarchique ou
Tier R sur les ancres à cover ≥ 2^10) attaquerait directement 30–60 % de
W_sweep1 sur les surfaces stationnaires. La sonde contrefactuelle appariée
du plan E6 doit viser cette population.

## Provenance

- pin `cfaf6b41` (arbre v6 identique à b3e64205, l'état du binaire) ;
- binaire privé + source : sha256 identiques au META ; toolchain gravée ;
- machine : codespace 8 vCPU au repos (charge gravée au META) — les temps ne
  sont pas des mesures ; compteurs déterministes seulement ;
- matrice : 4 familles × {8000, 16000, 32000} × graines {3, 4, 5}.
