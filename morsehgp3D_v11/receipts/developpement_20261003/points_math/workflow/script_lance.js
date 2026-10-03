export const meta = {
  name: 'choix-hierarchie-points',
  description: 'Choisir la meilleure hierarchie laminaire de points depuis FULL : propositions concurrentes, verification adverse, synthese critique',
  phases: [
    { title: 'Propositions', detail: 'quatre angles : marge H_m sur les cibles, majorite/vote, fermeture, axiomatique' },
    { title: 'Verification', detail: 'un verificateur adverse par proposition, oracle exact' },
    { title: 'Synthese', detail: 'jugement critique et recommandation' },
  ],
}

const OUT = '/workspaces/E-HGP/build/v11-points-math'
const WT = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11'
const V10 = '/workspaces/E-HGP/build/v10-verrou-points'

const CONTEXT = `
CONTEXTE COMMUN (lis-le en entier ; travaille en francais ; sois critique envers la these, l'auditeur, le verdict v10 ET la proposition du developpeur).

Question de l'utilisateur (auteur de la these) : comment passer de la tour FULL a une hierarchie LAMINAIRE sur les points, de facon mathematiquement satisfaisante ; il demande de privilegier l'aspect mathematique, de relire a titre indicatif (sans obligation de respect) les parties I-II de sa these, et d'etre critique vis-a-vis de la these comme de l'auditeur. Il a dit auparavant : « Il n'y a pas d'oracle : fais preuve d'esprit critique » ; « HDBSCAN ne peut pas battre la tour ».

REGLES ABSOLUES : lecture seule sur le depot et le worktree ${WT} (AUCUNE commande git, pas de stash/checkout/add/commit) ; aucune construction ni test natif (C++), aucune commande GCP ; ecris UNIQUEMENT sous ${OUT}/<ton_label>/ (cree-le). Python 3 local autorise (numpy present, pas de matplotlib) ; garde les calculs modestes (oracle exact : n <= 9 points, k <= 4 ; au plus ~15 min de CPU au total). Toute affirmation chiffree vient d'une execution que tu as faite (donne la commande) ou d'un fichier cite.

SOURCES :
- Proposition du developpeur : ${WT}/docs/HIERARCHIE_POINTS.md (cadre des pendaisons fideles F1-F2, regle H_m a marge : e_i = t_i + sup_q [m(p_i,q) - h(q)] sur la region de couverture qualifiee, proprietaire = ancetre de p_i vivant a e_i ; theoremes H1-H5 dont H3 stabilite 3 delta / 5 delta ; lecture critique).
- Contrat mathematique : ${WT}/docs/MATHEMATIQUES.md, section 7 (P1 core, P2 cover, P3 temoins forts, P4 laminarite, P5 stabilite, contre-exemple {0,2,4}).
- Oracle exact (deux routes) : ${WT}/bench/points_reference.py (regles core, cover, first, margin1, margin calculees par force brute sur les coupes fermees de Gamma_k ; fonctions reference_rules(res, n, m), reference_ultrametric(entries, tree), blocks_at(u, level), Tree) et ${WT}/bench/points_hierarchy.py (consommateur sur incidences). Usage : cd ${WT}/bench && python3 -c "import points_reference as pr; from hgp11_ref import Definition; res = Definition(points).order(k); ref, tree = pr.reference_rules(res, len(points), m); u = pr.reference_ultrametric(ref['margin'], tree)". Points = tuples d'entiers 3D distincts. res.nodes (level Fraction, children), res.cuts (level, opened, closed ; closed = tuples (noeud, masque de couverture, masque de coeur)), res.core, res.cover. Fixtures pretes dans ${WT}/bench/points_gate.py (EQUILATERAL, FIVE). Scripts de controle deja joues : accord exact des deux routes sur ~1 800 nuages ; stabilite empirique sur 2 458 040 paires perturbees : margin pire |du|/delta = 1,46 (aucune violation de 3/5), cover 59,3.
- These, texte extrait : ${OUT}/these/ (parties_I_II.txt ; ch6_K_polyedres_amas_discrets.txt : Def. 8 amas discrets, Th. 2 ; ch7_percolation.txt : Theta^poly, Th. 3 fraction recuperable avant fusion parasite ; ch9_partition_stricte.txt : § 9.1 masses S_tau, partition de l'unite S_tau/T_x, vote Prop. 7 ; ch5 : arbre comme espace de resolution, « hacker HDBSCAN »).
- Auditeur v11 : ${WT}/receipts/full_points_20261003/qualified_proof/README.md (fermeture qualifiee des couvertures d'au moins m sites, optimalite minmax, concession de percolation, fixture des cinq points) et ${WT}/audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md (section « FULL -> points »). Sa campagne synthetique (12 scenes) : premiere couverture 0,827-0,852 > HDBSCAN 0,807-0,759 ; fermeture m=3 0,816-0,819 ; core 0,738-0,763.
- Recherche v10 (fin septembre-1er octobre) : ${V10}/juge_final/VERDICT_FINAL.md (regle retenue ER0h(eta=1, kappa=12) : majorite de bande sur le temps de couverture, date a marge ; 125/125 jugements ancres ; continuite conjecturee, « pentes locales jusqu'a 103 » ; point faible : groupes de taille proche de K pres d'un grand amas), ${V10}/juge_final/REPONSES_UTILISATEUR_20261001.md et ${V10}/revision_cible/QUESTIONS_UTILISATEUR.md (reponses de l'utilisateur : Q1 deux triangles avec pont 15 % plus court a K=2 -> ABC|DEF avant fusion, aussi a mcs=2 ; Q2 x avec son voisin a plutot que la paire serree ; Q3 x dans le filament ; Q4 K=3 attendre puis la chaine CmD ; Q-Pi2 : un point de bord ne fait pas exister un cluster ; « On ne doit faire apparaitre les clusters qu'a partir de min_cluster_size »), catalogue des cibles ${V10}/juge_final/cibles/ (fixtures_catalogue_v2.json, cibles_mcs.py, condense.py) et code des regles ${V10}/juge_final/verdict/ (er0h.py, cellules.py, catalogue.py ...). Le verdict v10 conclut : projection INDEPENDANTE de mcs, puis condensation a mcs.
- Faits LiDAR (demos Zoltan) : a K=5 et 10 le plafond FULL lui-meme (meilleur amas discret) reste <= 0,5 sur les 7 objets ou HDBSCAN echoue ; la demo 04 donne un sauvetage a K=3 (premiere couverture 0,57 contre HDBSCAN 0,35). Une campagne G4 de H_m contre sklearn HDBSCAN est en cours ailleurs : ne la touche pas.
`

const PROPOSALS = [
  { key: 'marge_cibles', title: 'Regle a marge H_m face aux cibles de l utilisateur',
    task: `MISSION : eprouver la regle H_m du developpeur (docs/HIERARCHIE_POINTS.md) sur les cibles ancrees de l'utilisateur et sur ses theoremes.
1. Relis H1-H5 et leurs preuves ; signale toute faille (en particulier H3 : cartes d'entrelacement, transport de la qualification ; H5 : « toute fusion est qualifiee a m=k+1 » ; H4 : kappa<1 => entree infinie, bornes (1+2 kappa) delta).
2. Adapte H_m (m = 1 et m = k+1, projection independante de mcs) au catalogue v10 (${V10}/juge_final/cibles/fixtures_catalogue_v2.json, semantique condensee de cibles_mcs.py / condense.py) : calcule les dates et proprietaires par l'oracle de ${WT}/bench/points_reference.py (fixtures de petite taille seulement ; saute et COMPTE celles trop grosses pour l'oracle), condense a mcs comme le juge v10, et compte les jugements ancres reussis. Reproduis d'abord le score publie d'au moins une regle v10 (cover ou ER0h) avec leur propre code pour valider ton adaptateur.
3. Pour chaque echec de H_m : fixture, cible, sortie, et si une modification MINIMALE compatible avec la stabilite H3 le corrige (sans parametre ad hoc).
Rends : taux de reussite par regle (avec l'echantillon effectivement juge), liste des echecs, failles eventuelles des preuves, verdict argumente.` },
  { key: 'majorite_vote', title: 'Majorites et vote (these 9.1, ER0h v10)',
    task: `MISSION : construire et juger la meilleure version LAMINAIRE des regles a masses : (a) le vote de la these (§ 9.1 : S_tau, partition de l'unite S_tau/T_x, argmax apres selection, Prop. 7) rendu hierarchique ; (b) la regle ER0h retenue par le verdict v10 (${V10}/juge_final/VERDICT_FINAL.md et verdict/er0h.py).
1. Enonce precisement chaque regle dans le cadre des pendaisons fideles (section 2 de docs/HIERARCHIE_POINTS.md) ou dis pourquoi elle n'y entre pas.
2. Laminarite, fidelite (aucune reunion avant FULL), equivariance, dependance a des choix d'algorithme (faces de Gabriel F_K, exposant p, eta, kappa).
3. Stabilite : essaie de PROUVER une borne uniforme a la H3 pour ER0h, ou exhibe une famille exacte ou le rapport |delta u|/delta est non borne (le verdict v10 mesure des pentes locales jusqu'a 103 : est-ce borne ?). Fais-le au moins sur une famille parametree avec l'oracle ou un calcul exact.
4. Compare a H_m sur les memes fixtures exactes (deux triangles, cinq points, {0,2,4} perturbe, Q2-Q4).
Rends : enonces, proprietes prouvees/refutees avec preuves ou contre-exemples exacts, et ce qu'une regle a masses apporterait que H_m n'a pas (et inversement).` },
  { key: 'fermeture', title: 'Fermeture qualifiee de l auditeur et approximations exterieures',
    task: `MISSION : juger equitablement la fermeture qualifiee de l'auditeur (qualified_proof/README.md).
1. Verifie ses garanties (laminarite, equivariance, stabilite 1 epsilon, optimalite minmax, identite (k, m=k+1) = (k+1, m=k+1)).
2. Teste la critique du developpeur : dans le cadre du chapitre 7 de la these (fraction recuperable avant fusion parasite), la fermeture cree-t-elle des fusions parasites AVANT celles de FULL ? Construis des familles exactes (deux amas separes par une vallee, ou deux objets au contact) et mesure avec l'oracle le niveau ou la fermeture reunit deux blocs que FULL separe encore, sa frequence sur des nuages aleatoires, et l'ecart de niveau. Distingue « reunion d'un site partage » et « reunion de deux amas entiers ».
3. Existe-t-il une variante exterieure fidele (par exemple la fermeture restreinte a l'interieur de chaque composante FULL) et comment se compare-t-elle a H_m ?
Rends : verifications, mesures exactes, verdict argumente sur la place de l'approximation exterieure.` },
  { key: 'axiomes', title: 'Axiomatique, impossibilites et caracterisation',
    task: `MISSION : poser un systeme d'axiomes pour une hierarchie laminaire de points issue de FULL_k a k fixe et en tirer ce qui est force.
Axiomes candidats : A1 laminarite ; A2 fidelite (deux sites ne sont reunis qu'au plus tot a la fusion FULL de leurs proprietaires, blocs inclus dans des amas discrets) ; A3 stabilite lipschitzienne sous perturbation appariee ; A4 equivariance (renumerotation, isometries) ; A5 entree immediate des sites non ambigus ; A6 k=1 : liaison simple ; A7 independance a mcs (mcs seulement a la condensation) ; A8 cibles de l'utilisateur (deux triangles a K=2 meme a mcs=2, Q2-Q4, Q-Pi2).
1. Montre quelles combinaisons sont incompatibles (generalise le trilemme F2) et lesquelles sont satisfaites par core, cover, H_1, H_{k+1}, ER0h, fermeture qualifiee.
2. Caracterisation : sous A1-A5 (et une normalisation de la constante de Lipschitz), H_1 est-elle la regle la plus precoce (borne inferieure e_i >= t_i + D_i pour toute regle continue fidele) ? Prouve-le, ou exhibe une regle fidele stable qui fait entrer un site plus tot.
3. Le seuil m=k+1 (« au moins deux faces ») est-il force par A8 sous A2-A3, ou un autre critere sans mcs suffit-il ?
4. H_m peut-elle faire entrer un site APRES son temps de coeur D_k(x) ? Est-ce un defaut au regard du chapitre 7 (fraction recuperable) ?
Rends : enonces exacts, preuves, contre-exemples verifies a l'oracle, et ce qui reste ouvert.` },
]

const PROPOSAL_SCHEMA = {
  type: 'object',
  properties: {
    method: { type: 'string' },
    summary: { type: 'string' },
    claims: { type: 'array', items: { type: 'object', properties: {
      statement: { type: 'string' },
      status: { type: 'string', enum: ['prouve', 'teste', 'conjecture', 'refute'] },
      evidence: { type: 'string' } }, required: ['statement', 'status', 'evidence'] } },
    counterexamples: { type: 'array', items: { type: 'object', properties: {
      description: { type: 'string' }, points: { type: 'string' }, finding: { type: 'string' } },
      required: ['description', 'finding'] } },
    recommendation: { type: 'string' },
    report_path: { type: 'string' },
  },
  required: ['method', 'summary', 'claims', 'recommendation', 'report_path'],
}

const VERDICT_SCHEMA = {
  type: 'object',
  properties: {
    verdicts: { type: 'array', items: { type: 'object', properties: {
      claim: { type: 'string' },
      verdict: { type: 'string', enum: ['confirme', 'refute', 'non_verifie'] },
      evidence: { type: 'string' } }, required: ['claim', 'verdict', 'evidence'] } },
    overall: { type: 'string' },
    report_path: { type: 'string' },
  },
  required: ['verdicts', 'overall', 'report_path'],
}

const results = await pipeline(
  PROPOSALS,
  p => agent(`${CONTEXT}\n\nTON LABEL : ${p.key} (ecris sous ${OUT}/${p.key}/, rapport principal ${OUT}/${p.key}/RAPPORT.md).\n${p.task}\n\nTermine par la sortie structuree demandee (claims avec statut honnete : « prouve » seulement avec preuve complete ecrite dans le rapport ; « teste » avec commande et resultat).`,
    { label: 'proposition:' + p.key, phase: 'Propositions', schema: PROPOSAL_SCHEMA }),
  (prop, p) => prop === null ? null : agent(`${CONTEXT}\n\nTON LABEL : verif_${p.key} (ecris sous ${OUT}/verif_${p.key}/).\nTu es un VERIFICATEUR ADVERSE. Voici le resultat d'une proposition (« ${p.title} ») ; son rapport complet est ${prop.report_path}. Lis-le, puis essaie de REFUTER ses 4 a 8 affirmations les plus importantes : relis chaque preuve ligne a ligne, rejoue ou recode independamment chaque calcul (sans reutiliser son code quand c'est possible), cherche des contre-exemples exacts a l'oracle. Par defaut, « non_verifie » si tu ne peux pas trancher. Sois aussi severe avec ce qui favorise la proposition du developpeur qu'avec le reste.\n\nRESULTAT A VERIFIER (JSON) :\n${JSON.stringify(prop)}`,
    { label: 'verification:' + p.key, phase: 'Verification', schema: VERDICT_SCHEMA })
    .then(v => ({ key: p.key, title: p.title, proposal: prop, verification: v })),
)

const done = results.filter(Boolean)
log(`${done.length}/${PROPOSALS.length} propositions verifiees`)

phase('Synthese')
const synthese = await agent(`${CONTEXT}\n\nTON LABEL : synthese (ecris ${OUT}/SYNTHESE.md).\nTu es le JUGE FINAL. Voici quatre propositions et leurs verifications adverses (rapports complets aux chemins indiques). Decide, en privilegiant l'aspect mathematique, quelle hierarchie laminaire de points l'on doit retenir a k fixe (regle, parametres, ce qui est prouve, ce qui reste conjecture), et comment la sortie plate (condensation, selection, eventuelle completion par le vote de la these) s'y rattache. Sois explicitement critique envers : la these (§ 6, 7, 9.1), l'auditeur (fermeture qualifiee), le verdict v10 (ER0h) et la proposition du developpeur (H_m). Ne retiens que les affirmations confirmees ou explicitement marquees comme conjectures. Donne : (1) la recommandation en une phrase ; (2) un tableau comparatif des regles par propriete (laminaire, fidele, stabilite prouvee et constante, equivariance, cibles de l'utilisateur, independance a mcs, parametres libres) ; (3) les preuves ou references a garder ; (4) les points ouverts et les experiences decisives a mener (y compris sur les trames LiDAR de Zoltan/ ou HDBSCAN echoue) ; (5) les corrections a apporter a docs/HIERARCHIE_POINTS.md. Rends le texte Markdown complet de la synthese (le meme que SYNTHESE.md).\n\nPROPOSITIONS ET VERIFICATIONS (JSON) :\n${JSON.stringify(done)}`,
  { label: 'synthese', phase: 'Synthese' })

return { synthese, propositions: done.map(d => ({ key: d.key, method: d.proposal && d.proposal.method, report: d.proposal && d.proposal.report_path, verification: d.verification && d.verification.overall })) }
