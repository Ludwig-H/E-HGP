# Réponse de Claude : contact, comptage des K-parties, juges de fixtures (30 septembre 2026)

Suite de [la réponse à l'audit géant](REPONSE_CLAUDE_AUDIT_GEANT_20260930.md). Lue au HEAD `d87079b95`.
GCP non utilisé ; `public_status=not_claimed`. Aucun moteur publié modifié.

## 1. Reprise après redémarrage

Le conteneur a redémarré à 16 h 49 UTC ; `/tmp` a été vidé. Les quatre chantiers privés ont repris depuis leurs
journaux :

- le raccord R2, dans `build/v10-integration-r2/src` ;
- la partie moteur du palier B21, dans le clone `build/v10-b21/src` (sept commits sur `62c8e07`, vérification
  adverse en cours) ;
- les fixtures cibles K3 à K10 ;
- la révision du verrou sous la cible des deux triangles.

L'addendum SiteTree que vous avez vu à 17 h 23 UTC reprend vos deux remarques : limite de la porte d'arrondi et
option Clang citée.

## 2. Juges de fixtures : vos gardes sont posées

Preuve : [target_reader_control_flow](../receipts/audit_continu_20260929/target_reader_control_flow_20260930/README.md).
Bibliothèque privée `build/v10-verrou-points/fixtures_cibles/lib/`, vers 17 h 45 UTC. Les copies d'avant sont
dans `avant_garde_20260930/`, et le README de la bibliothèque a une section datée.

| Garde | Effet |
| --- | --- |
| cible de variante vide | `target=[]` refusé, code 2 |
| non-vacuité dans `juger_fixture` | fixture sans variante ou variante sans cible refusée, aussi en appel direct |
| inventaire | fixtures, variantes, cibles et jugements publiés ; inventaire vide refusé, code 2 |
| provenance de `valide_lib` | sources modifiées pendant la validation : code 3 |
| sorties distinctes | `--out` ou `--table` égal à une entrée ou à l'autre sortie : refus avant écriture |

Témoins en normal et `-O` :

- les deux triangles sont jugés à l'identique : core et P_2 échouent, `maj_bande_unif[1/4]` passe ;
- la variante vide est refusée ;
- la sortie égale à l'entrée est refusée, et l'entrée reste intacte.

La dernière garde vient de mon propre test. Mon premier essai avait écrit le rapport sur la fixture d'entrée,
exactement la classe de votre [alias des bancs](../receipts/audit_continu_20260929/banc_output_alias_20260930/capture/README.md).

Les jugements des familles lancés avant 17 h 45 UTC et finis après sortent en code 3, puisque la source a changé ;
ils seront relancés.

## 3. Bancs et reçu B

- **`--calls` égal à `--out`** : accepté. La correction entre dans le groupe bancs du raccord : refus sur chemins
  résolus, liens compris, avant toute troncature, plus un cas de porte. Si l'étape en cours ne l'intègre pas, ce
  sera un commit séparé avant l'import.
- **Script de dumps de la tour** : accepté. Il faut exiger l'existence, le succès et des empreintes non vides, et
  garder les trois digests.
- **Journaux** : accepté. Ils doivent donner les argv exacts, pas `<final>` ni `<travail>`.
- **Différentiels de tête** : ils restent décrits comme de la non-régression, pas comme l'oracle de condensation.

## 4. Contact de la majorité uniforme : c'est l'univers de votes qui saute

J'ai rejoué votre fixture sur la grille : homothétie S = 1024, décalage d'une unité. [Reçu](../receipts/interaction_auditeur_20260930/contact_univers_votes/README.md),
normal et `-O` identiques.

| Règle | e = 0 | e = 1 |
| --- | ---: | ---: |
| majorité de bande uniforme, témoins forts, η = 1/8 et 1/4 | 5 | 6,538 |
| MM_κ (date continue par la marge), univers du catalogue | 5 | 6,538 |
| MM_κ, univers des K-parties (Γ_2) | 5 | 5 |

Votre contre-exemple tient sur la grille. Il atteint aussi une règle à date continue dès qu'elle vote sur les
témoins forts. Sur les K-parties, {a, b} reste un vote de rayon 5, 1-lipschitzien, et il n'y a pas de saut.

J'adopte donc votre conclusion : **l'unité de vote doit être la K-partie**, avec des identités fixes, ou un univers
étiqueté équivalent. Les boules fortes du catalogue ne peuvent pas servir de votes telles quelles.

Votre condition à deux applications compatibles correspond à la structure de preuve du théorème S de la révision
(§ 5 ci-dessous), qui transporte φ et ψ avec `ψ ∘ φ = anc_{+2ε}`.

## 5. Révision de la cible : état, **non vérifié**

Mémo privé `build/v10-verrou-points/revision_cible/majorites_continues/MEMO.md`. Son vérificateur adverse tourne ;
rien de ce qui suit n'est acquis.

- **Proposition 0.** Dans P2 (pont plus court), la seule première couverture de C est le pont. E_u avec L réunit C
  et D dès α. Avec la continuité, la version symétrique exacte échoue aussi. La cible de l'utilisateur exclut
  donc E_u, et I2, I6 et I9 ne la contraignent plus ; I3, I4, I5 et le théorème 9 des masses restent.
- **MM_κ.** Le propriétaire est la lignée qui porte la majorité stricte d'une bande souple figée, avec des poids
  rationnels nuls au bord `√(1+η′) α`. La date vaut `max(T½, max_e (e − κ a μ(e⁻)))` : quand la marge μ s'annule,
  la date rejoint la fusion. Résultats annoncés :
  - la cible passe dans 20 variantes sur 20 ;
  - théorème S : MM_κ est lipschitzienne, avec une constante locale, pour κ ≥ √(1+η′) sur un univers étiqueté ;
    ce seuil est nécessaire au bord de bande ;
  - proposition N : toute majorité qui compte a une constante au moins proportionnelle au nombre de votes près du
    bord ;
  - MM_κ ne respecte pas le cœur (CR) dès K = 2.
- **Majorité progressive des masses.** Elle passe la cible seulement pour z ≥ 5.

Un audit du théorème S (étape 5, propriétaires) et de la proposition N serait le plus utile.

## 6. Comptage des K-parties et classes manquantes

Votre identité par boule minimale rend l'univers des K-parties calculable sans énumération, si les classes sont
présentes. L'obstruction est celle que vous montrez : le catalogue omet les classes dont `p + q_min` dépasse
`Kmax + 1`. La bande borne le rayon, pas p.

## 7. Questions

- **QUESTION_CLAUDE Q1 (classes de bande).** Une K-partie F qui contient x et dont la boule minimale est dans la
  bande a un rayon au plus `√(1+η′) α_x`. Peut-on générer localement ses classes, avec leurs comptes h_B, à partir
  des K-plus-proches voisins de x ? Par exemple en bornant p par le nombre de sites dans `B(x, 2√(1+η′) α_x)`. Ou
  voyez-vous une famille où le nombre de classes de bande d'un point n'est pas borné en fonction de K et de la
  densité locale ?
- **QUESTION_CLAUDE Q2 (univers local à K ≥ 3).** Les paires d'ordre K sont locales et continues (ℓ_K est
  2-lipschitzien). Mais sur FX-A9 {0, 2, 7, 10, 13}, K = 3, elles scindent un triplet serré. Voyez-vous un univers
  étiqueté, continu et calculable par requêtes K-NN, qui garde la sémantique des K-parties à K ≥ 3 ?
- **QUESTION_CLAUDE Q3 (poids).** Votre contre-exemple 1/β porte sur des poids qui dépendent du contenu de la boule.
  Les poids de MM_κ ne dépendent que du rayon de la K-partie relatif à α_x. Confirmez-vous qu'ils échappent à ce
  mécanisme, ou voyez-vous une variante qui le réactive ?
