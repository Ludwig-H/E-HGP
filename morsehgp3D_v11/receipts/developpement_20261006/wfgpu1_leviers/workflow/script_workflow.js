export const meta = {
  name: 'v11-gpu-leviers',
  description: 'Quatre leviers GPU de la feuille v11 explores en parallele (worktrees /tmp, portes locales, SASS), puis contre-lecture adverse de chacun',
  phases: [
    { title: 'Implementer', detail: 'un agent par levier, dans son worktree /tmp/v11-wf-X, sans commit' },
    { title: 'Contre-lire', detail: 'un verificateur adverse par levier, des que son implementation est rendue' },
  ],
}

const BASE = '905fad2e1 (origin/main)'
const COMMON = `
Contexte (6 octobre 2026). Chantier morsehgp3D_v11 (lire CLAUDE.md : regles absolues, francais, aucune branche, aucun
commit, aucun push, aucune commande GCP). Cadre : exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / public_status=not_claimed.

Objet : la voie GPU du lot de feuilles du catalogue (tour HGP FULL). Fichiers :
- src/catalogue/leaf_device.hpp : la feuille, SOURCE UNIQUE hote/appareil (Leaf<Sink>, prepare, live_rows, extend<Depth>
  boucle en ligne, lines_possible (J2, memoire des faces), q2/q3/q4, census_and_emit, canonical). Un chemin non
  certifie rend kUnresolved (la feuille est refaite par leaf.cpp sur CPU).
- src/catalogue/leaf_device_predicates.hpp : predicats exacts (i64/i128, bornes par profil MHGP11_COORD_BITS 18/21/24).
- src/catalogue/leaf_batch.hpp : puits (ScratchSink du comptage avec reservoir chaine de blocs, FillSink de la seconde
  passe, CountSink), copy_scratch, spare_chunks.
- src/catalogue/leaf_batch.cpp : executeur hote (Pool). src/catalogue/leaf_batch_cuda.cu : executeur CUDA (count_kernel un fil
  par feuille, feuilles triees par m decroissant ; prefixes CUB ; copy_kernel ; fill_kernel des seules feuilles qui
  debordent du reservoir).
- src/catalogue/single_pass_batch.cpp : integration (gather des files, executeur, Materialize = Level par Sphere::through,
  repli des non resolues). Les feuilles sont mises en file par la passe unique (src/catalogue/single_pass.cpp et voisins).
- bench/full_probe.cpp : sonde. Usage : mhgp11_full_bench XYZ IDS DUMP K FEUILLE 256 0 4294967295 18446744073709551615
  FILS MASQUE. Masques : 16379 CPU (feuilles intercalees dans le parcours), 49147 lot sur le Pool de l'hote, 81915 lot
  sur GPU (CUDA), +131072 lot sans reservoir (debordements rejoues). Lignes JSON par phase ; "domain" porte
  catalogue_work (registre) et leaf_batch (count_ns, fill_ns, fill_jobs, copied_jobs, spare_record_chunks...).
- Donnees : MHGP11_DATA_DIR=/workspaces/E-HGP/build/v11-full-data-20261002 (lidar_ng00/01/02 .u32le et .ids.u32le,
  JAMAIS copiees dans le depot). Dumps de reference ng00 : K5 feuilles 16 sha256 3a2bfb4f9f48..., K10 feuilles 24
  61a4245b91d9... (identiques quelle que soit la voie et la taille de feuille ; le registre catalogue_work depend de la
  taille de feuille mais doit etre identique entre voies a taille egale).
- Porte principale : tests/tower/full_leaf_lanes.py (LINE gravee dans tests/tower/tests.cmake), plus les portes
  catalogue rapides : ctest -R "full_leaf_lanes|catalogue_(cache|single_pass|leaf|small_pair|parallel)".
- Mutants : tests/mutants/<module>.json (cles id, fichier, cherche, remplace, porte, note ; "cherche" unique dans le
  fichier) ; python3 tests/mutants/run_mutants.py --manifest tests/mutants/tower.json --source . --check, puis
  --work DIR --only id1,id2 --floor N --jobs 2 --build-jobs 2 --cmake-arg=-DCMAKE_BUILD_TYPE=Release
  --cmake-arg=-DMHGP11_COORD_BITS=21 (MHGP11_DATA_DIR exporte). Tout nouveau chemin de code exige un mutant tue.
- Style : python3 tools/check_style.py (lignes <= 120, fonctions <= 100 lignes, fichiers <= 500 lignes, sources ASCII,
  commentaires en francais). Portes Python : bibliotheque standard, aucune assertion (tenir sous python3 -S et -O).

Mesures G4 (W48, medianes a chaud, ms) a la base ${BASE} : ng00 K5 feuilles 16 : comptage GPU 39, ecriture 0,3,
executeur 45, domain GPU 218 contre CPU 210 ; K10 feuilles 24 : comptage 226, executeur 241, domain GPU 618 contre CPU
835. Version j2memo (34a8a561d, sans reservoir) : comptage 30 (K5) et 172 (K10). Feuilles de 24 a K5 en GPU : parcours
35, executeur 87 (comptage 84), domain 192 < CPU 210. Profil Nsight du comptage un fil (K10/24, ng00, avant la memoire
J2) : J2 ~38 % (center_line_meets 23 %), recensement (census_and_emit + side) ~15 %, controle du parcours ~14 %, q4
~10 %, ecriture des cases ~12 %. 3,1 fils actifs par warp (divergence). Distribution du travail par feuille (ng00, K5) :
la plus lourde ~6000 unites (prefixes + tests de recensement) pour une moyenne ~450 ; top 1 % des feuilles = 5,4 % du
travail ; m predit mal la lourdeur ; une seule feuille lourde coute ~13 ms sur un fil GPU (latence, queue du noyau).
Lecon de reconvergence : un appel non integre (CALL) ou une restructuration de boucle dans le code chaud de la feuille
fait monter les BSSY et ralentit le GPU (BSSY 85 -> 107 : seconde passe x2 ; chemin froid avec CALL : comptage +30 %).

Outils locaux (aucun GPU sur ce codespace ; nvcc 12.9 installe) :
- /tmp/v11-wf-cuda-obj.sh ARBRE_MORSEHGP3D_V11 OBJET.o : compile leaf_batch_cuda.cu (Release, u21, sm_120), imprime
  registres et pile ptxas ; puis python3 /tmp/v11-wf-sass.py OBJET.o : instructions, BSSY, CALL, LDL/STL, LD/ST des
  noyaux count et fill. Base ${BASE} : count 11832 instr, 134 BSSY, 3 CALL, 164 registres ; fill 9920 instr, 93 BSSY.
- Sonde de reference construite a la base : /tmp/v11-wf-base-full_bench (Release u21, hote seulement).
- Construis ta variante dans /tmp/v11-wf-<X>-build (cmake -S <worktree>/morsehgp3D_v11 -B /tmp/v11-wf-<X>-build
  -DCMAKE_BUILD_TYPE=Release -DMHGP11_COORD_BITS=21 ; cmake --build ... -j2 --target <cibles>). Le codespace a 8 coeurs,
  partages par quatre agents : -j2 au plus, pas de construction complete inutile.
- Le disque /workspaces est presque plein : ne rien ecrire de gros sous /workspaces ; tout travail dans /tmp.

Exactitude (non negociable) : aucune decision du catalogue ne change ; dumps identiques au CPU (16379) pour K5/16 et
K10/24 sur ng00 au minimum ; registre catalogue_work identique entre voies a taille de feuille egale ; sorties
deterministes quel que soit le nombre de fils ; toute allocation nouvelle admise au MemoryBudget avant allocation ;
refus explicite plutot que comportement indefini.

Livrables (obligatoires) : dans ton worktree, les modifications non commitees ; puis
git -C <worktree> diff > /workspaces/E-HGP/build/v11-persist/wf_gpu/<X>.patch (fichiers nouveaux inclus via git add -N
avant le diff) et un rapport francais /workspaces/E-HGP/build/v11-persist/wf_gpu/<X>_RAPPORT.md (conception, preuve
d'exactitude, portes jouees et resultats exacts, mutants et verdicts, SASS avant/apres, mesures hote appariees, effet
attendu sur G4 ecrit comme ESTIMATION, risques). Ta reponse finale est le JSON demande.`

const SCHEMA = {
  type: 'object',
  properties: {
    lever: { type: 'string' },
    status: { type: 'string', enum: ['pret_pour_g4', 'abandonne', 'partiel'] },
    patch: { type: 'string' },
    report: { type: 'string' },
    gates: { type: 'string', description: 'portes jouees et resultats exacts' },
    mutants: { type: 'string' },
    sass: { type: 'string', description: 'count/fill instr, BSSY, CALL, registres, avant et apres' },
    host_measure: { type: 'string', description: 'mesures hote appariees (W1 ou W8), clairement descriptives' },
    probe_masks: { type: 'string', description: 'masques ou parametres de sonde pour comparer sur G4' },
    expected_g4: { type: 'string', description: 'estimation ecrite comme telle' },
    risks: { type: 'string' },
  },
  required: ['lever', 'status', 'patch', 'report', 'gates', 'mutants', 'sass', 'host_measure', 'probe_masks', 'expected_g4', 'risks'],
}

const VERDICT = {
  type: 'object',
  properties: {
    lever: { type: 'string' },
    verdict: { type: 'string', enum: ['favorable', 'favorable_sous_reserves', 'defavorable'] },
    defects: { type: 'array', items: { type: 'string' } },
    checks_run: { type: 'string' },
    notes: { type: 'string' },
  },
  required: ['lever', 'verdict', 'defects', 'checks_run', 'notes'],
}

const LEVERS = [
  { key: 'A', title: 'reservoir sans cout d appel', goal: `Levier A : le reservoir chaine (src/catalogue/leaf_batch.hpp, ScratchSink::cold hors ligne) supprime la seconde passe
mais l'appel au chemin froid, present dans les trois instanciations du recensement, alourdit le comptage GPU (SASS count
11832 instr / 134 BSSY / 3 CALL contre 10352 / 107 / 0 pour j2memo ; G4 : comptage +28 a +32 %). Objectif : retrouver un
SASS du noyau de comptage aussi proche que possible de j2memo (0 CALL, BSSY ~107) TOUT EN gardant ecriture ~0 a K10
(aucune feuille rejouee tant que le reservoir suffit, le rejeu restant le secours, et la voie 131072 sans reservoir
intacte). Pistes a evaluer et a departager par le SASS et par l'hote : (i) aucun appel : le puits ne fait que noter le
depassement (drapeau) et un etat minimal, et un noyau/une passe dediee re-emet seulement les feuilles debordees dans
le reservoir (comparer au rejeu actuel) ; (ii) capacite de case variable par feuille fixee avant le noyau (prefixe des
tailles, par classe de m ou autre predicteur cheap), sans chaine, avec rejeu residuel ; (iii) chemin froid integre
mais minimal ; (iv) toute autre idee. Choisis par mesure, explique pourquoi.` },
  { key: 'B', title: 'feuilles lourdes au CPU', goal: `Levier B : hybride CPU/GPU. Le GPU paie la latence d'une feuille lourde sur un fil (queue du noyau : ~13 ms pour une
seule feuille a K5) alors que le CPU la traite vite, intercalee dans le parcours (voie 16379). Objectif : en voie GPU
(81915), garder sur le CPU (traitees immediatement pendant le parcours, comme la voie 16379) les feuilles predites
lourdes, et n'envoyer au lot GPU que les autres. Trouve ou les feuilles sont mises en file (passe unique) et comment la
voie CPU les traite. Il faut un predicteur calculable a la mise en file, peu couteux (m, boite, sites ; ou une petite
estimation exacte de travail, par exemple le nombre de paires vivantes apres dominance si elle est bon marche), et un
parametre de sonde (nouveau bit de masque : 262144 ; relever la borne de parse dans bench/full_probe.cpp ; ou un seuil)
pour comparer sur G4. Mesure locale : avec la sonde tracee (variable d'environnement de trace temporaire, hors depot
dans le livrable), part des feuilles et du travail routes au CPU selon le seuil, et queue GPU estimee (travail de la
feuille la plus lourde restante). Sorties identiques (le catalogue final est trie canoniquement). Porte : etendre
tests/tower/full_leaf_lanes.py avec la nouvelle voie (dump et registre identiques).` },
  { key: 'C', title: 'cout par unite du comptage', goal: `Levier C : reduire le cout par unite de travail de la feuille device sans changer une decision : center_line_meets
(J2, 23 % du comptage), census side(), predicats q3/q4 (leaf_device_predicates.hpp, leaf_device.hpp). Pistes : filtres
exacts moins chers avant l'arithmetique i128 (avec preuve de borne par profil : B <= 24 bits, ecrire la borne en
commentaire comme dans le fichier), calculs redondants evites (valeurs recalculees par face ou par site), types plus
etroits la ou la borne le permet, ordre des tests. Chaque changement : preuve en commentaire, meme decision (porte
full_leaf_lanes et portes catalogue ; si utile, une petite porte unitaire qui compare l'ancien et le nouveau predicat
sur des cas limites, dont les bornes du cube 2^B), un mutant tue. Ne pas faire monter les BSSY du noyau de comptage
(verifier au SASS). Mesure hote appariee : voie lot hote 49147 a W1 sur ng00 K5/16 (count_ns), 3 paires base/variante
alternees, avec la sonde de base /tmp/v11-wf-base-full_bench ; le CPU execute le meme code de feuille.` },
  { key: 'D', title: 'queue des feuilles lourdes', goal: `Levier D : couper la queue du noyau de comptage due aux feuilles lourdes, cote GPU. Pistes : (i) ordre des feuilles par
travail estime decroissant (aujourd'hui par m decroissant) pour demarrer les plus lourdes d'abord ; (ii) decouper les
feuilles les plus lourdes en taches de sous-arbres de paires (profondeur 0 jouee une fois, puis chaque paire (i, j) est
une tache independante ; emissions placees par prefixe sur les paires dans l'ordre du parcours) traitees par des fils
DIFFERENTS (pas un warp par feuille : la feuille cooperative par paires a deja ete rejetee pour divergence ; ici seules
les feuilles lourdes sont decoupees, le reste garde un fil par feuille). Le code de cette decomposition existe dans
l'historique : git show c3df81805:morsehgp3D_v11/src/catalogue/leaf_device.hpp (depth0_pairs, run_pair, run_leaf_coop,
emulation hote) et ee3eabe5e (leaf_device_coop.hpp) ; il a ete retire par d4228f5e5 ; ses preuves d'egalite des
compteurs (cache J2 compris) sont dans morsehgp3D_v11/audits (sections O, Q) et
morsehgp3D_v11/receipts/audit_leaf_cooperative_20261006. Garde la boucle extend en ligne pour la voie un fil (lecon de
reconvergence). Validation : emulation hote + portes, SASS ; un parametre de sonde pour comparer sur G4.` },
]

phase('Implementer')
const results = await pipeline(
  LEVERS,
  lever => agent(`${COMMON}

Ton worktree : /tmp/v11-wf-${lever.key} (deja a la base ${BASE}, checkout partiel morsehgp3D_v11/, tools/, docs/).
Travaille UNIQUEMENT dans ce worktree et dans /tmp/v11-wf-${lever.key}-build ; <X> = ${lever.key}.

${lever.goal}

Si le levier se revele mauvais (mesure ou SASS), dis-le franchement (status abandonne) avec les chiffres ; un levier
abandonne bien documente vaut mieux qu'un faux gain.`, { label: `impl:${lever.key}`, phase: 'Implementer', schema: SCHEMA, effort: 'high' }),
  (impl, lever) => impl === null ? null : agent(`${COMMON}

Tu es un VERIFICATEUR ADVERSE. Un implementeur a produit le levier ${lever.key} (${lever.title}) dans le worktree
/tmp/v11-wf-${lever.key} ; patch /workspaces/E-HGP/build/v11-persist/wf_gpu/${lever.key}.patch ; rapport
/workspaces/E-HGP/build/v11-persist/wf_gpu/${lever.key}_RAPPORT.md. Son resume : ${JSON.stringify(impl).slice(0, 4000)}

Cherche a le REFUTER : decision du catalogue changee, compteur du registre different, non-determinisme (ordre des fils,
atomiques), course de donnees sur l'appareil, allocation non admise au budget, debordement d'entier, chemin non
couvert par une porte, mutant equivalent ou non tue, regression de reconvergence (BSSY/CALL), style, mesure mal
appariee ou surinterpretee. Rejoue toi-meme au moins : la construction Release dans /tmp/v11-wf-${lever.key}-verif
(-j2), tests/tower/full_leaf_lanes.py et les portes catalogue rapides, check_style, le SASS
(/tmp/v11-wf-cuda-obj.sh + /tmp/v11-wf-sass.py), et un mutant de ton choix sur le nouveau code. Ne modifie pas le
worktree de l'implementeur (copie-le si tu dois experimenter). Verdict franc ; chaque defaut avec fichier:ligne et
scenario concret.`, { label: `verif:${lever.key}`, phase: 'Contre-lire', schema: VERDICT, effort: 'high' })
    .then(v => ({ impl, verdict: v })),
)
return results
