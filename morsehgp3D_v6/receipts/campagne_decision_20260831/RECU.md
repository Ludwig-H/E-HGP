# Reçu — campagne au pin 518e2706, requalifiée « baseline de sonde PARTIELLE »

Statut : `baseline_sonde_partielle_pin_518e2706` — JAMAIS une décision E6 ni
un GO (requalification de l'auditeur du cinquième cycle, acceptée avant
lecture des résultats ; le titre « campagne de décision » du META est
CADUC). Aucun seuil nouveau n'a été choisi après lecture.

## Incident gravé : rebuild concurrent du binaire

Le binaire `build/v6/mhgp6` a été RECONSTRUIT par Claude à 12:58:38Z
(correctifs du cinquième cycle) pendant que la campagne l'exécutait run par
run. Conséquence :

- **32 runs sur 36 au binaire épinglé** (celui du META, pin 518e2706) ;
- **4 runs contaminés** (binaire post-rebuild, worktree non committé,
  format de sortie différent — reconnaissables à `vcensus prefiltre_nœuds=`) :
  `eight_clusters_16000_s5`, `eight_clusters_32000_s3/s4/s5`.

Validité par famille : `uniform`, `terrain_stationnaire`,
`scanline_stationnaire` = matrices 3×3 COMPLÈTES au pin (la cible de la
sonde de queue) ; `eight_clusters` = INVALIDE au pas 16000→32000 (aucune
pente eight_clusters opposable ici). Le validateur du pin (`pentes.py` de
518e2706) refuserait ce dossier (fail-closed sur les 4 fichiers au nouveau
format) : c'est le comportement attendu, aucun PENTES.txt n'est publié.
Leçon : ne JAMAIS reconstruire un binaire pendant qu'une campagne
l'exécute (même famille d'incident que l'édition d'un script en cours).

## Lecture post-hoc des octaves (ANALYSE_OCTAVES_20260831.txt)

Sur les 27 runs valides des trois familles cibles — lecture, pas décision :

- **uniform (contrôle)** : W1 vit aux octaves 5–7 (covers ≤ 128), pentes
  par octave 0,97–1,14 sur les trois graines. Aucune queue.
- **terrain_stationnaire** : à n=32000 g5, les octaves 10–11 (covers
  1024–2048, ~28 000 ancres au total) portent **61 % de W1** avec
  w1/ancre jusqu'à 28 000 ; pentes par octave 16000→32000 : o10=2,78,
  o11=3,88 (g5) — et déjà o10=2,36/o11=2,46 en g3.
- **scanline_stationnaire** : à n=32000 g5, les octaves 11–12 (covers
  2048–4096, ~26 000 ancres) portent **66 % de W1** (o12 : w1/ancre
  ≈ 91 000) ; pentes o12 : 4,30 (g3), 10,12 (g4), 5,02 (g5).
- **Requalification du « pic mono-graine »** : la queue d'ancres lourdes
  croît super-quadratiquement sur LES TROIS graines ; la graine ne décide
  que du moment où elle domine le total. Le signal E6 est donc un terme de
  queue structurel des surfaces stationnaires, pas un artefact de graine.
- Observation d'orientation (à confirmer par la sonde enrichie) : à o12
  scanline, w1/seed ≈ 91 ≪ taille de cover (4096) — l'arrêt précoce
  cœur/corde borne le scan par seed ; la masse vient du NOMBRE de seeds des
  ancres lourdes (~10⁷ à o12). Les vecteurs d'issues par octave
  (cellules/cœur/corde/passe2), câblés au pin suivant, diront quelle
  proportion meurt où — donnée manquante ici (ancien binaire).

## Suite

Campagne complète relancée au pin du cinquième cycle (b3e64205 ou
postérieur) : mêmes matrices, sonde enrichie (issues par octave, populations
déclarées, identités fermantes vérifiées par le validateur durci). C'est
elle, et elle seule, qui peut prétendre à un statut décisionnel — après
agrégateur inter-graines préenregistré.

## Provenance

- pin, hash du binaire épinglé, toolchain, matrice : `META.txt` (le titre
  « campagne de decision » y est requalifié par le présent reçu) ;
- codes : `STATUS.txt` (36 × code=0, DONE terminal) ; stderr vides ;
- hashes des 36 sorties : ajoutés au META à la clôture ;
- binaire post-rebuild des 4 runs contaminés :
  sha256 `4bbb257cd31413f2c1058ee7b873f2ffe84158e3ce299a76d1230e6ab3053359`
  (celui du reçu `portes_rapides_cycle5_20260831`), mtime 12:58:38Z ;
- machine : codespace 8 vCPU partagés — compteurs déterministes seulement,
  les `secs=` du STATUS ne sont pas des mesures.
