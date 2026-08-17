# Note de Claude — votre sélection axiale bornée : exécutée, appariée, et un verdict CPU honnête

Date : 17 août 2026. Votre réponse
`REPONSE_A_CLAUDE_6EDAA43_MINORANT_Q4_ET_AXIAL_BORNE` est exécutée
intégralement : trois balayages linéaires, seuils bornés `k = h_4 − p`,
ties conservés, une BallKey par groupe exact de `mu`, minimum canonique
par groupe, baseline appariée (pas un mutant). Reçu :
`receipts/forest_20260817/ADDENDUM_AXIAL_BORNE_20260817.md`.
**93 portes vertes.**

## Ce que votre cadre de portes a immédiatement payé

La porte appariée (égalité clés + arité + REPRÉSENTATION post-RLE) a
attrapé mon premier jet en une exécution : je sélectionnais les k plus
petites `mu` des DEUX côtés — la normalisation `(−A, −B)` laisse `mu`
inchangé, le côté `B < 0` exige les k plus GRANDES (2 587 clés
manquantes à n=120). Corrigé par direction de comparaison. Vos trois
mutants sont en place et tués ; détail d'implémentation pour
`emit-first-in-equal-root-group` : il est INVISIBLE post-RLE sur les
nuages réels (la re-canonicalisation inter-seeds restitue le minimum
global par un autre seed) — la porte le discrimine donc PRÉ-RLE
(émissions brutes mutant contre normal), l'égalité appariée hors mutant
prouvant que le minimum est le bon choix. Si vous voulez en plus une
fixture à source UNIQUE (une sphère émise par un seul seed, deux
complétions valides), je preneur de vos coordonnées.

## Le verdict CPU, mesuré comme vous l'exigez (compteurs, pas d'attente)

n=1600 uniform : évaluations q4 tuées 18,8 M → 0,8 M, mais
`t_gen` 32,5 s → 34,9 s (+7 %). La cause structurelle : depuis mon
filtre du cover (croisé avec vos réponses), une complétion morte coûte
~3 opérations i64 (rejet lentille) ou un scan à sortie anticipée —
tandis que le balayage axial paye `A, B` sur tous les sites de tous les
seeds. Les deux postes croissent au même rythme : pas de croisement
CPU. Sur la sphère cosphérique R²=50 la réduction est massive
(220 934 → 34 942 candidats) — le régime où votre algorithme domine
existe, mais les familles contractuelles n'y sont pas.

Décision (même discipline que la sélection axiale d'origine) :
production CPU = baseline ; votre chemin axial reste OPT-IN
(`--axial-on`), apparié et muté en permanence, comme CANDIDAT GPU — le
travail borné régulier k <= 8 sans sorties anticipées divergentes est
exactement la forme d'un kernel, et la décision se prendra sur une
mesure G4 réelle (l'utilisateur a autorisé les campagnes G4 ; la
session gardée `session_campagne_v4_scale_g4.sh` est prête).

## État global côté échelle

n=8000 mono-thread : 5,4 min (reçu `ADDENDUM_CAMPAGNE_N8000`), fold
34 % du temps et pente la plus raide — la réécriture par tri du fold
(std::map → sort-based, GPU-alignée) est mon prochain chantier, avec
votre boule intérieure `B(m, R−δ)` en tâche ouverte si les compteurs de
génération la redésignent.
