#!/usr/bin/env python3
"""Modele abstrait de l'index G-c et de sa file16. Aucun C++, chrono ou nuage prive.
Le hash du modele est volontairement simple : le masque0 force la collision totale.
"""
import json


def need(value, message):
    if not value:
        raise RuntimeError(message)


class Index:
    def __init__(self, rows, mask):
        self.mask = mask
        self.data = sorted((self.key(row), row, birth, rank) for row,birth,rank in rows)
        need(all(a[:2] < b[:2] for a,b in zip(self.data,self.data[1:])), 'duplicate population')
        bits = (len(rows)-1).bit_length() if len(rows)>1 else 0
        self.shift = 64-bits
        self.directory = []
        pos = 0
        for bucket in range((1 << bits)+1):
            while pos<len(rows) and self.bucket(self.data[pos][0])<bucket:
                pos += 1
            self.directory.append(pos)

    def key(self, row):
        return (sum((x*0x9e3779b97f4a7c15) & ((1<<64)-1) for x in row) & ((1<<64)-1)) & self.mask

    def bucket(self, key):
        return key >> self.shift if self.shift < 64 else 0

    def lower(self, lo, hi, key, row=None):
        steps = 0
        while lo<hi:
            mid = lo+(hi-lo)//2
            at = self.data[mid]
            below = at[0]<key if at[0]!=key else row is not None and at[1]<row
            if below:
                lo=mid+1
            else:
                hi=mid
            steps += 1
        return lo,steps

    def candidate(self, key):
        bucket=self.bucket(key)
        end=self.directory[bucket+1]
        pos,steps=self.lower(self.directory[bucket],end,key)
        return (pos if pos<end and self.data[pos][0]==key else None),steps

    def find(self, row):
        key=self.key(row)
        bucket=self.bucket(key)
        end=self.directory[bucket+1]
        pos,steps=self.lower(self.directory[bucket],end,key,row)
        return (self.data[pos][2:] if pos<end and self.data[pos][:2]==(key,row) else None),steps

    def verify(self, candidate, row):
        if candidate is None:
            return None,0
        at=self.data[candidate]
        if at[1]==row:
            return at[2:],0
        end=self.directory[self.bucket(at[0])+1]
        pos,steps=self.lower(candidate+1,end,at[0],row)
        return (self.data[pos][2:] if pos<end and self.data[pos][:2]==(at[0],row) else None),steps


def main():
    runs=[]
    total=0
    for count in (0,1,2,17,64,257):
        rows=[((0,i+1,i+513),1000+3*i,7+i) for i in range(count)]
        expected={r:(birth,rank) for r,birth,rank in rows}
        queries=[r for r,_,_ in rows]*2+[(0,i+1,i+514) for i in range(count+3)]
        for mask in (0,3,(1<<64)-1):
            index=Index(list(reversed(rows)),mask)
            maxima=[0,0,0]
            for row in queries:
                direct,s0=index.find(row)
                candidate,s1=index.candidate(index.key(row))
                checked,s2=index.verify(candidate,row)
                need(direct==checked==expected.get(row), 'wrong association')
                maxima=[max(x,y) for x,y in zip(maxima,(s0,s1,s2))]
                need(max(s0,s1,s2)<=count.bit_length(), 'binary bound exceeded')
            runs.append(dict(entries=count,mask=str(mask),queries=len(queries),
                             max_binary_steps=dict(zip(('find','candidate','verify'),maxima))))
            total += len(queries)
    refused=0
    for mask in (0,3,(1<<64)-1):
        try:
            Index([((0,1,2),7,3),((0,1,2),19,9)],mask)
        except RuntimeError as error:
            need(str(error)=='duplicate population', 'wrong refusal')
            refused+=1
    need(refused==3,'duplicate masked')
    # Meme ordre FIFO que passes.cpp, y compris le remplissage initial et la vidange finale.
    sizes=(0,1,7,8,15,16,17,31,32,33,257)
    for count in sizes:
        ring=[None]*16
        incoming=outgoing=0
        completed=[]
        for rep in range(count):
            need(ring[incoming%16] is None,'overwritten representative')
            ring[incoming%16]=(rep,rep//3)
            if incoming>=8:
                need(ring[(incoming-8)%16] is not None,'prefetch consumed entry')
            incoming+=1
            if incoming-outgoing==16:
                completed.append(ring[outgoing%16])
                ring[outgoing%16]=None
                outgoing+=1
        while outgoing<incoming:
            completed.append(ring[outgoing%16])
            ring[outgoing%16]=None
            outgoing+=1
        need(completed==[(rep,rep//3) for rep in range(count)],'FIFO changed targets ownership')
    print(json.dumps(dict(native_executed=False,queries=total,index_runs=runs,
                          duplicate_refusals=refused,fifo_sizes=list(sizes)),indent=2,sort_keys=True))


if __name__=='__main__':
    main()
