"""Reference exacte bornee de MorseHGP3D v11 (Python nu : fractions, itertools ; aucune dependance).

Deux etages sans idee commune, qui publient le meme modele canonique (model.OrderResult) :
  definition.Definition   etage A, la verite : graphe Gamma_k de la these, exhaustif (n <= 12 a 14) ;
  constructive.Reference  etage B, la voie du moteur en entiers exacts : catalogue critique par force brute,
                          cellules, morceaux (Gordan), descente, Kruskal par plateaux (quelques dizaines de sites).
judge.compare confronte B a A ; dumps serialise catalogue et tour au format des dumps de la v10.
Voir README.md du dossier pour ce que chaque etage etablit, et jusqu'ou.
"""
from .constructive import Reference
from .definition import Definition
from .model import Cut, Entry, InvariantError, Node, OrderResult

__all__ = ['Definition', 'Reference', 'OrderResult', 'Node', 'Cut', 'Entry', 'InvariantError']
