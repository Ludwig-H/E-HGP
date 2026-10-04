#!/usr/bin/env python3
"""Transcription ligne a ligne des fonctions cdef de sklearn/cluster/_hdbscan/_tree.pyx (scikit-learn 1.9.1,
identique a 1.7.2 hors style) qui ne sont pas appelables depuis Python : _compute_stability (l. 240-278),
traverse_upwards (l. 578-604), epsilon_search (l. 606-640), _get_clusters (l. 643-799, sans les probabilites).

Usage limite : verifier la semantique d'epsilon que numpy 2.5.3 empeche d'executer dans le code compile
(traverse_upwards convertit un tableau de taille 1 en scalaire : TypeError depuis numpy 2.5). La condensation et
l'etiquetage restent ceux de sklearn (_condense_tree et _do_labelling sont cpdef, appeles tels quels).
Ce n'est PAS l'adversaire de la comparaison : HDBSCAN tel quel reste sklearn.cluster.HDBSCAN.
La transcription est validee la ou le code compile s'execute (equivalence.py, colonne transcription_vs_fit).
"""
import numpy as np


def compute_stability(condensed_tree):
    parents = condensed_tree['parent']
    largest_child = int(condensed_tree['child'].max())
    smallest_cluster = int(np.min(parents))
    num_clusters = int(np.max(parents)) - smallest_cluster + 1
    largest_child = max(largest_child, smallest_cluster)
    births = np.full(largest_child + 1, np.nan, dtype=np.float64)
    for row in condensed_tree:
        births[int(row['child'])] = row['value']
    births[smallest_cluster] = 0.0
    result = np.zeros(num_clusters, dtype=np.float64)
    for row in condensed_tree:
        parent = int(row['parent'])
        result[parent - smallest_cluster] += (row['value'] - births[parent]) * row['cluster_size']
    return {idx + smallest_cluster: result[idx] for idx in range(num_clusters)}


def bfs_from_cluster_tree(cluster_tree, bfs_root):
    result = []
    process_queue = np.array([bfs_root], dtype=np.intp)
    children = cluster_tree['child']
    parents = cluster_tree['parent']
    while len(process_queue) > 0:
        result.extend(process_queue.tolist())
        process_queue = children[np.isin(parents, process_queue)]
    return result


def recurse_leaf_dfs(cluster_tree, current_node):
    children = cluster_tree[cluster_tree['parent'] == current_node]['child']
    if children.shape[0] == 0:
        return [current_node]
    return sum([recurse_leaf_dfs(cluster_tree, int(child)) for child in children], [])


def get_cluster_tree_leaves(cluster_tree):
    if cluster_tree.shape[0] == 0:
        return []
    root = int(cluster_tree['parent'].min())
    return recurse_leaf_dfs(cluster_tree, root)


def traverse_upwards(cluster_tree, cluster_selection_epsilon, leaf, allow_single_cluster):
    root = int(cluster_tree['parent'].min())
    parent = int(cluster_tree[cluster_tree['child'] == leaf]['parent'][0])
    if parent == root:
        if allow_single_cluster:
            return parent
        return leaf
    parent_eps = 1 / cluster_tree[cluster_tree['child'] == parent]['value'][0]
    if parent_eps > cluster_selection_epsilon:
        return parent
    return traverse_upwards(cluster_tree, cluster_selection_epsilon, parent, allow_single_cluster)


def epsilon_search(leaves, cluster_tree, cluster_selection_epsilon, allow_single_cluster):
    selected_clusters = []
    processed = []
    children = cluster_tree['child']
    distances = cluster_tree['value']
    for leaf in leaves:
        leaf_nodes = children == leaf
        eps = 1 / distances[leaf_nodes][0]
        if eps < cluster_selection_epsilon:
            if leaf not in processed:
                epsilon_child = traverse_upwards(cluster_tree, cluster_selection_epsilon, leaf,
                                                 allow_single_cluster)
                selected_clusters.append(epsilon_child)
                for sub_node in bfs_from_cluster_tree(cluster_tree, epsilon_child):
                    if sub_node != epsilon_child:
                        processed.append(sub_node)
        else:
            selected_clusters.append(leaf)
    return set(selected_clusters)


def get_clusters_labels(condensed_tree, stability, cluster_selection_method='eom', allow_single_cluster=False,
                        cluster_selection_epsilon=0.0, max_cluster_size=None):
    from sklearn.cluster._hdbscan._tree import _do_labelling
    if allow_single_cluster:
        node_list = sorted(stability.keys(), reverse=True)
    else:
        node_list = sorted(stability.keys(), reverse=True)[:-1]
    cluster_tree = condensed_tree[condensed_tree['cluster_size'] > 1]
    is_cluster = {cluster: True for cluster in node_list}
    n_samples = int(np.max(condensed_tree[condensed_tree['cluster_size'] == 1]['child'])) + 1
    if max_cluster_size is None:
        max_cluster_size = n_samples + 1
    cluster_sizes = {int(child): int(cluster_size) for child, cluster_size
                     in zip(cluster_tree['child'], cluster_tree['cluster_size'])}
    if allow_single_cluster:
        cluster_sizes[node_list[-1]] = int(np.sum(cluster_tree[cluster_tree['parent'] == node_list[-1]]
                                                  ['cluster_size']))
    if cluster_selection_method == 'eom':
        for node in node_list:
            child_selection = (cluster_tree['parent'] == node)
            subtree_stability = np.sum([stability[int(child)] for child in cluster_tree['child'][child_selection]])
            if subtree_stability > stability[node] or cluster_sizes[node] > max_cluster_size:
                is_cluster[node] = False
                stability[node] = subtree_stability
            else:
                for sub_node in bfs_from_cluster_tree(cluster_tree, node):
                    if sub_node != node:
                        is_cluster[sub_node] = False
        if cluster_selection_epsilon != 0.0 and cluster_tree.shape[0] > 0:
            eom_clusters = [c for c in is_cluster if is_cluster[c]]
            selected_clusters = []
            if len(eom_clusters) == 1 and eom_clusters[0] == cluster_tree['parent'].min():
                if allow_single_cluster:
                    selected_clusters = eom_clusters
            else:
                selected_clusters = epsilon_search(set(eom_clusters), cluster_tree, cluster_selection_epsilon,
                                                   allow_single_cluster)
            for c in is_cluster:
                is_cluster[c] = c in selected_clusters
    elif cluster_selection_method == 'leaf':
        leaves = set(get_cluster_tree_leaves(cluster_tree))
        if len(leaves) == 0:
            for c in is_cluster:
                is_cluster[c] = False
            is_cluster[condensed_tree['parent'].min()] = True
        if cluster_selection_epsilon != 0.0:
            selected_clusters = epsilon_search(leaves, cluster_tree, cluster_selection_epsilon,
                                               allow_single_cluster)
        else:
            selected_clusters = leaves
        for c in is_cluster:
            is_cluster[c] = c in selected_clusters
    clusters = {c for c in is_cluster if is_cluster[c]}
    cluster_map = {c: n for n, c in enumerate(sorted(list(clusters)))}
    labels = _do_labelling(condensed_tree, clusters, cluster_map, allow_single_cluster, cluster_selection_epsilon)
    return np.asarray(labels, dtype=np.int64), clusters


def tree_to_labels(tree, min_cluster_size, method='eom', allow_single_cluster=False, epsilon=0.0,
                   max_cluster_size=None):
    """Condensation par le code de sklearn (_condense_tree, cpdef), selection transcrite, etiquetage de sklearn."""
    from sklearn.cluster._hdbscan._tree import _condense_tree
    tree = np.ascontiguousarray(tree)
    condensed = _condense_tree(tree, min_cluster_size)
    stab = compute_stability(condensed)
    labels, _clusters = get_clusters_labels(condensed, stab, method, allow_single_cluster, epsilon,
                                            max_cluster_size)
    return labels
