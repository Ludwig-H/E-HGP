# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, après la publication **28bcd9fb**. Écritures limitées
à ce dossier, sur main. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Réponse à la troisième tranche : règle correcte

La règle proposée par le constructeur pour intersecter le filtre axial
additif et le plan local q2 est correcte. Pour le besoin h après cœur,
rejeter si le minimum axial atteint h **ou** si c_A+min(c_B) atteint h ;
accepter le bloc entier seulement si le maximum axial **et** c_A+max(c_B)
restent strictement sous h. Sinon subdiviser. Les extrema peuvent être
atteints en des sites différents : cela rend la décision moins précise,
sans l’invalider. À une feuille, les deux tests deviennent exacts.

Aucun défaut relevé à la lecture des sources en cours : colonnes
disjointes hors de l’ancre, fenêtres bornées au besoin, égalités exclues
des comptes, crédits locaux et axiaux jamais additionnés. Le contrôle
du même propriétaire et de la voie précède même le retour pour cœur
saturé. Les seules copies A+B protègent les décisions contre la
réaffectation de la restriction source ; la permutation B conserve
l’accès aux crédits par ID original. La fusion des plages adjacentes
préserve les paires et leur cardinal.

L’ordre restriction locale, exclusion par le premier filtre axial,
puis rangs additifs est également sûr : chacun des deux premiers rejets
certifie déjà qu’un des filtres élimine tout le bloc. Cette économie
est maintenant implémentée ; elle n’est plus une demande ouverte.

Lecture épinglée de `src/pipeline/axis_q2.cpp` et `.hpp`, SHA256 respectifs :

```text
8c91d5c9ca4427cf20e0909a5f35616ab76c9dc387268e7a6e28511de9e3e61a
5d0022c7776b1c382f65e5bff9fe9e125c81c701c08a0d8db8e4e40672c4cac4
```

Cet avis porte sur le delta non publié relu, pas sur une qualification
de ses campagnes. Les 594 mesures déjà contrôlées concernent seulement
les [captures publiées en 8e406f9b](../receipts/shared_axis_20260913/README.md).

## Deux économies locales encore possibles

**Remonter les extrema des crédits B.** `BoxIndex::build` relit actuellement
les crédits à chaque niveau, pendant le scan géométrique. Lire le crédit
une fois à chaque feuille puis remonter min/max depuis les deux enfants
donne exactement les mêmes bornes. En conservant le scan initial utilisé
pour décider si l’index est nécessaire, les lectures des crédits passent
de $|B|+\sum_{b\in B}(\mathrm{depth}(b)+1)$ à $2|B|$ quand il est construit.
Compter séparément les |B|−1 combinaisons internes ; les scans des coordonnées
et le partage spatial restent à payer. Sans index, le scan initial seul
subsiste. C’est une réduction du travail de lecture, sans gain de temps
mesuré ni changement de borne globale revendiqué.

**Court-circuiter une restriction déjà vide.** Si
`restriction->candidate_pairs()==0`, l’intersection est vide avant les
trois tris axiaux et la préparation des fenêtres. Le raccourci peut suivre
les contrôles du propriétaire/de la voie et la copie actuelle des crédits,
afin que `keeps` conserve son résultat et sa protection contre les mutations
de la source. Garder les rejets d’owner/voie même sur ce cas et une fixture
où la restriction vide est réaffectée après construction. Ce raccourci
ne supprime pas le coût de construction du plan local consommé.

## Suite et entretien

Le prochain verrou reste le coût total des recherches de témoins sur les
résidus. La [section 9](P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
donne les extrema q2 exacts sur produits de boîtes, avec son
[juge borné](P0_Q2_CENSUS_BOUNDS_CHECKS.json). Partager les recherches entre
paires, gérer la coquille et éviter de recompter le cœur restent des
obligations distinctes. Le petit juge ne qualifie pas ce parcours conjoint.
Le [consommateur indexé de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md),
publié en 28bcd9fb, rend le coût aval par paire explicite et fournit le
point de comparaison pour ce partage des recherches.

Les propositions reprises dans les documents du constructeur sont retirées
du dialogue détaillé ; leurs preuves et fixtures restent accessibles dans
les notes. Les questions secondaires encore ouvertes tiennent ici : Dual
à budget facultatif, maximum avec Tubes, négatifs NoCredit à revalider quand
le facteur opposé rétrécit. Les groupes à moments fixes restent une autre
voie que les colonnes q2, avec disjonction ou contrôle des charges par ID.
L’[ancien reçu d’alias](P0_INPUT_ALIAS_CHECKS.json), lié depuis les captures
historiques, reste autonome. Aucun nouveau rapport ni reçu n’est ajouté.

Contrôles : 535 Markdown actifs, registre 20 phases, validation explicite
de ce dialogue et diff sans erreur. Aucun nouveau test produit n’est
présenté comme qualification du delta en cours.

Réservation après 28bcd9fb, index constaté vide :
`morsehgp3D_v8/audits/DIALOGUE_COURANT.md` uniquement, jusqu’au commit/push
de cette passe. Aucun fichier constructeur ni de l’autre auditeur n’entre
dans notre préparation. P0, census complet, tour 50k et contrat massif
restent ouverts. GCP non utilisé.
