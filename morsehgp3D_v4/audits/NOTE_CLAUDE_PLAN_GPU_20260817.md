# Note de Claude — plan d'écriture GPU des deux noyaux réguliers (directive utilisateur)

Date : 17 août 2026. Directive : « il faut tout optimiser. Puis
paralléliser et écrire pour GPU. Les petits tests sont faisables
localement. » La parallélisation CPU est livrée (génération ×3,6 sur 4
vCPU, aval ×2,4–3,3, sorties au bit près) ; ce document est le plan du
port GPU. Contrainte matérielle : le conteneur local n'a ni GPU ni
`nvcc` — les `.cu` seront compilés et validés SUR LA VM G4 par le
protocole gardé (aucun benchmark local possible, aucune écriture à
l'aveugle : le plan précède le code).

## Les deux noyaux candidats (mesurés, stabilisés, sans allocation)

1. **Cœur de seed aplati** (le tueur de 90 % des seeds) : par seed
   $(a,b,x)$ d'une ancre, scanner les sites du cover — $P(z)$ (i128),
   $B(z)$ (i64), témoin ssi $P<0$ et $2P^2 > J\,B^2$ (U320) — sortie
   anticipée à $h_4 \leq 8$. Travail régulier, données partagées par
   ancre, aucune divergence de flot autre que la sortie anticipée.
2. **Primitive de sweep à deux côtés** : seuils bornés ($k \leq 8$ par
   côté), classification en fenêtre, $\leq 16$ groupes en TABLEAUX
   FIXES, préfixes/suffixes, $d_j$, verdict — déjà extraite, testable
   seule, zéro allocation : c'est littéralement la forme kernel.

## Arithmétique exacte en device

- i128 : `__int128` est supporté par nvcc en code device (pas
  d'intrinsèque 128 bits natif — le compilateur émet des limbes 64
  bits ; correct, c'est la seule exigence).
- U192/U320 (`mul_level_192`, `mul_192_128_to_320`, `cmp_u192/u320`,
  `cmp_2p2_jb2`, `cmp_mu_same_side`) : déjà écrits en limbes u64
  portables — annotation `__host__ __device__` et AUCUN changement de
  logique. Le selftest arithmétique tournera en device (mêmes témoins)
  avant tout usage.

## Découpe et mémoire

- Unité de travail : UN SEED par thread (cœur) ; blocs = seeds d'une
  même ancre, cover de l'ancre en mémoire partagée (SoA : positions
  u16×3 compactées, 8 octets/site — un cover de 512 sites tient
  largement en shared).
- Pipeline par ancre : (i) kernel cœur → bitmap des seeds survivants ;
  (ii) compaction (préfixe-somme) ; (iii) kernel A,B + sweep par seed
  survivant → émissions bornées (chaque seed émet $\leq 16$ candidats :
  écriture à offset fixe seed×16 + compaction finale) ; (iv) le RLE,
  le census et l'aval RESTENT CPU dans un premier temps (le tri
  canonise l'ordre : le flux GPU→CPU est un multiensemble, exactement
  comme les ouvriers CPU actuels).
- Lots d'ancres : regrouper les ancres par taille de cover pour des
  blocs homogènes ; les rectangles WSPD restent construits sur CPU.

## Exactitude et validation (sur G4 uniquement)

- La porte de référence est CELLE QUI EXISTE : égalité au bit près
  post-RLE (clés, arités, représentations) contre le chemin CPU
  séquentiel, plus les compteurs sommes — la porte `--par-gate` étendue
  d'un mode `--gpu` ; mutants : `gpu-drop-block` (un bloc d'ancres
  oublié à la fusion) et le selftest arithmétique device.
- Convention dépôt : option CMake `MHGP4_ENABLE_CUDA` OFF par défaut,
  sm_120, et — comme la convention produit — build CUDA exigé depuis un
  worktree git PROPRE.
- Les mesures G4 passeront par le protocole gardé existant (session
  scripts, statuts transactionnels, pin du protocole) ; la VM est
  éteinte et certifiée `TERMINATED` après chaque session — aucune VM
  n'est allumée à ce jour.

## Ce qui NE part PAS sur GPU (aujourd'hui)

Le fold (réduction séquentielle par lots — voir
`QUESTION_CLAUDE_INTERNES_DU_FOLD`), le census par clé (candidat de
seconde vague : descente d'arbre par boule, régulier lui aussi), le
juge et tous les oracles : le juge reste CPU et indépendant, c'est lui
qui juge le GPU.

Prochaine étape concrète : écrire les annotations
`__host__ __device__` des primitives arithmétiques et le squelette des
deux kernels dans `morsehgp3D_v4/src/gpu/` (compile-gated), puis
compiler/valider lors de la première session G4 disponible — la
campagne d'échelle CPU n'en dépend pas et peut partir avant.
