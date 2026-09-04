#!/usr/bin/env bash
# SONDE D'ABLATION du reduce — reçu FAIL-CLOSED (2 septembre 2026, arbre
# § 5.10 de REPONSE_AUDITEURS_MULTICPU_V6 « decomposer la fenetre AVANT
# d'ecrire un palier » ; fermeture minimale exigee par
# audits/ALERTE_SONDE_ABLATION_REDUCE_20260902.md, puis les cinq
# contre-fixtures encore vertes de la fin de son « Etat du WIP » et du § 5.21
# de REPONSE_AUDITEURS_MULTICPU_V6, puis les dents d'ETAT_COURANT du
# 2 septembre (ligne « sonde equilibree … harnais ») et de la fin du § 5.22 :
# compteurs facultatifs, profil malforme, repertoire vide, hashes de
# protocoles non recalcules, identite de cible, TOCTOU avant scellement,
# `diff` fail-open ; puis les deux coutures ACTIVES nommees par
# audits/REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md (« Reponse au verrou
# de pre-inscription 53610911 ») : outil final herite d'un PATH hostile et
# liaison exacte commande/META au regime, a la famille et a la vivacite).
# Doctrine :
#   - BORNES EXPLORATOIRES NON CAUSALES (statut=exploratory_noncausal_upper_bounds) :
#     binaire de PROFIL sous MHGP7_TESTING (mhgp7_profile_sonde), fenetres
#     par K (profil_reduce), join=1 pour isoler l'etage B. Chaque ablation
#     CHANGE l'objet (tuee code 4 par mhgp7_mutant_ablation-*) : ses sorties
#     ne valent que par leurs fenetres — jamais un mur, jamais un benchmark,
#     jamais un choix de palier. Le bras ablation-post-cle-factice est une
#     BORNE COMPOSITE (lecture keys[] + tri de cles egales), pas « lecture
#     seule » : il change aussi la distribution du tri de la fenetre
#     materialisation_tri_copie.
#   - OUTILS HORS PATH, MODE PRIVILEGIE (couture 1 des auditeurs du
#     2 septembre) : le lanceur se re-execute UNE fois sous `bash -p`
#     (aucune fonction importee de l'environnement BASH_FUNC_*, BASH_ENV /
#     ENV / SHELLOPTS / CDPATH ignores) et refuse (2) tout environnement de
#     chargement etranger (BASH_ENV, ENV, LD_PRELOAD, LD_AUDIT,
#     LD_LIBRARY_PATH, PYTHONHOME, PYTHONPATH definis). Puis, AVANT toute
#     ecriture, chaque outil externe employe (cmp cp chmod diff find git
#     lscpu mkdir mktemp mv readlink rm sha256sum sort taskset uname xargs ;
#     gcc facultatif, informatif) est resolu HORS PATH (`command -v -p`,
#     chemin standard des utilitaires, PATH jamais consulte), canonise
#     (`readlink -f`, lui-meme resolu en premier et point fixe de sa propre
#     canonisation), exige fichier regulier, executable et binaire ELF (magie
#     lue par le shell, aucun outil), hache (sha256), grave au META
#     (`outils=nom=chemin:sha256 …`) et N'EST PLUS APPELE QUE PAR CE CHEMIN
#     (variables MV, SHA256SUM, CMP, DIFF, FIND, SORT, TASKSET…). Un outil non
#     resoluble, non canonique, non regulier, non executable ou non ELF est
#     un REFUS 2 sans .partial. Avant publication, chaque outil est RELU et
#     son sha256 compare a `outils=` (INVALIDE 3 sinon). Tout le reste passe
#     par des builtins bash (EPOCHREALTIME pour les chronos, printf %()T
#     pour les dates, read pour /proc/loadavg et la magie ELF). Effet
#     mesure par la porte : un faux `mv` / `sha256sum` / `cmp` / `diff` /
#     `find` / `sort` / `taskset` en PATH — depose AVANT le lancement ou
#     APRES la resolution (par le binaire de sonde a son premier appel) —
#     n'est JAMAIS execute et ne laisse aucune trace ; une fonction `mv`
#     exportee est ignoree (mode privilegie). SEULE exception a la regle,
#     assumee : `python3` de PATH n'est employe que (a) pour la sonde
#     sys.executable qui determine l'interpreteur de la reagregation
#     (verifie point fixe + ELF, grave) et (b) pour la PREMIERE agregation,
#     dont le resume n'est jamais publie sans REAGREGATION bit a bit par
#     l'interpreteur absolu grave (`-I` : PYTHONPATH et site utilisateur
#     ignores). Ce qu'un environnement hostile AVANT la resolution peut
#     encore faire, honnetement : (1) un ELF imposteur (vrai `mv` renomme,
#     ou un cheval de Troie compile) DANS le chemin standard des
#     utilitaires (/bin, /usr/bin : ecriture root) est accepte et grave —
#     son sha256 est visible a l'auditeur (comparable au paquet de la
#     distribution), le lanceur ne l'authentifie pas ; (2) une fonction
#     exportee nommee `exec` (ou un BASH_ENV deja source par le premier
#     bash, avant sa premiere ligne) peut empecher le re-exec en mode
#     privilegie ou remplacer les builtins de ce prologue — c'est une
#     compromission du shell lui-meme, de la classe d'un bash ou d'un noyau
#     hostiles, hors de portee d'un script ; (3) un `python3` ELF de PATH
#     hostile (interpreteur reel accompagne d'un site hostile) est grave
#     comme interpreteur : couture residuelle deja documentee ci-dessous.
#   - TOPOLOGIE ATTESTEE (lscpu -p=CPU,CORE,SOCKET,ONLINE, /proc/self/status
#     Cpus_allowed_list, taskset -p) AVANT toute ecriture : META grave
#     `topologie=sockets= coeurs= fils= cpus_en_ligne=`, `cpuset=` (masque
#     et liste du lanceur) et `affinite=cpus= fils_materiels=
#     coeurs_physiques= sockets=` RECALCULEE depuis lscpu pour CPUS ; un CPU
#     de CPUS hors ligne ou hors du cpuset du lanceur est un REFUS 2. Le
#     defaut CPUS=0-7 designe, sur la machine de reference (lscpu atteste
#     le 2 septembre 2026 : AMD EPYC 9V74 sous hyperviseur, 1 socket,
#     4 coeurs, 2 fils par coeur, CPU 0-7 en ligne), HUIT FILS MATERIELS
#     SUR QUATRE COEURS PHYSIQUES — jamais « huit coeurs » ; le lanceur ne
#     prend pas cette description pour acquise, il la recalcule et la grave.
#   - LIAISON COMMANDE / META / SORTIE (couture 2) : chaque commande gravee
#     (`commande=` du .status) est reconstruite par l'agregateur depuis META
#     (famille, n, s, smax, seed, threads, fold_inflight, fold_join, cpus) en
#     argv EXACT (ni argument en plus, ni en moins, ni ordre different), la
#     ligne d'identite de la sortie (`famille= n= coord= s= smax= seed=
#     threads=`) doit lui etre EGALE champ a champ, `profil_kind fold_join=`
#     et `inflight_demande=` aussi, et `coord=` est identique entre tous les
#     tuples d'une meme taille ; tout ecart est un refus 1 (INVALIDE 3 ici).
#   - IDENTITE DE CIBLE (P2, claim BORNE) : mhgp7_profile_sonde est construit
#     sous MHGP7_TESTING et `mutants_enable` accepte TOUT nom de kMutants
#     (drop-nonmerge compris, verifie le 2 septembre : rc 0). Le lanceur ne
#     peut donc pas distinguer la cible d'un autre binaire de test par un
#     mutant produit ; il ne le pretend pas. Ce qu'il garantit : (1) les
#     sondes d'identite (accepte --inject=ablation-mat-sans-tris, refuse un
#     nom inconnu par le code 2 exact) ; (2) les SEULS --inject= qu'il emet
#     sont les trois ablations — ABLATIONS est verifie contre cette liste
#     avant toute ecriture (refus 2), chaque tuple la re-verifie avant
#     d'executer (un plan.txt altere en campagne => INVALIDE 3), et apres le
#     dernier tuple les lignes commande= des .status sont RECOMPTEES
#     (ensemble exact et une occurrence par tuple non temoin, sinon INVALIDE
#     3) ; META grave identite_cible, injections_autorisees (avant) et
#     injections_emises (apres) ; l'agregateur refuse tout .status dont le
#     --inject= n'est pas l'ablation de son bras.
#   - PLAN EQUILIBRE (carre de Williams 4x4 : ABCD BDAC CADB DCBA) : les
#     blocs sont les repetitions ; sur quatre blocs consecutifs chaque bras
#     occupe une fois chaque position et suit une fois chaque autre bras.
#     REPS doit etre un multiple de 4 (defaut 4), sinon REFUS ; le plan
#     (bloc, position, bras) est grave dans plan.txt AVANT le premier run et
#     la boucle l'execute tel quel. L'agregateur apparie PAR BLOC (bras −
#     aucune au sein du meme bloc et de la meme taille) puis publie
#     mediane/min/max des differences appariees ; les medianes brutes ne sont
#     qu'un second tableau.
#   - COPIE PRIVEE du binaire dans <OUT>.partial/bin/ (chmod 555), SEULE
#     executee (les sondes d'identite aussi) ; sha256 grave au META, verifie
#     AVANT et APRES chaque tuple (HASHES.txt) ; divergence => INVALIDE
#     (code 3), aucun mv de publication. Tout hash passe par la primitive
#     hash_de : exactement 64 hexadecimaux ou echec FATAL (un sha256sum muet
#     ne produit jamais une chaine vide « egale » a une autre chaine vide).
#   - PROTOCOLES ARCHIVES : protocole_lanceur.sh / protocole_agregateur.py
#     sont copies dans le reçu, leur sha256 grave au META et compare a la
#     source relue des la copie (INVALIDE si different) puis A NOUVEAU avant
#     publication (copie relue == META == source relue) ; l'agregateur les
#     recalcule et refuse toute difference.
#   - FAIL-CLOSED : liste de tailles vide, taille DUPLIQUEE (deux tuples
#     porteraient le meme tag <bras>_n<n>_r<bloc> et s'ecraseraient),
#     REPS <= 0, REPS non multiple de 4, CPUS invalide, hash illisible, bras
#     hors sonde, binaire qui n'accepte pas --inject=ablation-* (binaire
#     produit) ou qui accepte un nom inconnu => REFUS 2 avant tout run (le
#     .partial est retire) ; un run a code non nul => INVALIDE 3 immediat ;
#     runs_effectues et runs_attendus sont graves (cardinal exact bras x
#     tailles x repetitions) et l'agregateur (copie archivee
#     protocole_agregateur.py) les exige, exige l'ensemble EXACT bras x
#     tailles x repetitions, le triplet EXACT <tag>.txt|.err|.status par
#     tuple et rien d'autre dans out/, le plan de Williams (successions
#     ordonnees, pas seulement les positions), tout champ unique (META,
#     statut, plan, profil), la grammaire 64-hex de chaque hash, dix lignes
#     K a la grammaire stricte (jeton sans `=`, valeur non numerique, K non
#     entier, ligne tronquee = refus, jamais ignores), toutes les fenetres
#     finies (jamais un zero substitue), la racine du reçu exacte (aucun
#     repertoire vide ou inattendu) et rend 1 sinon.
#   - SCELLEMENT SANS TOCTOU : AVANT l'agregateur, l'empreinte sha256 de
#     chaque fichier du reçu est relevee (ce que l'agregateur va lire) ;
#     APRES, le manifeste SHA256SUMS est genere et doit porter exactement ces
#     empreintes plus resume.txt/resume.err (un fichier modifie entre la
#     lecture de l'agregateur et le scellement => INVALIDE 3 : une mutation
#     semantique n'est jamais publiee). Puis, sans autre operation entre eux
#     que ces controles : inventaire des repertoires (exactement bin/ et
#     out/, un repertoire vide ou inattendu => INVALIDE), inventaire final des
#     fichiers reguliers == entrees du manifeste (fichier apparu ou disparu
#     apres le manifeste => INVALIDE), protocoles archives relus == META ==
#     sources, copie privee relue == META, outils relus == META,
#     `sha256sum -c --strict`, et le `mv -T` de publication IMMEDIATEMENT
#     apres (un dossier de publication apparu pendant la campagne => INVALIDE,
#     jamais un reçu deplace DANS un dossier etranger). Toute comparaison de
#     listes passe par exiger_listes_egales : le rc de `diff` est CAPTURE (0 =
#     identique, 1 = different => INVALIDE, autre => INVALIDE « comparaison
#     impossible ») — jamais un `|| true`, jamais un test sur la seule sortie
#     vide. Le manifeste couvre TOUT fichier regulier sauf le seul
#     ./SHA256SUMS racine (un out/SHA256SUMS ou tout intrus est hache, donc
#     visible, puis refuse par l'inventaire). Un reçu INVALIDE reste en
#     `<OUT>.partial`, jamais publie.
#   - REAGREGATION DU JEU SCELLE (TOCTOU semantique, « Reception critique du
#     pin 1cb60655 » de l'alerte) : la premiere agregation passe par `python3`
#     en PATH ; un faux interpreteur peut relayer vers le vrai agregateur puis
#     muter META (vu par les empreintes d'avant agregation) ou FORGER le
#     resume qu'il relaie (invisible a toute empreinte : resume.txt nait de
#     lui). Donc, APRES le manifeste et sa premiere verification, le jeu
#     scelle est REAGREGE depuis la copie archivee protocole_agregateur.py par
#     l'INTERPRETEUR ABSOLU ET CANONIQUE resolu AVANT la campagne et grave au
#     META (`interpreteur=`, hors PATH : point fixe de sys.executable, binaire
#     ELF — un script relais ou un chemin qui se resout ailleurs est un REFUS
#     2 avant toute ecriture), hors du reçu, et compare BIT A BIT (stdout et
#     stderr, rc de `cmp` capture ET sha256 des deux fichiers) a resume.txt et
#     resume.err scelles ; puis TOUS les controles de publication sont
#     rejoues (repertoires, inventaire == manifeste, protocoles, copie privee,
#     outils, `sha256sum -c --strict`) et le `mv` suit immediatement. Toute
#     divergence => INVALIDE 3, jamais publie. Fenetre residuelle : un faux
#     interpreteur COMPILE qui se nomme lui-meme de facon coherente — aucun
#     controle interne ne peut le voir ; il est au moins grave au META pour
#     l'auditeur. (Le `mv` lui-meme n'est plus une fenetre : chemin absolu
#     grave, sha256 relu avant publication.)
#   - Worktree : commit et nombre de fichiers modifies graves ; si non nul,
#     `git diff HEAD` embarque dans worktree_diff.patch (+ statut porcelain et
#     `git diff HEAD --summary --stat` dans worktree_diff_summary.txt), sinon
#     worktree_diff=aucun.
# Porte : tests/sonde_ablation_gate.py (faux binaire rapide, vingt-trois
# scenes, cas (a)–(z)).
# Usage : sonde_ablation_reduce.sh OUT_DIR BIN_SONDE [N_LIST="8000 16000 32000"] [REPS=4]
#   (un troisieme argument VIDE est une liste vide, donc un refus — pas le defaut)
#   ou  : sonde_ablation_reduce.sh --inventaire DIR
#   (lecture seule : liste, telle que le manifeste la couvrirait, des fichiers
#   reguliers de DIR — seul ./SHA256SUMS racine exclu ; sert a la porte)

# --- Mode privilegie : re-exec UNE fois sous `bash -p` --------------------
# Aucune fonction importee de l'environnement (BASH_FUNC_x%%), BASH_ENV/ENV
# non sources, SHELLOPTS/BASHOPTS/CDPATH/GLOBIGNORE ignores. ${BASH} est le
# chemin du bash courant (pose par le shell lui-meme, jamais par l'env).
if [[ $- != *p* ]]; then
  if [[ -n "${MHGP7_SONDE_REEXEC-}" ]]; then
    printf 'REFUS : mode privilegie (bash -p) non obtenu apres re-exec\n' >&2
    exit 2
  fi
  MHGP7_SONDE_REEXEC=1 exec "${BASH}" -p "$0" "$@"
fi
set -u

refus() { printf 'REFUS : %s\n' "$1" >&2; exit 2; }

# Environnement de chargement etranger : refuse avant tout (un BASH_ENV a
# deja pu etre source par le premier bash, avant sa premiere ligne ; LD_*
# s'applique a CHAQUE outil ELF et au binaire de sonde ; PYTHON* a
# l'agregation). Lancer sous `env -u <VAR>` si la variable est legitime.
for v in BASH_ENV ENV LD_PRELOAD LD_AUDIT LD_LIBRARY_PATH PYTHONHOME PYTHONPATH; do
  [[ -z "${!v-}" ]] || refus "variable ${v} definie dans l'environnement (${!v}) : un code etranger peut se charger avant la premiere ligne du lanceur, dans chaque outil ELF ou dans l'agregateur — jamais grave (lancer sous env -u ${v})"
done

# Magie ELF lue par le shell (aucun outil) : exactement 7f 45 4c 46.
est_elf() {
  local m=""
  LC_ALL=C IFS= read -r -N 4 m < "$1" 2>/dev/null || return 1
  [[ "${m}" == $'\x7fELF' ]]
}

# --- Outils : resolus HORS PATH, canoniques, ELF, AVANT toute ecriture -----
# `command -v -p` cherche dans le chemin standard des utilitaires (PATH
# jamais consulte) ; une fonction, un builtin ou un alias repondrait un nom
# sans `/` : refus. readlink est resolu en premier (point fixe de sa propre
# canonisation), puis canonise chaque autre outil.
resoudre_outil() { # $1 = nom -> chemin canonique sur stdout, ou rc 1
  local p="" c=""
  p="$(command -v -p -- "$1" 2>/dev/null)" || return 1
  [[ "${p}" == /* ]] || return 1
  c="$("${READLINK}" -f -- "${p}" 2>/dev/null)" || return 1
  [[ "${c}" == /* && "${c}" != *[[:space:]]* && -f "${c}" && -x "${c}" ]] || return 1
  est_elf "${c}" || return 1
  printf '%s\n' "${c}"
}
READLINK="$(command -v -p -- readlink 2>/dev/null)" || refus "readlink introuvable hors PATH (command -v -p) — aucun outil ne peut etre canonise"
[[ "${READLINK}" == /* ]] || refus "readlink resolu en « ${READLINK} » (fonction, builtin ou alias) — jamais grave"
READLINK_CANON="$("${READLINK}" -f -- "${READLINK}" 2>/dev/null)" || refus "readlink (${READLINK}) ne se canonise pas lui-meme"
[[ "${READLINK_CANON}" == /* && "${READLINK_CANON}" != *[[:space:]]* && -f "${READLINK_CANON}" && -x "${READLINK_CANON}" ]] \
  || refus "readlink canonique (${READLINK_CANON:-vide}) absent, non regulier, non executable ou avec blanc"
est_elf "${READLINK_CANON}" || refus "readlink canonique (${READLINK_CANON}) n'est pas un binaire ELF"
[[ "$("${READLINK_CANON}" -f -- "${READLINK_CANON}" 2>/dev/null)" == "${READLINK_CANON}" ]] \
  || refus "readlink canonique (${READLINK_CANON}) n'est pas le point fixe de sa propre canonisation"
READLINK="${READLINK_CANON}"
# Liste FERMEE des outils externes employes par ce lanceur (la porte
# remplace cette ligne pour exiger un outil inexistant ou non ELF).
OUTILS_REQUIS="cmp cp chmod diff find git lscpu mkdir mktemp mv readlink rm sha256sum sort taskset uname xargs"
declare -A OUTIL=()
for t in ${OUTILS_REQUIS}; do
  OUTIL["${t}"]="$(resoudre_outil "${t}")" \
    || refus "outil ${t} non resoluble hors PATH (command -v -p), non canonique, non regulier, non executable ou non ELF — jamais grave, aucune ecriture"
done
[[ "${OUTIL[readlink]}" == "${READLINK}" ]] || refus "readlink resolu deux fois vers deux chemins (${READLINK} / ${OUTIL[readlink]})"
GCC=""
GCC="$(resoudre_outil gcc 2>/dev/null)" || GCC=""   # facultatif, informatif seulement
: # ANCRE_PORTE_OUTILS (tests/sonde_ablation_gate.py) : un lanceur MUTANT substitue ici un outil resolu par un faux ; aucun effet ici
CMP="${OUTIL[cmp]}" CP="${OUTIL[cp]}" CHMOD="${OUTIL[chmod]}" DIFF="${OUTIL[diff]}" FIND="${OUTIL[find]}"
GIT="${OUTIL[git]}" LSCPU="${OUTIL[lscpu]}" MKDIR="${OUTIL[mkdir]}" MKTEMP="${OUTIL[mktemp]}" MV="${OUTIL[mv]}"
RM="${OUTIL[rm]}" SHA256SUM="${OUTIL[sha256sum]}" SORT="${OUTIL[sort]}" TASKSET="${OUTIL[taskset]}"
UNAME="${OUTIL[uname]}" XARGS="${OUTIL[xargs]}"

# Primitive de hash FATALE : imprime le sha256 (64 hexadecimaux) ou rend 1.
# Jamais une chaine vide : un sha256sum muet, absent ou tronque est un echec.
# (Un seul argument, jamais `--` : tout chemin hache ici est absolu ou
# prefixe par le reçu ; la porte distingue « un fichier » de « generation
# du manifeste » au nombre d'arguments.)
hash_de() {
  local h="" reste=""
  read -r h reste < <("${SHA256SUM}" "$1" 2>/dev/null) || true
  [[ "${h}" =~ ^[0-9a-f]{64}$ ]] || return 1
  printf '%s\n' "${h}"
}
# sha256 de chaque outil resolu (sha256sum compris), grave au META en ordre
# fixe nom=chemin:sha256 ; un hash illisible est un refus avant toute ecriture.
declare -A OUTIL_H=()
OUTILS_META=""
for t in ${OUTILS_REQUIS}; do
  OUTIL_H["${t}"]="$(hash_de "${OUTIL[${t}]}")" \
    || refus "sha256 de l'outil ${t} (${OUTIL[${t}]}) illisible (sha256sum muet ou hors grammaire 64-hex) — aucun hash vide n'est grave, aucune ecriture"
  OUTILS_META="${OUTILS_META}${OUTILS_META:+ }${t}=${OUTIL[${t}]}:${OUTIL_H[${t}]}"
done
if [[ -n "${GCC}" ]]; then
  GCC_H="$(hash_de "${GCC}")" && OUTILS_META="${OUTILS_META} gcc=${GCC}:${GCC_H}" || GCC=""
fi

# Nombre de lignes d'un fichier ou d'un flux (builtin, jamais wc).
compter_lignes() {
  local n=0 l=""
  while IFS= read -r l || [[ -n "${l}" ]]; do n=$((n + 1)); done
  printf '%s\n' "${n}"
}
# Inventaire du manifeste : tout fichier regulier, seul ./SHA256SUMS racine
# exclu (JAMAIS `! -name SHA256SUMS`, qui rendrait un out/SHA256SUMS invisible).
lister_fichiers() {
  "${FIND}" . -type f ! -path ./SHA256SUMS -printf '%P\n' | LC_ALL=C "${SORT}"
}
if [[ "${1-}" == "--inventaire" ]]; then
  [[ -d "${2-}" ]] || refus "--inventaire exige un dossier existant"
  ( cd "$2" && lister_fichiers )
  exit $?
fi

OUT="${1:?dossier de reçu requis}"
BIN_SRC="${2:?binaire mhgp7_profile_sonde requis}"
N_LIST="${3-8000 16000 32000}"
REPS="${4-4}"
THREADS="${THREADS:-8}"
CPUS="${CPUS:-0-7}"
FAMILY="${FAMILY:-uniform}"
ABLATIONS="aucune ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice"
# Les SEULS --inject= que ce lanceur emet (identite de cible, claim borne :
# la cible accepte tout kMutants, la sonde ne selectionne que ces trois-la).
INJECTIONS_SONDE="ablation-mat-sans-copie ablation-mat-sans-tris ablation-post-cle-factice"
IDENTITE_CIBLE="mhgp7_profile_sonde (accepte tout mutant de kMutants ; seules les ablations sont selectionnees ici)"
# Carre de Williams pour quatre traitements : chaque lettre une fois par
# position et chaque succession ordonnee (X puis Y) exactement une fois.
WILLIAMS=("A B C D" "B D A C" "C A D B" "D C B A")

bras_de() {
  case "$1" in
    A) echo aucune ;;
    B) echo ablation-mat-sans-copie ;;
    C) echo ablation-mat-sans-tris ;;
    D) echo ablation-post-cle-factice ;;
    *) refus "lettre de plan inconnue ($1)" ;;
  esac
}
# Une ablation de la sonde, et rien d'autre (liste fermee, jamais un mutant
# produit de kMutants meme si mhgp7_profile_sonde l'accepte).
est_ablation_sonde() {
  case "$1" in
    ablation-mat-sans-copie|ablation-mat-sans-tris|ablation-post-cle-factice) return 0 ;;
  esac
  return 1
}
# Liste de CPU `a-b[,c[,d-e]]` -> liste d'entiers (une par ligne, ordre de la
# grammaire, sans doublon), ou rc 1 (grammaire, plage inversee ou > 4096).
expandre_cpus() {
  local spec="$1" part a b i
  local -A vus=()
  [[ "${spec}" =~ ^[0-9]+(-[0-9]+)?(,[0-9]+(-[0-9]+)?)*$ ]] || return 1
  local -a parts=()
  IFS=, read -r -a parts <<< "${spec}"
  for part in "${parts[@]}"; do
    a="${part%-*}"; b="${part#*-}"
    a=$((10#${a})); b=$((10#${b}))
    [[ "${a}" -le "${b}" && $((b - a)) -le 4096 ]] || return 1
    for ((i = a; i <= b; i++)); do
      [[ -n "${vus[${i}]+x}" ]] && continue
      vus["${i}"]=1
      printf '%s\n' "${i}"
    done
  done
}
# Liste d'entiers TRIES (une par ligne sur stdin) -> plages compactes a-b,c.
compacter_cpus() {
  local out="" deb="" fin="" x
  while IFS= read -r x; do
    [[ -n "${x}" ]] || continue
    if [[ -z "${deb}" ]]; then deb="${x}"; fin="${x}"; continue; fi
    if [[ "${x}" -eq $((fin + 1)) ]]; then fin="${x}"; continue; fi
    out="${out}${out:+,}${deb}"; [[ "${deb}" -eq "${fin}" ]] || out="${out}-${fin}"
    deb="${x}"; fin="${x}"
  done
  if [[ -n "${deb}" ]]; then
    out="${out}${out:+,}${deb}"; [[ "${deb}" -eq "${fin}" ]] || out="${out}-${fin}"
  fi
  printf '%s\n' "${out}"
}
date_utc() { TZ=UTC printf '%(%Y-%m-%dT%H:%M:%SZ)T\n' -1; }

# --- Refus avant toute ecriture ------------------------------------------
if [[ -e "${OUT}" || -e "${OUT}.partial" ]]; then
  refus "dossier de reçu preexistant (${OUT}[.partial]) — un reçu ne s'ecrit jamais en place"
fi
NN=0
N_NORM=""
for n in ${N_LIST}; do
  case "${n}" in ''|*[!0-9]*) refus "taille non entiere (${n})" ;; esac
  [[ "${#n}" -le 12 ]] || refus "taille trop longue (${n})"
  n=$((10#${n}))   # forme canonique : 064 et 64 sont la meme taille (meme tag)
  [[ "${n}" -gt 0 ]] || refus "taille nulle"
  case " ${N_NORM} " in
    *" ${n} "*) refus "taille dupliquee dans N_LIST (${n}) — deux tuples porteraient le meme tag <bras>_n${n}_r<bloc> et s'ecraseraient" ;;
  esac
  N_NORM="${N_NORM}${N_NORM:+ }${n}"
  NN=$((NN + 1))
done
N_LIST="${N_NORM}"
[[ -n "${N_LIST}" ]] || refus "liste de tailles vide (N_LIST) — un reçu vide n'est pas un reçu"
case "${REPS}" in ''|*[!0-9]*) refus "REPS non entier (${REPS})" ;; esac
REPS=$((10#${REPS}))
[[ "${REPS}" -gt 0 ]] || refus "REPS <= 0 — un reçu vide n'est pas un reçu"
[[ $((REPS % 4)) -eq 0 ]] || refus "REPS=${REPS} n'est pas un multiple de 4 (carre de Williams 4x4 : chaque bras une fois a chaque position par groupe de quatre blocs)"
case "${THREADS}" in ''|*[!0-9]*) refus "THREADS non entier (${THREADS})" ;; esac
THREADS=$((10#${THREADS}))
[[ "${THREADS}" -gt 0 ]] || refus "THREADS <= 0"
case "${FAMILY}" in ''|*[!a-z0-9_]*) refus "FAMILY hors grammaire ^[a-z0-9_]+$ (${FAMILY})" ;; esac
# Identite de cible tenue par les bras : le temoin et les trois ablations,
# rien d'autre — un ABLATIONS edite vers un mutant produit est un refus.
case " ${ABLATIONS} " in *" aucune "*) ;; *) refus "bras temoin aucune absent de ABLATIONS" ;; esac
NBRAS=0
for abl in ${ABLATIONS}; do
  NBRAS=$((NBRAS + 1))
  [[ "${abl}" == aucune ]] || est_ablation_sonde "${abl}" \
    || refus "bras ${abl} hors sonde : les seuls --inject= emis par ce lanceur sont les trois ablations (${INJECTIONS_SONDE}) — jamais un mutant produit de kMutants, meme si mhgp7_profile_sonde l'accepte"
done
[[ "${NBRAS}" -eq 4 ]] || refus "ABLATIONS porte ${NBRAS} bras au lieu de 4 (temoin + trois ablations)"
[[ -f "${BIN_SRC}" && -x "${BIN_SRC}" ]] || refus "binaire absent, non regulier ou non executable (${BIN_SRC})"

# --- Topologie et cpuset : attestes AVANT toute ecriture ------------------
CPUS_LISTE="$(expandre_cpus "${CPUS}")" || refus "CPUS=${CPUS} hors grammaire a-b[,c[,d-e]] (entiers, tirets, virgules ; plage inversee ou > 4096 refusee)"
"${TASKSET}" -c "${CPUS}" "${BASH}" -p -c ':' >/dev/null 2>&1 || refus "CPUS=${CPUS} invalide pour taskset"
declare -A CORE_DE=() SOCKET_DE=()
EN_LIGNE=""
LSCPU_P="$(LC_ALL=C "${LSCPU}" -p=CPU,CORE,SOCKET,ONLINE 2>/dev/null)" || refus "lscpu -p=CPU,CORE,SOCKET,ONLINE en echec : topologie non attestable, jamais gravee"
while IFS=, read -r l_cpu l_core l_socket l_online; do
  [[ -n "${l_cpu}" && "${l_cpu}" != \#* ]] || continue
  [[ "${l_cpu}" =~ ^[0-9]+$ ]] || refus "lscpu -p illisible (CPU « ${l_cpu} »)"
  [[ "${l_online}" == Y ]] || continue
  [[ "${l_core}" =~ ^[0-9]+$ && "${l_socket}" =~ ^[0-9]+$ ]] || refus "lscpu -p illisible (CPU ${l_cpu} en ligne sans coeur/socket : « ${l_core} »,« ${l_socket} »)"
  CORE_DE["${l_cpu}"]="${l_core}"
  SOCKET_DE["${l_cpu}"]="${l_socket}"
  EN_LIGNE="${EN_LIGNE}${l_cpu}"$'\n'
done <<< "${LSCPU_P}"
[[ -n "${EN_LIGNE}" ]] || refus "lscpu -p : aucun CPU en ligne lisible — topologie non attestable"
N_FILS="$(printf '%s' "${EN_LIGNE}" | compter_lignes)"
declare -A COEURS_VUS=() SOCKETS_VUS=()
for c in "${!CORE_DE[@]}"; do COEURS_VUS["${CORE_DE[${c}]}"]=1; SOCKETS_VUS["${SOCKET_DE[${c}]}"]=1; done
N_COEURS="${#COEURS_VUS[@]}"
N_SOCKETS="${#SOCKETS_VUS[@]}"
EN_LIGNE_COMPACT="$(printf '%s' "${EN_LIGNE}" | LC_ALL=C "${SORT}" -n | compacter_cpus)"
# cpuset du lanceur : /proc/self/status (lu par le shell) et taskset -p.
CPUSET=""
while read -r s_cle s_val s_reste; do
  [[ "${s_cle}" == "Cpus_allowed_list:" ]] && CPUSET="${s_val}"
done < /proc/self/status
[[ -n "${CPUSET}" ]] || refus "cpuset du lanceur illisible (/proc/self/status Cpus_allowed_list)"
CPUSET_LISTE="$(expandre_cpus "${CPUSET}")" || refus "cpuset du lanceur hors grammaire (${CPUSET})"
MASQUE_LIGNE="$("${TASKSET}" -p $$ 2>/dev/null)" || refus "taskset -p \$\$ en echec : masque d'affinite non attestable"
MASQUE="${MASQUE_LIGNE##* }"
[[ "${MASQUE}" =~ ^[0-9a-f,]+$ ]] || refus "taskset -p \$\$ illisible (« ${MASQUE_LIGNE} »)"
declare -A CPUSET_VUS=() AFF_COEURS=() AFF_SOCKETS=()
while IFS= read -r c; do [[ -n "${c}" ]] && CPUSET_VUS["${c}"]=1; done <<< "${CPUSET_LISTE}"
FILS_MAT=0
while IFS= read -r c; do
  [[ -n "${c}" ]] || continue
  [[ -n "${CPUSET_VUS[${c}]+x}" ]] || refus "CPU ${c} de CPUS=${CPUS} hors du cpuset du lanceur (${CPUSET}) : l'affinite demandee ne serait pas effective — jamais gravee"
  [[ -n "${CORE_DE[${c}]+x}" ]] || refus "CPU ${c} de CPUS=${CPUS} hors ligne ou inexistant d'apres lscpu (en ligne : ${EN_LIGNE_COMPACT})"
  AFF_COEURS["${CORE_DE[${c}]}"]=1
  AFF_SOCKETS["${SOCKET_DE[${c}]}"]=1
  FILS_MAT=$((FILS_MAT + 1))
done <<< "${CPUS_LISTE}"
COEURS_PHYS="${#AFF_COEURS[@]}"
SOCKETS_AFF="${#AFF_SOCKETS[@]}"
[[ "${FILS_MAT}" -gt 0 && "${COEURS_PHYS}" -gt 0 ]] || refus "affinite CPUS=${CPUS} vide"

# --- Interpreteur de la reagregation : resolu AVANT la campagne, hors PATH --
# `python3` en PATH ne sert qu'a la sonde sys.executable et a la premiere
# agregation ; la reagregation du jeu scelle passe par ce chemin ABSOLU et
# CANONIQUE (readlink -f), grave au META. Un relais (script ou chemin qui se
# resout ailleurs) est refuse : le chemin doit etre son propre
# sys.executable (point fixe) et un binaire ELF.
INTERPRETEUR="$(python3 -c 'import sys; print(sys.executable)' 2>/dev/null)" \
  || refus "interpreteur python3 introuvable ou muet (python3 -c 'import sys; print(sys.executable)')"
case "${INTERPRETEUR}" in
  /*) ;;
  *) refus "interpreteur python3 non absolu (${INTERPRETEUR:-vide}) — jamais grave" ;;
esac
case "${INTERPRETEUR}" in *[[:space:]]*) refus "interpreteur python3 avec blanc (${INTERPRETEUR}) — jamais grave" ;; esac
INTERPRETEUR="$("${READLINK}" -f -- "${INTERPRETEUR}" 2>/dev/null)" || refus "interpreteur python3 non canonisable (readlink -f)"
[[ -f "${INTERPRETEUR}" && -x "${INTERPRETEUR}" ]] \
  || refus "interpreteur ${INTERPRETEUR:-vide} absent, non regulier ou non executable"
FIXE="$("${INTERPRETEUR}" -c 'import sys; print(sys.executable)' 2>/dev/null)" \
  || refus "interpreteur ${INTERPRETEUR} muet ou en echec sur sys.executable"
FIXE_CANON="$("${READLINK}" -f -- "${FIXE}" 2>/dev/null || printf '%s' "${FIXE}")"
[[ "${FIXE_CANON}" == "${INTERPRETEUR}" ]] \
  || refus "interpreteur ${INTERPRETEUR} ne se resout pas sur lui-meme (sys.executable=${FIXE:-vide}) : relais ou wrapper, jamais grave"
est_elf "${INTERPRETEUR}" \
  || refus "interpreteur ${INTERPRETEUR} n'est pas un binaire ELF : script relais, jamais grave"

case "$0" in */*) HERE="${0%/*}" ;; *) HERE="." ;; esac
HERE="$(cd "${HERE}" && pwd)"
REPO_ROOT="$(cd "${HERE}/../.." && pwd)"
SRC_PATHS="morsehgp3D_v7/src morsehgp3D_v7/cli morsehgp3D_v7/CMakeLists.txt morsehgp3D_v7/bench"
PIN="$("${GIT}" -C "${REPO_ROOT}" rev-parse HEAD 2>/dev/null || echo hors_depot)"
# shellcheck disable=SC2086
DIRTY="$("${GIT}" -C "${REPO_ROOT}" status --porcelain -- ${SRC_PATHS} 2>/dev/null | compter_lignes)"

# --- Copie privee et sondes d'identite (sur la copie, jamais la source) ----
WORKD="${OUT}.partial"
"${MKDIR}" -p "${WORKD}/out" "${WORKD}/bin" || refus "creation de ${WORKD} impossible"
BIN="${WORKD}/bin/mhgp7_profile_sonde"
"${CP}" -- "${BIN_SRC}" "${BIN}" && "${CHMOD}" 555 "${BIN}" || { "${RM}" -rf "${WORKD}"; refus "copie privee impossible"; }
H="$(hash_de "${BIN}")" || { "${RM}" -rf "${WORKD}"; refus "sha256 de la copie privee illisible (sha256sum muet ou hors grammaire 64-hex) — aucun hash vide n'est grave"; }
H_SRC="$(hash_de "${BIN_SRC}")" || { "${RM}" -rf "${WORKD}"; refus "sha256 de la source illisible (sha256sum muet ou hors grammaire 64-hex)"; }
[[ "${H}" == "${H_SRC}" ]] || { "${RM}" -rf "${WORKD}"; refus "copie privee (${H}) != source relue (${H_SRC}) — copie concurrente ou dechiree"; }
# Cible de sonde : accepte un nom d'ablation CONNU (un binaire produit refuse
# --inject= tout court, code 2) ET refuse un nom INCONNU (code 2 exact).
if ! "${BIN}" --family=uniform --n=64 --threads=1 --inject=ablation-mat-sans-tris >/dev/null 2>&1; then
  "${RM}" -rf "${WORKD}"
  refus "${BIN_SRC} n'accepte pas --inject=ablation-* (binaire produit, ou pas la cible mhgp7_profile_sonde)"
fi
rc=0
"${BIN}" --family=uniform --n=64 --threads=1 --inject=ablation-inconnue >/dev/null 2>&1 || rc=$?
if [[ "${rc}" -ne 2 ]]; then
  "${RM}" -rf "${WORKD}"
  refus "${BIN_SRC} ne refuse pas un --inject= inconnu par le code 2 (code ${rc}) : pas la cible mhgp7_profile_sonde"
fi

invalide() {
  printf 'campagne INVALIDE : %s\n' "$1" >> "${WORKD}/META.txt"
  printf 'INVALIDE : %s — reçu laisse en %s, jamais publie\n' "$1" "${WORKD}" >&2
  exit 3
}
# Comparaison EXACTE de deux listes (une entree par ligne, deja triees) :
# le rc de diff est CAPTURE — 0 = identique (et sortie vide), 1 = different
# => INVALIDE avec les ecarts, tout autre (diff absent, en erreur, muet)
# => INVALIDE « comparaison impossible ». Jamais un `|| true`, jamais un test
# sur la seule sortie vide.
exiger_listes_egales() { # $1 = libelle, $2 = attendu, $3 = obtenu
  local sortie rc=0 ecarts="" n=0 l
  sortie="$(LC_ALL=C "${DIFF}" <(printf '%s\n' "$2") <(printf '%s\n' "$3") 2>&1)" || rc=$?
  case "${rc}" in
    0) [[ -z "${sortie}" ]] || invalide "$1 : diff rc=0 mais sortie non vide (${sortie:0:200})" ;;
    1) while IFS= read -r l; do
         [[ "${l}" == [\<\>]* ]] || continue
         n=$((n + 1)); [[ "${n}" -le 6 ]] && ecarts="${ecarts}${l} "
       done <<< "${sortie}"
       invalide "$1 (< attendu absent, > inattendu) : ${ecarts}" ;;
    *) invalide "$1 : comparaison impossible (diff rc=${rc}${sortie:+ : ${sortie:0:200}}) — jamais fail-open" ;;
  esac
}
H2="$(hash_de "${BIN}")" || invalide "sha256 de la copie privee illisible apres les sondes d'identite (sha256sum muet ou hors grammaire 64-hex)"
[[ "${H2}" == "${H}" ]] || invalide "copie privee alteree par les sondes d'identite (${H2} != ${H})"

# --- Plan equilibre grave AVANT le premier run ----------------------------
{
  echo "# plan equilibre : carre de Williams 4x4 (A=aucune B=ablation-mat-sans-copie C=ablation-mat-sans-tris D=ablation-post-cle-factice)"
  echo "# lignes : $(printf '%s | ' "${WILLIAMS[@]}")bloc b utilise la ligne ((b-1) mod 4) ; a l'interieur d'un bloc les tailles sont parcourues dans l'ordre n_list, et pour chaque taille les quatre bras dans l'ordre de la ligne"
  for ((rep = 1; rep <= REPS; rep++)); do
    pos=0
    for lettre in ${WILLIAMS[$(((rep - 1) % 4))]}; do
      pos=$((pos + 1))
      echo "bloc=${rep} position=${pos} bras=$(bras_de "${lettre}")"
    done
  done
} > "${WORKD}/plan.txt"

# Protocoles archives : copie == source relue, des maintenant (et a nouveau
# avant publication). Un hash grave est un hash relu, jamais suppose.
"${CP}" -- "${HERE}/sonde_ablation_reduce.sh" "${WORKD}/protocole_lanceur.sh" || invalide "archivage du lanceur impossible"
"${CP}" -- "${HERE}/sonde_ablation_reduce.py" "${WORKD}/protocole_agregateur.py" || invalide "archivage de l'agregateur impossible"
H_LANCEUR="$(hash_de "${WORKD}/protocole_lanceur.sh")" || invalide "sha256 du lanceur archive illisible (hors grammaire 64-hex)"
H_AGREGATEUR="$(hash_de "${WORKD}/protocole_agregateur.py")" || invalide "sha256 de l'agregateur archive illisible (hors grammaire 64-hex)"
H_LANCEUR_SRC="$(hash_de "${HERE}/sonde_ablation_reduce.sh")" || invalide "sha256 du lanceur source illisible (hors grammaire 64-hex)"
H_AGREGATEUR_SRC="$(hash_de "${HERE}/sonde_ablation_reduce.py")" || invalide "sha256 de l'agregateur source illisible (hors grammaire 64-hex)"
[[ "${H_LANCEUR}" == "${H_LANCEUR_SRC}" ]] || invalide "protocole_lanceur.sh archive (${H_LANCEUR}) != source relue (${H_LANCEUR_SRC})"
[[ "${H_AGREGATEUR}" == "${H_AGREGATEUR_SRC}" ]] || invalide "protocole_agregateur.py archive (${H_AGREGATEUR}) != source relue (${H_AGREGATEUR_SRC})"
{
  LC_ALL=C "${LSCPU}" 2>&1
  echo "# lscpu -p=CPU,CORE,SOCKET,ONLINE (source de topologie= et affinite= du META) :"
  printf '%s\n' "${LSCPU_P}"
  echo "# /proc/self/status Cpus_allowed_list du lanceur : ${CPUSET}"
  echo "# taskset -p du lanceur : ${MASQUE_LIGNE}"
} > "${WORKD}/lscpu.txt" || invalide "ecriture de lscpu.txt impossible"

WT_DIFF="aucun"
if [[ "${DIRTY}" -ne 0 ]]; then
  # shellcheck disable=SC2086
  "${GIT}" -C "${REPO_ROOT}" diff HEAD -- ${SRC_PATHS} > "${WORKD}/worktree_diff.patch" 2>/dev/null || true
  {
    echo "# git status --porcelain (les fichiers non suivis '??' ne figurent pas dans le patch) :"
    # shellcheck disable=SC2086
    "${GIT}" -C "${REPO_ROOT}" status --porcelain -- ${SRC_PATHS} 2>/dev/null
    echo "# git diff HEAD --summary --stat (modes et volumes) :"
    # shellcheck disable=SC2086
    "${GIT}" -C "${REPO_ROOT}" diff HEAD --summary --stat -- ${SRC_PATHS} 2>/dev/null
  } > "${WORKD}/worktree_diff_summary.txt"
  WT_DIFF="worktree_diff.patch (git diff HEAD -- ${SRC_PATHS} ; resume : worktree_diff_summary.txt)"
fi

{
  echo "schema=e-hgp.sonde-ablation-reduce.v5"
  echo "date_utc=$(date_utc)"
  echo "commit=${PIN}"
  echo "worktree_sources_modifies=${DIRTY}"
  echo "worktree_diff=${WT_DIFF}"
  echo "binaire_source=${BIN_SRC}"
  echo "binaire_prive=bin/mhgp7_profile_sonde (copie immuable chmod 555, seule executee ; sha256 verifie avant et apres chaque tuple, HASHES.txt)"
  echo "binaire_sha256=${H}"
  echo "identite_cible=${IDENTITE_CIBLE}"
  echo "injections_autorisees=${INJECTIONS_SONDE}"
  echo "famille=${FAMILY}"
  echo "n_list=${N_LIST}"
  echo "reps=${REPS}"
  echo "parametres=threads=${THREADS} cpus=${CPUS} fold_inflight=2 fold_join=1 seed=3 s=8 smax=11"
  echo "topologie=sockets=${N_SOCKETS} coeurs=${N_COEURS} fils=${N_FILS} cpus_en_ligne=${EN_LIGNE_COMPACT} (lscpu -p=CPU,CORE,SOCKET,ONLINE, atteste avant toute ecriture ; lscpu.txt)"
  echo "cpuset=${CPUSET} masque=${MASQUE} (/proc/self/status Cpus_allowed_list et taskset -p du lanceur ; CPUS=${CPUS} inclus, sinon refus)"
  echo "affinite=cpus=${CPUS} fils_materiels=${FILS_MAT} coeurs_physiques=${COEURS_PHYS} sockets=${SOCKETS_AFF} (${FILS_MAT} fils materiels sur ${COEURS_PHYS} coeurs physiques d'apres lscpu — jamais « ${FILS_MAT} coeurs »)"
  echo "outils=${OUTILS_META}"
  echo "ablations=${ABLATIONS}"
  echo "plan=williams_4x4 blocs=${REPS} (plan.txt grave avant le premier run ; appariement par bloc dans l'agregateur)"
  echo "etiquette_ablation-post-cle-factice=borne composite (lecture keys[] + tri de cles egales) — jamais « lecture seule »"
  echo "sha256_lanceur=${H_LANCEUR}"
  echo "sha256_agregateur=${H_AGREGATEUR}"
  echo "interpreteur=${INTERPRETEUR}"
  echo "libstdcxx=$("${READLINK}" -f -- /usr/lib/x86_64-linux-gnu/libstdc++.so.6 2>/dev/null || echo inconnu)"
  if [[ -n "${GCC}" ]]; then echo "gcc=$("${GCC}" -dumpfullversion 2>/dev/null || echo inconnu)"; else echo "gcc=inconnu (non resolu hors PATH)"; fi
  echo "hote=$("${UNAME}" -srm)"
  echo "statut=exploratory_noncausal_upper_bounds (bornes exploratoires non causales sur binaire instrumente, join=1 : jamais un benchmark, jamais un mur, jamais un choix de palier)"
} > "${WORKD}/META.txt"
: > "${WORKD}/HASHES.txt"

# Chrono en microsecondes entieres depuis EPOCHREALTIME (builtin ; le
# separateur decimal depend de la locale : retire).
micros() { local t="${EPOCHREALTIME}"; t="${t/[.,]/}"; printf '%s\n' "$((10#${t}))"; }

run_one() { # $1 = ablation, $2 = n, $3 = bloc (repetition), $4 = position dans le bloc
  local abl="$1" n="$2" rep="$3" pos="$4"
  local tag="${abl}_n${n}_r${rep}"
  # Garde d'identite AVANT toute execution : un bras qui n'est ni le temoin
  # ni une ablation de la sonde (plan.txt altere en campagne) n'emet rien.
  case "${abl}" in
    aucune) ;;
    *) est_ablation_sonde "${abl}" || invalide "bras ${abl} hors sonde au tuple ${tag} (plan.txt altere ?) — aucun --inject= hors des trois ablations n'est emis" ;;
  esac
  local inj=()
  [[ "${abl}" == "aucune" ]] || inj=("--inject=${abl}")
  local load_before load_after t0 t1 d hb ha rc=0 l1 l2 l3 reste duree
  hb="$(hash_de "${BIN}")" || invalide "hash AVANT ${tag} illisible (sha256sum muet ou hors grammaire 64-hex) — jamais un hash vide"
  [[ "${hb}" == "${H}" ]] || invalide "hash AVANT ${tag} : ${hb} != ${H} (copie privee alteree)"
  read -r l1 l2 l3 reste < /proc/loadavg; load_before="${l1} ${l2} ${l3}"
  t0="$(micros)"
  "${TASKSET}" -c "${CPUS}" "${BIN}" "--family=${FAMILY}" "--n=${n}" --s=8 --smax=11 --seed=3 \
    "--threads=${THREADS}" --fold-inflight=2 --fold-join=1 "${inj[@]}" \
    < /dev/null > "${WORKD}/out/${tag}.txt" 2> "${WORKD}/out/${tag}.err" || rc=$?
  t1="$(micros)"
  read -r l1 l2 l3 reste < /proc/loadavg; load_after="${l1} ${l2} ${l3}"
  d=$((t1 - t0)); [[ "${d}" -ge 0 ]] || d=0
  duree="$((d / 1000000)).$(printf '%03d' $(((d % 1000000) / 1000)))"
  ha="$(hash_de "${BIN}")" || invalide "hash APRES ${tag} illisible (sha256sum muet ou hors grammaire 64-hex) — jamais un hash vide"
  {
    echo "ablation=${abl} n=${n} rep=${rep} position=${pos} code=${rc}"
    echo "commande=taskset -c ${CPUS} bin/mhgp7_profile_sonde --family=${FAMILY} --n=${n} --s=8 --smax=11 --seed=3 --threads=${THREADS} --fold-inflight=2 --fold-join=1 ${inj[*]:-}"
    echo "duree_s=${duree}"
    echo "loadavg_avant=${load_before}"
    echo "loadavg_apres=${load_after}"
    echo "sha256_avant=${hb}"
    echo "sha256_apres=${ha}"
  } > "${WORKD}/out/${tag}.status"
  echo "${tag} avant=${hb} apres=${ha}" >> "${WORKD}/HASHES.txt"
  echo "--- fini ${tag} (bloc ${rep} position ${pos}, code=${rc}, ${duree}s)"
  [[ "${ha}" == "${H}" ]] || invalide "hash APRES ${tag} : ${ha} != ${H} (copie privee alteree pendant le tuple)"
  [[ "${rc}" -eq 0 ]] || invalide "run ${tag} en echec (code ${rc}) — matrice incomplete"
}

echo "sonde ablation reduce : ${FAMILY} n=${N_LIST} blocs=${REPS} (williams 4x4) bras=${ABLATIONS} affinite=${CPUS} (${FILS_MAT} fils materiels / ${COEURS_PHYS} coeurs physiques) threads=${THREADS}"
NRUNS=0
for ((rep = 1; rep <= REPS; rep++)); do
  for n in ${N_LIST}; do
    # La boucle EXECUTE le plan grave, elle ne le recalcule pas.
    while read -r l_bloc l_pos l_bras; do
      [[ "${l_bloc}" == "bloc=${rep}" ]] || continue
      run_one "${l_bras#bras=}" "${n}" "${rep}" "${l_pos#position=}"
      NRUNS=$((NRUNS + 1))
    done < "${WORKD}/plan.txt"
  done
done
# Recomptage des --inject= REELLEMENT emis (lignes commande= des .status) :
# ensemble exact = les trois ablations, une occurrence par tuple non temoin.
NINJ=0
EMIS_BRUT=""
for st in "${WORKD}"/out/*.status; do
  while IFS= read -r ligne; do
    [[ "${ligne}" == commande=* ]] || continue
    for tok in ${ligne#commande=}; do
      [[ "${tok}" == --inject=* ]] || continue
      NINJ=$((NINJ + 1))
      EMIS_BRUT="${EMIS_BRUT}${tok#--inject=}"$'\n'
    done
  done < "${st}"
done
EMIS=""
while IFS= read -r x; do [[ -n "${x}" ]] && EMIS="${EMIS}${EMIS:+ }${x}"; done < <(printf '%s' "${EMIS_BRUT}" | LC_ALL=C "${SORT}" -u)
ATTENDU_INJ=""
while IFS= read -r x; do [[ -n "${x}" ]] && ATTENDU_INJ="${ATTENDU_INJ}${ATTENDU_INJ:+ }${x}"; done < <(printf '%s\n' ${INJECTIONS_SONDE} | LC_ALL=C "${SORT}")
{
  echo "injections_emises=${EMIS}"
  echo "runs_effectues=${NRUNS}"
  echo "runs_attendus=$((4 * NN * REPS))"
  echo "fin_utc=$(date_utc)"
} >> "${WORKD}/META.txt"
[[ "${EMIS}" == "${ATTENDU_INJ}" ]] || invalide "--inject= emis {${EMIS}} != les trois ablations {${ATTENDU_INJ}} (identite de cible non tenue)"
[[ "${NINJ}" -eq $((3 * NN * REPS)) ]] || invalide "${NINJ} occurrence(s) de --inject= dans les .status != $((3 * NN * REPS)) tuples non temoins"
[[ "${NRUNS}" -eq $((4 * NN * REPS)) ]] || invalide "runs effectues ${NRUNS} != attendus $((4 * NN * REPS))"

# --- Empreintes AVANT l'agregateur : ce qu'il lit est ce qui sera scelle ---
declare -A PRE_H=()
PRE_N=0
while IFS= read -r p; do
  [[ -n "${p}" ]] || continue
  h="$(hash_de "${WORKD}/${p}")" || invalide "empreinte avant agregation illisible (${p})"
  PRE_H["${p}"]="${h}"
  PRE_N=$((PRE_N + 1))
done < <(cd "${WORKD}" && lister_fichiers)
[[ "${PRE_N}" -gt 0 && "${PRE_N}" -eq "${#PRE_H[@]}" ]] || invalide "empreintes avant agregation vides ou non injectives (${PRE_N} / ${#PRE_H[@]})"

# --- Agregation FATALE (copie archivee), puis manifeste = dernier fichier -----
# `python3` de PATH, deliberement : son resume n'est jamais publie sans la
# reagregation bit a bit ci-dessous par l'interpreteur grave, hors PATH.
python3 -I "${WORKD}/protocole_agregateur.py" "${WORKD}" > "${WORKD}/resume.txt" 2> "${WORKD}/resume.err" \
  || invalide "agregateur en echec (code $?, voir ${WORKD}/resume.err)"
[[ -s "${WORKD}/resume.txt" ]] || invalide "resume.txt vide"
SPECIAUX="$(cd "${WORKD}" && "${FIND}" . ! -type f ! -type d | compter_lignes)"
[[ "${SPECIAUX}" -eq 0 ]] || invalide "${SPECIAUX} entree(s) non reguliere(s) (lien ou special) dans le reçu"
( cd "${WORKD}" && set -o pipefail && lister_fichiers | "${XARGS}" -d '\n' "${SHA256SUM}" -- > SHA256SUMS ) \
  || invalide "generation de SHA256SUMS en echec"
NFICHIERS="$(cd "${WORKD}" && lister_fichiers | compter_lignes)"
NSOMMES="$(compter_lignes < "${WORKD}/SHA256SUMS")"
[[ "${NSOMMES}" -gt 0 && "${NFICHIERS}" -eq "${NSOMMES}" ]] || invalide "manifeste incomplet (${NSOMMES} entrees pour ${NFICHIERS} fichiers)"
# Inventaire EXACT : les entrees du manifeste (ce qui a ete hache) doivent etre
# exactement l'ensemble attendu — tout fichier apparu entre l'agregateur et le
# manifeste (a la racine, dans out/, un out/SHA256SUMS...) est refuse ici.
inventaire_attendu() {
  local n rep abl ext
  printf '%s\n' HASHES.txt META.txt bin/mhgp7_profile_sonde lscpu.txt plan.txt \
    protocole_agregateur.py protocole_lanceur.sh resume.err resume.txt
  [[ "${DIRTY}" -eq 0 ]] || printf '%s\n' worktree_diff.patch worktree_diff_summary.txt
  for n in ${N_LIST}; do
    for ((rep = 1; rep <= REPS; rep++)); do
      for abl in ${ABLATIONS}; do
        for ext in txt err status; do
          printf 'out/%s_n%s_r%s.%s\n' "${abl}" "${n}" "${rep}" "${ext}"
        done
      done
    done
  done
}
entrees_manifeste() {
  local h p
  while read -r h p; do
    [[ "${h}" =~ ^[0-9a-f]{64}$ && -n "${p}" ]] && printf '%s\n' "${p}"
  done < "${WORKD}/SHA256SUMS" | LC_ALL=C "${SORT}"
}
exiger_listes_egales "inventaire du reçu != ensemble attendu" \
  "$(inventaire_attendu | LC_ALL=C "${SORT}")" "$(entrees_manifeste)"
# Le manifeste scelle EXACTEMENT ce que l'agregateur a lu : chaque empreinte
# d'avant agregation se retrouve a l'identique, et les seules entrees
# nouvelles sont resume.txt et resume.err (ecrits par l'agregateur).
while read -r h p; do
  if [[ -n "${PRE_H[${p}]+x}" ]]; then
    [[ "${PRE_H[${p}]}" == "${h}" ]] || invalide "${p} modifie entre la lecture de l'agregateur et le scellement (${PRE_H[${p}]} avant agregation, ${h} au manifeste) — mutation semantique, jamais publiee"
  else
    case "${p}" in resume.txt|resume.err) ;; *) invalide "${p} apparu apres les empreintes d'avant agregation" ;; esac
  fi
done < "${WORKD}/SHA256SUMS"
[[ "${NSOMMES}" -eq $((PRE_N + 2)) ]] || invalide "manifeste : ${NSOMMES} entrees != ${PRE_N} empreintes d'avant agregation + resume.txt + resume.err"

# --- Derniers controles (scelles puis RELUS), reagregation, publication -----
# controles_avant_publication : tout est RELU apres le manifeste —
# repertoires, entrees speciales, fichiers reguliers == manifeste, protocoles
# archives == META == sources, copie privee == META, outils == META, puis
# `sha256sum -c --strict`. Appele DEUX fois : avant la reagregation (le jeu
# est scelle et verifie) et apres elle (le jeu est integralement re-verifie),
# le `mv` IMMEDIATEMENT apres la seconde.
controles_avant_publication() {
  local hl hls ha has hf ho t
  exiger_listes_egales "repertoires du reçu != {bin out} (repertoire vide ou inattendu)" \
    "$(printf '%s\n' bin out)" \
    "$(cd "${WORKD}" && "${FIND}" . -mindepth 1 -type d -printf '%P\n' | LC_ALL=C "${SORT}")"
  SPECIAUX="$(cd "${WORKD}" && "${FIND}" . ! -type f ! -type d | compter_lignes)"
  [[ "${SPECIAUX}" -eq 0 ]] || invalide "${SPECIAUX} entree(s) non reguliere(s) (lien ou special) apparue(s) avant publication"
  exiger_listes_egales "inventaire final != manifeste (fichier apparu ou disparu apres le manifeste)" \
    "$(entrees_manifeste)" "$(cd "${WORKD}" && lister_fichiers)"
  hl="$(hash_de "${WORKD}/protocole_lanceur.sh")" || invalide "protocole_lanceur.sh archive illisible avant publication"
  [[ "${hl}" == "${H_LANCEUR}" ]] || invalide "protocole_lanceur.sh archive relu avant publication (${hl}) != sha256_lanceur du META (${H_LANCEUR})"
  hls="$(hash_de "${HERE}/sonde_ablation_reduce.sh")" || invalide "lanceur source illisible avant publication"
  [[ "${hls}" == "${H_LANCEUR}" ]] || invalide "lanceur source relu avant publication (${hls}) != copie archivee (${H_LANCEUR}) — protocole modifie pendant la campagne"
  ha="$(hash_de "${WORKD}/protocole_agregateur.py")" || invalide "protocole_agregateur.py archive illisible avant publication"
  [[ "${ha}" == "${H_AGREGATEUR}" ]] || invalide "protocole_agregateur.py archive relu avant publication (${ha}) != sha256_agregateur du META (${H_AGREGATEUR})"
  has="$(hash_de "${HERE}/sonde_ablation_reduce.py")" || invalide "agregateur source illisible avant publication"
  [[ "${has}" == "${H_AGREGATEUR}" ]] || invalide "agregateur source relu avant publication (${has}) != copie archivee (${H_AGREGATEUR}) — protocole modifie pendant la campagne"
  hf="$(hash_de "${BIN}")" || invalide "copie privee illisible avant publication"
  [[ "${hf}" == "${H}" ]] || invalide "copie privee relue avant publication (${hf}) != binaire_sha256 du META (${H})"
  for t in ${OUTILS_REQUIS}; do
    ho="$(hash_de "${OUTIL[${t}]}")" || invalide "outil ${t} (${OUTIL[${t}]}) illisible avant publication"
    [[ "${ho}" == "${OUTIL_H[${t}]}" ]] || invalide "outil ${t} (${OUTIL[${t}]}) relu avant publication (${ho}) != outils= du META (${OUTIL_H[${t}]}) — outil modifie pendant la campagne"
  done
  ( cd "${WORKD}" && "${SHA256SUM}" -c --quiet --strict SHA256SUMS ) >/dev/null 2>&1 \
    || invalide "verification finale sha256sum -c en echec"
}
# Identite BIT A BIT de deux fichiers, a deux temoins : le rc de `cmp` est
# CAPTURE (0 identique, 1 different, autre => « comparaison impossible ») ET
# les deux sha256 (hash_de) doivent coincider — un faux `cmp` muet a 0 est vu
# par les hashes, un faux sha256sum constant par `cmp`. Jamais fail-open.
exiger_fichiers_identiques() { # $1 = libelle, $2 = scelle, $3 = reagrege
  local rc=0 h2 h3 premier=""
  "${CMP}" -s -- "$2" "$3" || rc=$?
  case "${rc}" in
    0) ;;
    1) IFS= read -r premier < <("${CMP}" -- "$2" "$3" 2>&1) || true
       invalide "$1 (premier ecart : ${premier:0:200})" ;;
    *) invalide "$1 : comparaison impossible (cmp rc=${rc}) — jamais fail-open" ;;
  esac
  h2="$(hash_de "$2")" || invalide "$1 : sha256 du fichier scelle illisible"
  h3="$(hash_de "$3")" || invalide "$1 : sha256 de la reagregation illisible"
  [[ "${h2}" == "${h3}" ]] || invalide "$1 (cmp muet a 0 mais sha256 ${h2} != ${h3})"
}
controles_avant_publication

# --- REAGREGATION du jeu scelle (TOCTOU semantique apres agregation) --------
# La copie archivee protocole_agregateur.py (hash == META, relu a l'instant)
# est rejouee sur le jeu SCELLE par l'interpreteur ABSOLU grave au META
# (INTERPRETEUR, hors PATH, `-I` : PYTHONPATH et site utilisateur ignores),
# hors du reçu (dossier temporaire retire quoi qu'il arrive), et son
# stdout/stderr doivent etre BIT A BIT resume.txt / resume.err scelles : un
# resume forge par un faux `python3` en PATH, ou un META mute apres la
# premiere agregation, ne se reproduisent pas.
REAGR="$("${MKTEMP}" -d)" || invalide "reagregation : dossier temporaire impossible (mktemp -d)"
nettoyer_reagr() { "${RM}" -rf -- "${REAGR}"; }
trap nettoyer_reagr EXIT
rc=0
"${INTERPRETEUR}" -I "${WORKD}/protocole_agregateur.py" "${WORKD}" \
  > "${REAGR}/resume.txt" 2> "${REAGR}/resume.err" || rc=$?
if [[ "${rc}" -ne 0 ]]; then
  IFS= read -r -N 300 premier < "${REAGR}/resume.err" || true
  invalide "reagregation du jeu scelle par ${INTERPRETEUR} en echec (code ${rc} : ${premier//$'\n'/ })"
fi
exiger_fichiers_identiques "resume.txt scelle != reagregation du jeu scelle par ${INTERPRETEUR} (copie archivee) — META et resume incoherents ou resume forge, jamais publie" \
  "${WORKD}/resume.txt" "${REAGR}/resume.txt"
exiger_fichiers_identiques "resume.err scelle != reagregation du jeu scelle par ${INTERPRETEUR} (copie archivee) — jamais publie" \
  "${WORKD}/resume.err" "${REAGR}/resume.err"
nettoyer_reagr
# Le jeu est RE-VERIFIE integralement apres la reagregation (rien n'a pu
# apparaitre, disparaitre ni changer), puis publie sans autre operation :
# `mv -T` (jamais un deplacement DANS un dossier apparu entre-temps).
controles_avant_publication
[[ ! -e "${OUT}" ]] || invalide "dossier de publication ${OUT} apparu pendant la campagne — jamais un reçu deplace dans un dossier etranger"
"${MV}" -T -- "${WORKD}" "${OUT}" || invalide "publication (mv) impossible"
echo "reçu publie : ${OUT}"
while IFS= read -r ligne || [[ -n "${ligne}" ]]; do printf '%s\n' "${ligne}"; done < "${OUT}/resume.txt"

