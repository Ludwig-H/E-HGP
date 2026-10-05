# Audits courants de la v11

5 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Notes maintenues en place ;
preuves détaillées dans les reçus immuables.

**Audit général repris au pin 238734f1d.** Aucun nouveau défaut
mathématique FULL établi ; contre-épreuve du nerf complet, des coupes et
des verticales conforme sur 65 ordres. Le point important du raccord est
la qualification de l'assemblage L1 entier. S5 doit en outre empêcher la
publication d'un produit avec une Session étrangère, qui fausse son
contrat mémoire. Pour juger la voie K seule, mesurer le journal actif et
la sortie réellement livrée. [Verdict et priorités du développeur](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#audit-général-du-5-octobre--décisions-importantes),
[preuve et périmètre](../receipts/audit_geant_20261005/README.md).

**Suivi du raccord L1.** Le développeur a repris les demandes importantes
en **4f1e0fb3a**. Les portes de la coquille de 24 sites et de K10/K12 sont intégrées au brouillon S6a ;
les nouvelles primitives Python et leur refus budgété sont rejugés conformes.
L'assemblage et sa qualification G4 restent à faire. Une fixture précise
complète la correction S5 : déplacer la Session avec un produit vivant,
puis vérifier le refus d'une Session étrangère avant toute écriture.
La frontière K=n est tranchée pour la future L3 : refus explicite de
`points`/`plat` pour K≥2, FULL/supports conservés.
[Contrelecture et preuve bornée](../receipts/audit_l1_followup_20261005/README.md).

- [Mathématiques : supports, frontières, hiérarchies et sélection](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Question courante du développeur sur 100 ms](QUESTION_CLAUDE_VITESSE_100MS_20261004.md).
- [Réponse courante du développeur sur les supports](REPONSE_CLAUDE_SUPPORTS_20261004.md) : demandes de l'audit général adoptées, frontière K=n tranchée pour L3.
- [Contrat courant des points](../docs/HIERARCHIE_POINTS.md), avec
  [échange du 3 octobre archivé](../receipts/audit_supports_contract_20261005/notes_before/README.md).
- [Audit indépendant, maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

**Chantier courant : supports, arbre K seul.** S0/S1 **5adf6a59f** et
la demande **9290cf3bf** sont relus favorablement : lemmes P/W, deux
lectures E5, D2 et 13 mutants causaux. S2/257aabb92 et S4/f98aeed67 sont
livrés. Les WIP S3/S6 ont maintenant leurs gardes p+q/traces ; D2/E5 sont
présents en S3. Raccord final du journal relu favorablement ; S5 intègre
signature V2, état publié et SIGXFSZ. Qualification native G4 encore attendue.

**Réponse utile au développeur.** La sphère entière de rayon carré5,
24 sites, mêle 12 q2, 24 q3, 792 q4 ; N4=3906 diffère des 4068 incidences
par support. Elle fournit la porte au plafond, avec 25/30 sites de rayon
carré9 pour le refus entier. L'oracle WIP borne désormais
`_minimal_nonseparable`, en plus du calcul N_j par petites combinaisons.
Deux portes exactes complètent les K élevés : 12 sites à K10 et la
coquille de 24 sites à K12, **116 traces strictes**, sans construire FULL12.
Corriger le hook variadique des deux fautes IO avant G4 ; ce constat
concerne le harnais, sans défaut produit observé. Comparer les masques
FULL 16379 et ordre seul 7035, avec mêmes options applicables. Matrice G4 séparée
par livraison/profil, coût des portes fixé avant session.
[Relecture et 19 713 gardes portables nouvelles](../receipts/audit_native_integration_20261005/README.md).
[Preuve précédente de la coquille mixte](../receipts/audit_supports_contract_20261005/README.md).
Les preuves antérieures restent immuables, liées dans les deux notes.

**Tour FULL et temps.** La référence qualifiée K5/u21/W48 donne
**412 /352 /381 ms** sur trois trames sans sol de la séquence08.
Le banc F qualifie export FULL natif + projection Python exacte.
Sorties paramétrées points/plat natives, **100 ms, GPU et massif restent
ouverts**. Les nouveaux diagnostics CPU, comparaisons v10 et cohortes
réelles restent dans les deux notes, avec leurs limites de preuve.

**Suite GPU.** Compression22 mesurée dans GPU6 ; K5/W48 chaud reste plus
lent sur GPU, la baisse K10/leaf24 est de 2–3,5 %. Cela ne qualifie pas
100 ms ni les derniers ajouts. `copied_jobs` et pics physiques du pool sont
livrés57dd ; nouveau rejeu natif/GPU et portes numériques extrêmes attendus.
[Reçu GPU6 relu](../receipts/audit_gpu6_receipt_20261004/README.md),
[transport compact et budget](../receipts/audit_gpu_scratch_20261004/README.md).

**Fondations et pistes retenues.** Audit initial : sept modules/101 fichiers de source relus,
mathématiques, parallélisation, mémoires et protocole G4 compris.
[Audit général](../receipts/audit_giant_20261004/README.md),
[contrelecture approfondie](../receipts/audit_deep_20261004/README.md).
Deux idées de versions antérieures retenues : candidat q3 différé commun
catalogue/MEB, extrema q2 couplés pour un helper de census distinct.
[Preuves et contrats de port](../receipts/audit_heritage_20261004/README.md).
Pour la tête plate, B se calcule par LCA, avec date stable3ε et plafond
sB≤t′+d_k/2 ; perte de frontières et stabilité des labels restent distinctes.

**Six Markdown actifs**, sans nouveau journal. Les échanges dépassés sont
[archivés avec leurs empreintes](../receipts/audit_dialogues_20261004/README.md),
[réponse points comprise](../receipts/audit_supports_contract_20261005/notes_before/README.md).
Une contradiction utile devient une fixture liée à ses sources et à son
domaine. Aucun nouveau build/test natif, fit ou GCP lancé par les auditeurs
pour cette publication. Les rapports locaux S2/S4 ne sont pas promus en
qualification G4. Une seule session G4 gardée à la fois, arrêt ciblé certifié.
