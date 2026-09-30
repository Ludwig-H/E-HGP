# Filtre relatif certifié — prototype d'audit

30 septembre 2026. Cette brique prépare une sphère dans le repère de son
ancre, puis encadre ses distances par intervalles extérieurs. Elle répond
à des erreurs de contacts constatées lors des contre-épreuves du port précis.
Elle n'est pas raccordée au moteur et n'en lève pas le refus u18.
`backend=cpu_audit_prototype`, `public_status=not_claimed`, hors registre.

## Ce qui est acquis à ce périmètre

Le [prototype C++](prototype/relative_filter.cpp) accepte des coordonnées
u32 et des coefficients signés de magnitude 192 bits. La conversion entière
conserve les 53 bits hauts et vérifie le reste ; elle encadre le quotient
N/D, avec D strictement positif. Le centre relatif et le rayon sont préparés
une fois. Les écarts site−ancre sont calculés exactement en i64 avant leur
conversion en double. Un site n'est déclaré intérieur ou extérieur que si
les intervalles sont strictement disjoints ; sinon la réponse est `AMBIGU`.
Un contact doit rester ambigu et déclencher le futur prédicat exact.

La boîte fournit une borne de distance **minimale** : elle peut certifier
un rejet extérieur, jamais que tous ses points sont intérieurs. Les opérations
du filtre n'allouent pas explicitement ; la CLI, elle, peut allouer.

Le [panel indépendant](independent/inputs.py) résout les centres de supports
positifs par Gram rationnel, distinct des formules natives. Il ajoute des
sphères rationnelles génériques : celles-ci ne sont pas toutes des supports
Gabriel/MEB positifs. Un seul panel de 3 600 requêtes est utilisé :
2 299 conversions, 1 051 sites et 250 boîtes.

- Normal et UBSan : sorties identiques, deux invocations natives, chacune
  jugée en Python normal et `-O`. Les 196 contacts sont conservés ; 40
  translations communes donnent exactement les mêmes bits de sortie.
- Deux mutants exécutés code0 sont rejetés causalement. Supprimer le reste
  de conversion exclut 2^53+1 à la requête 315. Remettre la marge fixe classe
  intérieur un contact exact à la requête 2687, alors que les intervalles
  imprimés restent corrects. Ces refus ne sont pas des crashes.
- Le lemme de sélection par borne supérieure du rang est contrôlé 147 fois
  sur des listes statiques Python. Ce n'est pas une exécution native de k-NN
  ou du SiteTree.

L'auteur du prototype a fait séparément deux mini-batches de 27 lignes et
une porte native de 50 contrôles, soit trois invocations. Le lot indépendant
compte quatre invocations natives, mutants compris. Les rejugements ultérieurs
ne sont pas de nouveaux cas géométriques ni de nouvelles exécutions natives.

## Environnement et partage

Les calculs publics vérifient FE_TONEAREST et les contrôles SSE2 courants :
arrondi au plus proche, FTZ/DAZ désactivés et exceptions SSE masquées, y
compris après préparation. Un environnement x87 non piégeant reste une
précondition. Les fonctions ne changent pas les modes de contrôle ; les
indicateurs d'exception collants peuvent naturellement changer.
Sans SSE2, le prototype refuse au lieu de présumer ces garanties.

Le descripteur préparé est privé et les requêtes sont const. Le partage
en lecture paraît compatible avec des workers respectant chacun ces règles,
mais aucun test concurrent, TSan ou GPU n'est acquis ici. Les gardes hôte
ne qualifient pas un environnement CUDA. Aucun chrono de contrat n'est mesuré.

## Largeur du futur repli exact

Pour un site, le signe exact relatif est celui de
`P = D*||v||² − 2*N·v`. Pour le protocole générique, avec
M=2^32−1 et U=2^192−1, on a
`|P| ≤ U*(3M²+6M) = 3U*(2^64−1)`.
La borne est atteinte pour D=U, N_i=−U et v_i=M : **258 bits de magnitude**.
Matérialiser cette valeur dans 256 bits serait donc incorrect sans garde.

Le domaine réel de supports u32 possède des bornes plus fortes :
q3 demande au plus 201 bits, q4 au plus 168 bits pour ce signe, selon
l'[audit de largeur](../../development_frontier_precision_20260930/precision_math/AUDIT.md).
Une garde préalable D<2^134 avec N de magnitude 192 bits donne déjà
`|P|<2^227`. Pour le signe seul, un dépassement positif certifié du produit
D*||v||² au-delà de 256 bits prouve aussi P>0, car `|2*N·v|<2^227`.
Cette dernière astuce ne fournit ni la valeur exacte P ni son ordre.
Ces options sont des bornes mathématiques ; aucun repli n'est implémenté.

## Juges, préflights et provenance

La [contre-vérification R1](judge_adversarial_r1/receipt.json) refuse 48
falsifications de sorties, cinq altérations de requêtes et une clé JSON
dupliquée, normal et `-O`. Trois tolérances hors contrat numérique sont
consignées, pas effacées. R2 refuse les IDs booléens et NaN/±Infinity
littéraux. Le contrôle indépendant trouve encore `1e999` converti en infini
dans un champ ignoré ; R3 ajoute un contrôle de conversion flottante.
Les sources et captures R1/R2/R3 sont séparées. R3 rejoue les mêmes quatre
sorties natives, normal/`-O`, et sept refus, sans relancer de natif.

L'omission d'un marqueur facultatif `exact_contact` n'est pas un défaut
numérique du filtre : le panel doit rester gelé, haché et régénérable.
Même sans ce marqueur, un contact ne peut passer le test exact strict.
Le lecteur vérifie les hashes, régénère le panel, rejoue les trois juges
sur les sorties archivées et contrôle les conclusions adversariales enregistrées.

Le générateur initial contenait deux coefficients de 193 bits ; ils ont
été corrigés **avant transmission au natif**. La relecture de domaine est
un nouveau diagnostic Python, pas la capture inventée d'un échec natif passé.
Un premier build de smoke chevauche l'ajout d'une assertion de type ; non
exécuté, il reste historique. Le smoke v2 figé est le témoin exécuté.

Les binaires sont hachés mais restent hors reçu ; leurs chemins d'origine
ont changé après les exécutions, à hashes identiques. Les dépendances système
ne sont pas embarquées. Ce reçu est une archive de sorties, pas une preuve
LIVE d'une recompilation ou d'une exécution du moteur.

```bash
python3 -B morsehgp3D_v10/receipts/audit_continu_20260929/relative_filter_20260930/verify.py
python3 -B -O morsehgp3D_v10/receipts/audit_continu_20260929/relative_filter_20260930/verify.py
```

Manquent toujours : raccord au propriétaire/index, supports et niveaux
larges, repli exact, nearest natif, catalogue/FULL, export des unités/IDs,
tests de croissance 8k/16k/32k et qualification G4. Aucun contrat 100 ms,
gain de performance ou caractère sous-quadratique global n'est acquis.
GCP non utilisé ; aucune source du moteur modifiée dans cette tranche.
