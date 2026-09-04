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

Ces acquis justifient de poursuivre cette architecture. La qualification
industrielle exacte demande encore de composer les preuves globales, de
certifier la verticale si elle fait partie de l'objet livré et de mesurer
les coûts de bout en bout. Une campagne bornée ne suffit pas à ces trois
conclusions. Le statut `not_claimed` et le refus de `--require-exact` restent
cohérents avec les preuves disponibles.

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
| Entrée et archive | L'ordre des points, les refus, la cohérence des lots et le rejeu structurel sont testés, y compris après corruption et recalcul des hashes. | [Audit des interfaces](AUDIT_INTERFACES_20260904.md) |
| Tri | Objet stable conservé, zéro tampon local observé ; pic supplémentaire de heap de 262 736 octets pour 32 768 enregistrements et deux ouvriers. | [Audit de résidence](AUDIT_RESIDENCE_20260904.md) |
| Construction ciblée | `selftest` et `bad_alloc_gate` reconstruits en Release GCC 13.3 avec les avertissements fatals ; résultats des cinq portes associées épinglés. | [Reçu Release courant](receipts_20260904/release_current.json) |

Chaque rapport identifie les octets réellement exécutés. Le HEAD de départ
`de69851e3820781145f859a08a993f15f2f9e738` ne représentait pas les sources v7
non suivies. Les manifestes et hashes restent indispensables pendant le
travail concurrent du constructeur. Le [reçu de validation](receipts_20260904/validation_current.json)
centralise les portées, les commandes et le dernier contrôle de fraîcheur.

## Verrous à lever et critères de fermeture

### 1. Composer la preuve horizontale

La preuve locale détaillée dans l'audit mathématique montre pourquoi une
seule première incidence peut suffire : les co-minimiseurs non-Gabriel ont
un apex antérieur commun sous les hypothèses de support et de régularité.
La décroissance exacte et le terminal direct donnent un certificat de
localisation. C'est un argument constructif pour le choix actuel.

Il reste à relier, dans un même énoncé, le catalogue direct complet,
l'autorité de régularité sur les objets non visités, la fenêtre de rang et
l'inertie des blocs de haut rang, la rétraction sur le cœur, puis les
transitions normalisées. Les contrôles locaux de shells et les petites
fixtures ne remplacent pas ces prémisses globales. **Critère de fermeture :**
chaque prémisse est soit prouvée pour le profil u16 annoncé, soit vérifiée
par un certificat et un refus explicite ; leur composition désigne
exactement l'objet exporté.

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

**Prochaines actions mesurables :** compter les capacités réellement
vivantes dans l'admission mémoire ; essayer une destination directe
d'expansion par comptes et sommes préfixes ; mesurer les distributions de
longueur des chaînes et les reparcours spatiaux. Conserver les objets et
refus appariés à chaque optimisation. Une libération par lots doit expliquer
comment une incidence ultérieure retrouve son représentant certifié.

Les campagnes 8k/16k/32k, 50k et grande échelle devront épingler entrée,
source, binaire, options, statuts, RAM, latence et compteurs de travail sur
la route avec complétion activée. Un budget sur des tampons nommés ne vaut
pas plafond RSS ; un refus borné ne vaut pas résultat d'échelle.

## Limites de validation

Les preuves actives ci-dessus sont des vérifications CPU ciblées sur leurs
snapshots identifiés. Cet audit ne revendique pas une suite Release complète
sur le dernier ensemble de sources. Les résultats des campagnes antérieures
de cette séance restent des reçus bruts séparés, jamais un verdict courant.
Le contrôle documentaire global exclut ces audits ; leur Markdown et leurs
liens sont donc contrôlés explicitement en complément.

GCP non utilisé. Aucun résultat CUDA matériel ni mesure GPU dans cet audit.
