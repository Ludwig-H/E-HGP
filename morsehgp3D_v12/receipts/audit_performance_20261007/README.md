# Audit de la régression CPU et aide mathématique au développeur

7 octobre 2026. Demandes : « Reprends ton rôle d'auditeur et aide le développeur ; ce n'est pas normal d'avoir
une v12 plus lente que la v11 », puis « Audite et aide le développeur aussi mathématiquement ».

Code contre-lu : **`58d384721678d11ef8ccd86c76cca182f41a716c`** ; catalogue livré en `671072339`.
Mesures : sessions F2 (produit CPU), E (microbancs et v11 gelée), A/C (noyaux GPU), sans nouveau lancement.
La publication ultérieure G (`5c5fc7109`) est signalée séparément, sans nouvelle qualification de ses prises.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

## Avis et actions, par priorité

La régression du catalogue CPU est réelle sur les prises F2 : médianes chaudes K5/feuille24
**451,606 / 372,746 / 469,481 ms** sur ng00/01/02, contre la référence historique du domaine CPU v11
**200 / 163 / 195 ms**. C'est un rapport descriptif voisin de 2,3, pas une campagne A/B appariée :
le chrono v12 exclut préparation du nuage et destruction du résultat, la v11 a son propre périmètre.
Ni FULL v12 ni le catalogue GPU intégré n'ont été mesurés. Cette distinction n'excuse pas le surcoût CPU.

1. **Reprendre la finition avant de promettre le budget.** Assemblage+table : **122,5 à 171,3 ms** à K5.
   En gardant les autres coûts CPU identiques et la même séquence, supprimer entièrement parcours,
   comptage et écriture des feuilles laisse **145,1 à 201,7 ms** de catalogue. C'est une estimation
   conditionnelle à partir de chaque passe, pas une prédiction de la future voie GPU. Paralléliser rangs,
   préfixes CSR, copies et table ; transférer réellement la finition sur l'appareil pour la voie prévue.
   Le contrat de 35–45 ms n'est pas confirmé par l'addition des deux microbancs M2/M5.
2. **Supprimer du travail physique inutile dans la feuille CPU.** Les paires sont évaluées deux fois ;
   la simulation CPU du census continue après son rejet logique. Les compteurs reproduits de la v11
   ne comptent pas ces répétitions. Le reçu des feuilles contient l'instrumentation et une variante
   d'arrêt CPU testée hors produit. Préserver une source algorithmique, en adaptant l'exécution SIMD/SIMT ;
   ne pas réintroduire un second moteur DFS sans décision explicite. Un gain de temps reste à mesurer.
3. **Préparer les clés, pas affaiblir les prédicats.** Des rangs lexicographiques de sites et quatre rangs
   par support reproduisent exactement le départage par positions. Mettre en balance les octets ajoutés
   et le coût de tri mesuré (le tri seul n'est pas le poste dominant). Les preuves de réemploi des coquilles,
   populations et parties complètes sont livrées avec leurs conditions ; une empreinte seule n'est jamais
   un certificat. Ne pas élargir aveuglément les masques pour traiter les grosses dégénérescences.
4. **Pour la tour, suivre les mesures.** G-L3 évite 81–83 % des censuses saturés mais ralentit la résolution
   de 3–4 % : rejet confirmé. MES-M7 place les sondes puis la proposition de boule avant le census. La
   jointure de populations, le mémo et les autres leviers doivent être jugés sur le coût complet de G,
   préparation et mémoire comprises, puis dans FULL. Le profil instrumenté ne se compare pas comme un
   nouveau chrono de performance au bras non instrumenté.
5. **Diagnostiquer les dégénérescences avant d'élargir les capacités.** Le reçu G décrit `synth_sphere`
   comme exactement cosphérique ; cinq points du générateur arrondi réfutent cette propriété, pour chacune
   des trois graines vérifiées. Le refus `wide_leaf` demeure, mais ne prouve pas une coquille de même taille
   que la liste candidate. Le nouvel objectif sans refus de largeur demande une extension déclarée des
   limites 256/64 et des types, avec une stratégie qui évite l'énumération aveugle des présentations.
   [Contre-exemple exact et protocole](mathematiques/COMPLEMENT_SESSION_G.md).

## Livrables reproductibles

| Reçu | Preuve et usage |
| --- | --- |
| [Mesures](mesures/README.md) | relecture des prises brutes, bornes conditionnelles, séparation des périmètres, campagne suivante proposée |
| [Assemblage](assemblage/README.md) | preuve du scan bloqué avec halo, forme du premier niveau conservée, modèle testé et table par permutation |
| [Feuilles](feuilles/README.md) | prédicats physiques instrumentés, compteurs et émissions préservés, variante CPU bornée hors produit |
| [Mathématiques](mathematiques/README.md) | équivalence sémantique FULL, clé de support exacte, clés de caches suffisantes et contre-exemples |

Le scan proposé a **3 537** confrontations à un oracle de tableaux, quatre mutants tués et **13 468**
requêtes de table ; il ne constitue pas un port natif qualifié. La preuve mathématique teste **30 381**
comparaisons de clés et **66** présentations critiques. Les autres effectifs et épingles sont dans chaque reçu.

## Campagne à demander au développeur, sur son prochain instantané G4

- Même donnée, profil u21, K, feuille et périmètre pour v11 gelée/v12 base/v12 corrigée ; source et binaire
  épinglés. Lire et préparer une fois les nuages ; publier construction et destruction séparément, puis le
  vrai coût résident FULL quand il existe. Ne pas transformer la répétition d'une seule trame en preuve
  de résidence sur trames successives.
- Au moins cinq processus frais par bras et cas, dix passes chacun, bras entrelacés ; médianes et maxima,
  rapports appariés/IC, mémoire hôte et appareil. ng00–02 servent au diagnostic ; la décision de vitesse
  doit ensuite porter sur plusieurs séquences (`v12set`), les petits nuages et les scènes volumineuses.
- Ablations : finition seule ; travail de feuille seul ; cumul ; comparaison au chemin GPU intégré incluant
  les émissions, les replis, transferts, tri/rangs/CSR/table et coexistences mémoire. Un microbanc de
  comptage des feuilles n'est pas le coût du matériel de catalogue qu'il faut effectivement produire.
- Gardes : catalogue exact/oracle, empreinte sémantique FULL (formes rationnelles normalisées, pas de
  tolérance), W1/W48, mêmes refus, plateaux traversant les blocs, supports non canoniques absents de la
  table, collisions d'empreintes, numérique u21/u24/u32, budget et concurrence. Tests lourds et chronos G4,
  session gardée et verrou commun. Ne pas rendre les tests de preuve dépendants de ce reçu.

## Portée

Audit de code, calculs statistiques sur reçus existants et petits témoins synthétiques seulement.
Pas de nouveau GPU/GCP, de recalcul LiDAR, de correction produit intégrée, de matrice native ou de contrat
FULL/100 ms acquis. Aucun jeu sous licence ni copie de source complète dans ce reçu. Les outils de rejeu
lisent les sources épinglées et publient leurs empreintes ; `SHA256SUMS` ferme ce lot après vérification.
