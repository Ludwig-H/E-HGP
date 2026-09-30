class ExactResolverMethods:
    def pair_level(self, a, b):
        return Fraction(sq_dist(self.sites[a], self.sites[b]), 4)

    def third_sites(self, a, b):
        """Sites de la boule fermee de diametre [a, b], autres que a et b."""
        A, B = self.sites[a], self.sites[b]
        S = (A[0] + B[0], A[1] + B[1], A[2] + B[2])  # 2 m
        d2 = sq_dist(A, B)
        r = math.isqrt(d2) // 2 + 1
        lo = [S[i] // 2 - r - 1 for i in range(3)]
        hi = [(S[i] + 1) // 2 + r + 1 for i in range(3)]
        self.census_calls += 1
        out = []
        for z in self.grid.candidates(lo, hi):
            if z == a or z == b:
                continue
            Z = self.sites[z]
            if sq_dist((2 * Z[0], 2 * Z[1], 2 * Z[2]), S) <= d2:
                out.append(z)
        return out

    def resolve(self, a, b, keep=0):
        """Noeud vivant au niveau |a - b|^2 / 4 qui contient le milieu de {a, b}. `keep` choisit l'extremite gardee
        pendant la descente (0 : a, 1 : b) ; les deux chemins doivent donner le meme noeud (auto-controle)."""
        require(a != b, 'paire degeneree')
        key = (min(a, b), max(a, b))
        if keep == 0 and key in self.memo:
            return self.memo[key]
        path = []
        cur = (a, b) if keep == 0 else (b, a)
        while True:
            ck = (min(cur), max(cur))
            if keep == 0 and ck in self.memo:
                node = self.memo[ck]
                break
            third = self.third_sites(*cur)
            if not third:
                node = self.birth_of_empty_pair(*cur)
                self.births_reached += 1
                if keep == 0:
                    self.memo[ck] = node
                break
            x = cur[0]
            X = self.sites[x]
            z = min(third, key=lambda s: (sq_dist(X, self.sites[s]), s))
            path.append(cur)
            self.steps += 1
            cur = (x, z)
        for p in reversed(path):
            node = self.forest.ancestor(node, self.pair_level(*p), True)
            require(node is not None, 'descente : noeud posterieur au niveau de la paire')
            if keep == 0:
                self.memo[(min(p), max(p))] = node
        return node
