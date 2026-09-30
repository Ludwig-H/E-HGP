"""Strict independent exact integer/Fraction judge for the isolated comparator."""
from collections import Counter
from fractions import Fraction as F
import json
from math import isfinite
from pathlib import Path
import sys

sys.dont_write_bytecode = True


def require(value,message):
    if not value:
        raise ValueError(message)


def load(text):
    def unique(pairs):
        out = {}
        for key,value in pairs:
            require(key not in out,'duplicate JSON key')
            out[key] = value
        return out
    def constant(value):
        raise ValueError('nonfinite JSON constant')
    def finite(value):
        number = float(value)
        require(isfinite(number),'nonfinite JSON float')
        return number
    return json.loads(text,object_pairs_hook=unique,parse_constant=constant,parse_float=finite)


def integer(words):
    require(type(words) is list and len(words)==8 and
            all(type(v) is int and 0<=v<2**64 for v in words),'product word schema')
    result = 0
    for word in words:
        result = result*2**64+word
    return result


def domain(n,d):
    if n>=2**266:
        return 'REFUSED_NUM_DOMAIN'
    if d>=2**200:
        return 'REFUSED_DEN_DOMAIN'
    if d==0:
        return 'REFUSED_ZERO_DEN'
    return None


def judge(requests,answers):
    require(type(requests) is list and type(answers) is list and
            len(requests)==len(answers)==2444,'panel completeness')
    counts = Counter()
    for index,(request,actual) in enumerate(zip(requests,answers)):
        require(type(request['id']) is int and request['id']==index and
                actual['op']==request['op'],'paired identity')
        counts[request['op']] += 1
        if request['op']=='C':
            expected = domain(request['n1'],request['d1'])
            side = 'A'
            if expected is None:
                expected = domain(request['n2'],request['d2'])
                side = 'B'
            if expected is not None:
                require(set(actual)=={'op','status','side'} and actual['status']==expected and
                        actual['side']==side,'wrong domain refusal at '+str(index))
                counts['domain_refusals'] += 1
                continue
            require(set(actual)=={'op','status','cmp','lhs','rhs'} and
                    actual['status']=='READY','comparison schema')
            require(integer(actual['lhs'])==request['n1']*request['d2'] and
                    integer(actual['rhs'])==request['n2']*request['d1'],
                    'cross product mismatch at '+str(index))
            a,b = F(request['n1'],request['d1']),F(request['n2'],request['d2'])
            sign = int(a>b)-int(a<b)
            require(type(actual['cmp']) is int and actual['cmp']==sign,
                    'wrong exact comparison at '+str(index))
            counts['sign_'+str(sign)] += 1
            if request['label']=='positive_support_levels':
                require(F(request['geometry_a']['radius'])==a and
                        F(request['geometry_b']['radius'])==b,'independent support oracle disagreement')
                counts['geometric_comparisons'] += 1
        else:
            require(request['op']=='M','unknown request operation')
            product = request['n']*request['d']
            high = product >> 512
            require(type(actual['high_word']) is int and actual['high_word']==high,
                    'lost high product word at '+str(index))
            if high:
                require(set(actual)=={'op','status','high_word'} and
                        actual['status']=='REFUSED_OVERFLOW','raw overflow refusal')
                counts['raw_overflow_refusals'] += 1
            else:
                require(set(actual)=={'op','status','high_word','product'} and
                        actual['status']=='READY' and integer(actual['product'])==product,
                        'raw product mismatch at '+str(index))
                counts['raw_products'] += 1
    require(counts['domain_refusals']==6 and counts['raw_overflow_refusals']==14 and
            counts['geometric_comparisons']==729 and counts['sign_0']>=250 and
            counts['sign_1']>=50 and counts['sign_-1']>=50,'nonvacuity floors')
    return dict(status='PASS',requests=len(requests),counts=dict(counts),
                scope='bounded exact comparator and raw checked product; no native level construction/sort/FULL')


if __name__=='__main__':
    try:
        requests = load(Path(sys.argv[1]).read_text())
        answers = [load(line) for line in Path(sys.argv[2]).read_text().splitlines()]
        print(json.dumps(judge(requests,answers),sort_keys=True))
    except (ValueError,KeyError,TypeError,IndexError,ZeroDivisionError) as error:
        print(json.dumps(dict(status='FAIL',error=str(error)),sort_keys=True))
        raise SystemExit(1)
