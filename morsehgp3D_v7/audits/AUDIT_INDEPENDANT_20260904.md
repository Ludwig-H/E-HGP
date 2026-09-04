# Audit indépendant courant de MorseHGP3D v7

Date : 4 septembre 2026. Écritures limitées à `morsehgp3D_v7/audits/`.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

## Avis pour le constructeur

La v7 dispose d'une voie concrète vers une hiérarchie horizontale exacte :
catalogue direct, complétion des incidences silencieuses par descente stricte,
fold normalisé, puis rejeu des deltas. Les contre-fixtures permanentes
contrôlent les matérialisations, les continuations et les identités publiées.
La destination directe du census et le tri par permutation réduisent les
copies sans construire la mosaïque de Delaunay d'ordre supérieur.

La composition horizontale possède maintenant une justification sous les
prémisses S explicites du constructeur. La contre-lecture ferme les raccords
d'ancrage au cœur et d'inertie sans exiger une régularité géométrique globale.
La couverture S1 possède aussi un théorème géométrique conditionnel complet,
jusqu'au représentant d'arité minimale après RLE. La qualification
industrielle exacte demande encore de vérifier ses hypothèses sur les
primitives et le domaine d'exécution du produit, de
certifier la verticale si elle fait partie de l'objet livré et de mesurer
les coûts de bout en bout. Une campagne bornée ne suffit pas à ces trois
conclusions. Le statut `not_claimed` et le refus de `--require-exact` restent
cohérents avec les preuves disponibles.

Le défaut de nettoyage d’archive est **corrigé et requalifié** : la probe
indépendante conserve son refus d’allocation et revient sans terminaison
ni résidu. Les quatre portes archive/API passent, y compris les refus
tardifs K1/K2 et les diagnostics OS. Les 26 scènes CLI et six corruptions
sont aussi rejouées avec succès sur le nouveau binaire. Les
[preuves du delta](RETOUR_ARCHIVE_COURANT.md) lèvent A1 ; l’enregistrement
CTest du banc d’incidences lève C1 dans les [échanges courants](DIALOGUE_COURANT.md).

## Modèle lu et objet effectivement audité

Les parties I, pages PDF 35–76, puis II, pages PDF 77–134, du
[manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) ont été lues
intégralement. SHA-256 :
`579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.

Le théorème 2 relie les composantes des régions témoins, celles de Gamma et
les ensembles de points couverts. La proposition 5 permet les cofaces
élémentaires pour préserver les composantes, sans identifier les adjacences.
Le théorème 4 fournit la descente par remplacement d'un sommet essentiel.
Le § 9.1 distingue la partition des facettes et le recouvrement des points.

L'option `--complete-incidences` sélectionne
`forest_semantics=normalized_horizontal_h0_candidate`. Au-dessus de l'ordre
un, les facettes latentes ne sont pas des parents enracinés. `born` signifie
première matérialisation dans le sous-flot retenu ; ce champ ne donne pas la
naissance géométrique exhaustive dans Gamma. Les singletons de l'ordre un
restent normatifs. Le rendu pondéré et les applications entre ordres sont
des contrats distincts de cet export horizontal.

## Acquis et preuves exécutables

| Domaine | Résultat vérifié | Autorité détaillée |
| --- | --- | --- |
| Géométrie et réduction | Le triangle aigu donne zéro parent et trois matérialisations à la naissance ; E5 conserve ses continuations. Les contre-fixtures distinguent connexité, matérialisation et identité. | [Audit mathématique](AUDIT_MATHEMATIQUE_20260904.md) |
| Fenêtre de rang | Catalogue indépendant jusqu'à 24 points ; Gamma et deltas à K=2 et K=10 sur quatre exécutions à 13 points ; 88 contacts stricts et deux mutants ciblés. | [Retour mathématique](RETOUR_MATH_COURANT.md) |
| Entrée et archive | L'ordre des points, les refus, la cohérence des lots et le rejeu structurel sont testés, y compris après corruption et recalcul des hashes. | [Audit des interfaces](AUDIT_INTERFACES_20260904.md) |
| Tri | Objet stable conservé, zéro tampon local observé ; pic supplémentaire de heap de 262 736 octets pour 32 768 enregistrements et deux ouvriers. | [Audit de résidence](AUDIT_RESIDENCE_20260904.md) |
| Admission mémoire | Deux refus conservatifs évitables reproduits ; concurrence bornable par le nombre d'ordres ; coexistence A2/B1 mesurée sans assimiler proxy et RSS. | [Retour mémoire](RETOUR_MEMOIRE_COURANT.md) |
| Construction indépendante | Snapshot initial : toutes les cibles Release construites, 279/279 portes CPU, 203 fichiers et 31 binaires inchangés. Delta archive qualifié séparément. | [Reçu Release courant](receipts_20260904/iteration2/validation.json) |
| Mode mono | Quatre portes passent avec interposition réelle de `pthread_create` ; le nouveau CLI repasse les 26 scènes et six corruptions d’interface. | [Qualification mono](MONO_COURANT.md) |
| Banc d'incidences | Sept tests passent normalement et sous Python optimisé ; un vrai refus K=2 est classé sans succès moteur. Les deux CTests sont enregistrés et passent. | [Classification courante](CAMPAGNE_INCIDENCES_COURANTE.md) |

Chaque rapport identifie les octets réellement exécutés. Le HEAD de départ
`de69851e3820781145f859a08a993f15f2f9e738` ne représentait pas les sources v7
non suivies. Les manifestes et hashes restent indispensables pendant le
travail concurrent du constructeur. Le [reçu de validation](receipts_20260904/validation_current.json)
centralise les portées, les commandes et le dernier contrôle de fraîcheur.

## Verrous à lever et critères de fermeture

### 1. Rattacher la preuve S1 aux qualifications du produit

La preuve locale détaillée dans l'audit mathématique montre pourquoi une
seule première incidence peut suffire : les co-minimiseurs non-Gabriel ont
un apex antérieur commun sous les hypothèses de support et de régularité.
La décroissance exacte et le terminal direct donnent un certificat de
localisation. C'est un argument constructif pour le choix actuel.

La [composition conditionnelle](REPONSE_AUDITEUR_COMPOSITION.md) raccorde
les chaînes retenues, l'activation du cœur, l'inertie hors fenêtre et les
composantes par inclusion des facettes. Chaque composante possède un
ancrage direct ; identifier ces composantes par les seuls points couverts
serait incorrect à ordre supérieur. Les contrôles locaux suffisent à
écarter les contacts égaux avec les blocs irréguliers omis.

La [matrice S1](S1_COURANT.md) prouve ce parcours sous les contrats explicites
des primitives : toute boule pertinente atteint son propriétaire, son seed
et sa complétion, survit aux prunes stricts et conserve son arité minimale
après RLE. Les preuves séparées couvrent les témoins, secteurs, cordes,
cellules et marges flottantes, arrondis des bornes finales compris.

Le constructeur a intégré ces lemmes au théorème horizontal et publié la
[cartographie des primitives](../docs/QUALIFICATION_S1_PRIMITIVES.md). Les bornes
de cardinal/Karras et de PGCD y sont cohérentes avec la lecture indépendante.

**Critère de qualification produit :** rattacher la validité de l'index,
des tris, PGCD, Cramer,
produits larges et contrôles de shells aux octets exécutés. Le domaine
binaire64, l'arrondi et la séquence de calcul doivent être ceux qualifiés
par le build. Les fixtures de rang onze/douze en apportent des réfutations
ciblées ; elles ne remplacent pas la qualification universelle des primitives.

### 2. Déclarer la verticale et le rendu

L'archive annonce `vertical_maps=none`. Une collection de forêts par ordre
ne constitue pas à elle seule une hiérarchie multi-ordre certifiée.
**Critère de fermeture si cet objet est requis :** définir les applications
entre ordres, leurs domaines et leurs carrés de naturalité, puis vérifier
leurs certificats séparément de l'égalité horizontale à chaque coupe.

Pour le rendu du § 9.1, conserver les incidences et les contributions
nécessaires aux poids de vote. Les seules feuilles et leurs deltas ne
donnent pas ces multiplicités. Le carrier, la politique de coupe et le sens
du niveau doivent être déclarés par le consommateur.

### 3. Qualifier la mémoire et le coût de la complétion

La miniball locale examine au plus onze sites, donc au plus 550 supports
de tailles deux à quatre. Ce plafond local ne borne ni le nombre de
facettes du cœur ni la longueur totale des chaînes. Le tableau `BallData`
reste global pendant les folds, et l'expansion conserve des sorties par
tranche avant fusion.

**Prochaines actions mesurables :** appliquer le plafond constant
`min(fold_inflight, kmax_eff)` aux deux gardes de concurrence et précontrôler
le facteur silencieux `+3`, selon les fixtures du [retour mémoire](RETOUR_MEMOIRE_COURANT.md).
Le proxy déclaré reste distinct des capacités observées et du RSS.
Essayer ensuite une destination directe
d'expansion par comptes et sommes préfixes ; mesurer les distributions de
longueur des chaînes et les reparcours spatiaux. Conserver les objets et
refus appariés à chaque optimisation. Une libération par lots doit expliquer
comment une incidence ultérieure retrouve son représentant certifié.

Les campagnes 8k/16k/32k, 50k et grande échelle devront épingler entrée,
source, binaire, options, statuts, RAM, latence et compteurs de travail sur
la route avec complétion activée. Un budget sur des tampons nommés ne vaut
pas plafond RSS ; un refus borné ne vaut pas résultat d'échelle.

Le [contrat de performance v7 actuel](../docs/CONTRAT_PERFORMANCE.md)
vise 50 000 points et toute la tour K=1 à 10 sous une seconde, avec repli
sur toute la tour K=1 à 5. Il demande d'abord une qualification mono-thread,
puis multi-CPU et GPU ; le jalon suivant est 100 ms sur le même périmètre,
et la cible massive est GCP G4. Les présentes mesures ne qualifient aucun
de ces seuils.

## Limites de validation

La suite indépendante couvre les 279 portes `gate` du CMake figé, avec
46 tests d'échelle exclus. Les 203 fichiers copiés et 31 binaires sont
inchangés après exécution. Les modifications du banc, d’archive et du fold mono sont qualifiées par
leurs propres reçus ciblés. Le CLI mono n’est pas assimilé au binaire du snapshot
initial ; ses interfaces ont été rejouées. Le dernier delta
[AxisBounds](CENSUS_AXIS_COURANT.md) est relu statiquement : ses portes et
son CLI n’ont pas été exécutés indépendamment dans cet audit. Les durées sur l'hôte partagé ne
sont pas des mesures de performance industrielle. Les résultats antérieurs
restent des reçus bruts séparés, jamais un verdict courant.
Le contrôle documentaire global exclut ces audits ; leur Markdown et leurs
liens sont donc contrôlés explicitement en complément.

GCP non utilisé. Aucun résultat CUDA matériel ni mesure GPU dans cet audit.
