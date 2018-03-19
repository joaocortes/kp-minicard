/****************************************************************************[Encodings_MW.h]
Copyright (c) 2016-2017, Michal Karpinski, Marek Piotrow

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef __Encodings_TOT_h
#define __Encodings_TOT_h

template <class Solver>
class Encoding_TOT {
 private:
  bool makeModuloTotalizer(const vector<Lit>& invars, vector<Lit>& outvars, unsigned k);

  Solver* S;

 public:
  bool makeModuloTotalizer(const vector<Lit>& invars, vector<Lit>& outvars, unsigned const k);

  Encoding_TOT(Solver* _S) : S(_S) { }
  ~Encoding_TOT() { }
};

template<class Solver>
bool Encoding_TOT<Solver>::makeModuloTotalizer(const vector<Lit>& invars, vector<Lit>& outvars, unsigned const k) {
 
  vec<Lit> lits;
  int lcnt = 0; // loop count
  vec<Lit> linkingVar;
  bool mmodel[nbvar]; // koshi 2013.07.05
  long long int divisor = 1; // koshi 2013.10.04

  vec<long long int> cc; // cardinality constraints
  cc.clear();
        vec<Lit> dummy;

  // koshi 20140701        lbool ret = S.solveLimited(dummy);
  lbool ret;

  while ((ret = S.solveLimited(dummy)) == l_True) { // koshi 20140107
    lcnt++;
    long long int answerNew = 0;

    for (int i = 0; i < blockings.size(); i++) {
      int varnum = var(blockings[i]);
      if (sign(blockings[i])) {
        if (S.model[varnum] == l_False) {
          answerNew += weights[i];
        }
      } else {
        if (S.model[varnum] == l_True) {
          answerNew += weights[i];
        }
      }
    }

    if (lcnt == 1) { // first model: generate cardinal constraints
      genCardinals(card, comp, weights,blockings, answer,answerNew,divisor, S, lits, linkingVar);
    }
    
    if (answerNew > 0) {
      lessthan(card, linkingVar, answer,answerNew,divisor, cc, S, lits);
      answer = answerNew;
    } else {
      answer = answerNew;
      ret = l_False; // koshi 20140124
      break;
    }
  } // end of while


  return true;
}

// koshi 2013.04.05, 2013.05.21, 2013.06.28, 2013.07.01, 2013.10.04
// koshi 20140121
void genCardinals(int& card, int comp,
      vec<long long int>& weights, vec<Lit>& blockings, 
      long long int max, long long int k, 
      long long int& divisor, // koshi 2013.10.04
      Solver& S, vec<Lit>& lits, vec<Lit>& linkingVar) {
  assert(weights.size() == blockings.size());

  vec<long long int> sweights;
  vec<Lit> sblockings;
  wbSort(weights,blockings, sweights,sblockings);
  wbFilter(k,S,lits, sweights,sblockings, weights,blockings);
  long long int sum = sumWeight(weights); // koshi 20140124
 
  genOgawa0(card, weights, blockings, max, k, divisor, comp, S, lits, linkingVar);

  sweights.clear(); sblockings.clear();
}

// koshi 13.04.05, 13.06.28, 13.07.01, 13.10.04
void lessthan(int card, vec<Lit>& linking, long long int ok, long long int k,
              long long int divisor, // koshi 13.10.04
              vec<long long int>& cc, Solver& S, vec<Lit>& lits) {
  long long int upper = (k-1)/divisor;
  long long int lower = k%divisor;
  long long int oupper = ok/divisor;

  if (upper < oupper) {
    for (long long int i = divisor+upper+1; i < divisor+oupper+1; i++) {
      if (linking.size() <= i) break;
      else {
        lits.clear();
        lits.push(~linking[i]);
        S.addClause_(lits);
      }
    }
  }
  upper = k/divisor;

  lits.clear();
  lits.push(~linking[divisor+upper]);
  lits.push(~linking[lower]);

  S.addClause_(lits);
}

/*
  Cardinaltiy Constraints:
  Toru Ogawa, YangYang Liu, Ryuzo Hasegawa, Miyuki Koshimura, Hiroshi Fujita,
  "Modulo Based CNF Encoding of Cardinality Constraints and Its Application to
   MaxSAT Solvers",
  ICTAI 2013.
 */
// koshi 2013.10.03
void genOgawa(long long int weightX, vec<Lit>& linkingX,
        long long int weightY, vec<Lit>& linkingY,
        long long int& total, long long int divisor,
        Lit zero, Lit one, int comp,Solver& S, 
        vec<Lit>& lits, vec<Lit>& linkingVar, long long int UB) {

  total = weightX+weightY;
  if (weightX == 0) 
    for(int i = 0; i < linkingY.size(); i++) linkingVar.push(linkingY[i]);
  else if (weightY == 0)
    for(int i = 0; i < linkingX.size(); i++) linkingVar.push(linkingX[i]);
  else {
    long long int upper= total/divisor;
    long long int divisor1=divisor-1;
    /*
    printf("weightX = %lld, linkingX.size() = %d ", weightX,linkingX.size());
    printf("weightY = %lld, linkingY.size() = %d\n", weightY,linkingY.size());
    printf("upper = %lld, divisor1 = %lld\n", upper,divisor1);
    */

    linkingVar.push(one);
    for (int i = 0; i < divisor1; i++) linkingVar.push(mkLit(S.newVar()));
    linkingVar.push(one);
    for (int i = 0; i < upper; i++) linkingVar.push(mkLit(S.newVar()));
    Lit carry = mkLit(S.newVar());

    // lower part
    for (int i = 0; i < divisor; i++)
      for (int j = 0; j < divisor; j++) {
  int ij = i+j;
  lits.clear();
  lits.push(~linkingX[i]);
  lits.push(~linkingY[j]);
  if (ij < divisor) {
    lits.push(linkingVar[ij]);
    lits.push(carry);
  } else if (ij == divisor) lits.push(carry);
  else if (ij > divisor) lits.push(linkingVar[ij%divisor]);
  S.addClause_(lits);
      }

    // upper part
    for (int i = divisor; i < linkingX.size(); i++)
      for (int j = divisor; j < linkingY.size(); j++) {
  int ij = i+j-divisor;
  lits.clear();
  lits.push(~linkingX[i]);
  lits.push(~linkingY[j]);
  if (ij < linkingVar.size()) lits.push(linkingVar[ij]);
  S.addClause_(lits);
  //  printf("ij = %lld, linkingVar.size() = %lld\n",ij,linkingVar.size());
  lits.clear();
  lits.push(~carry);
  lits.push(~linkingX[i]);
  lits.push(~linkingY[j]);
  if (ij+1 < linkingVar.size()) lits.push(linkingVar[ij+1]);
  S.addClause_(lits);
      }
  }
  linkingX.clear(); linkingY.clear();
}

void genOgawa(vec<long long int>& weights, vec<Lit>& blockings,
        long long int& total, long long int divisor,
        Lit zero, Lit one, int comp,Solver& S, 
        vec<Lit>& lits, vec<Lit>& linkingVar, long long int UB) {

  linkingVar.clear();

  vec<Lit> linkingAlpha;
  vec<Lit> linkingBeta;

  if (total < divisor) {
    vec<Lit> linking;
    genBailleux(weights,blockings,total,
    zero,one, comp,S, lits, linking, UB);
    total = linking.size()-2;
    for(int i = 0; i < divisor; i++) 
      if (i < linking.size()) linkingVar.push(linking[i]);
      else linkingVar.push(zero);
    linkingVar.push(one);
    linking.clear();
    //    printf("total = %lld, linkngVar.size() = %d\n", total,linkingVar.size());
  } else if (blockings.size() == 1) {
    long long int weight = weights[0];
    if (weight < UB) {
      long long int upper = weight/divisor; 
      long long int lower = weight%divisor;
      long long int pad = divisor-lower-1;
      linkingVar.push(one);
      for (int i = 0; i < lower; i++) linkingVar.push(blockings[0]);
      for (int i = 0; i < pad; i++) linkingVar.push(zero);
      linkingVar.push(one);
      for (int i = 0; i < upper; i++) linkingVar.push(blockings[0]);
      total = weight;
    } else {
      lits.clear();
      lits.push(~blockings[0]);
      S.addClause_(lits);
      total = 0;
    }
  } else if (blockings.size() > 1) {
    long long int weightL = 0; long long int weightR = 0;
    vec<long long int> weightsL, weightsR;
    vec<Lit> blockingsL, blockingsR;
    long long int half = total/2;
    wbsplit(half, weightL,weightR, weights,blockings,
      weightsL,blockingsL, weightsR,blockingsR);

    genOgawa(weightsL,blockingsL,weightL,divisor,
       zero,one, comp,S, lits, linkingAlpha, UB);
    genOgawa(weightsR,blockingsR,weightR,divisor,
       zero,one, comp,S, lits, linkingBeta, UB);
    
    weightsL.clear();blockingsL.clear();
    weightsR.clear();blockingsR.clear();

    genOgawa(weightL,linkingAlpha, weightR,linkingBeta, total,divisor,
        zero,one, comp,S, lits, linkingVar, UB);
  }
  // koshi 2013.11.12
  long long int upper = (UB-1)/divisor;
  for (long long int i = divisor+upper+1; i < linkingVar.size(); i++) {
    lits.clear();
    lits.push(~linkingVar[i]);
    S.addClause_(lits);
  }
  while (divisor+upper+2 < linkingVar.size()) linkingVar.shrink(1);
}

void genOgawa0(int& card, // koshi 2013.12.24
         vec<long long int>& weights, vec<Lit>& blockings,
         long long int max, long long int k,
         long long int& divisor, int comp, Solver& S,
         vec<Lit>& lits, vec<Lit>& linkingVar) {

  long long int k0 = k;
  long long int odd = 1;
  divisor = 0;

  while (k0 > 0) {
    divisor++;
    k0 -= odd;
    odd += 2;
  }
  printf("c max = %lld, divisor = %lld\n", max,divisor);

  // koshi 2013.12.24
  if (divisor <= 2) {
    printf("c divisor is less than or equal to 2 ");
    printf("so we use Warner's encoding, i.e. -card=warn\n");
    card = 0;
    genWarners0(weights,blockings, max,k, comp, S, lits,linkingVar);
  } else {
    // koshi 20140109
    printf("c Ogawa's encoding for Cardinality Constraints\n");

    Lit one = mkLit(S.newVar());
    lits.clear();
    lits.push(one);
    S.addClause_(lits);
    genOgawa(weights,blockings, max,divisor, ~one, one, comp,S, lits, linkingVar, k);
  }
}


#endif // __Encodings_TOT_h



/*
  Cardinality Constraints:
  Joost P. Warners, "A linear-time transformation of linear inequalities
  into conjunctive normal form", 
  Information Processing Letters 68 (1998) 63-69
 */

// koshi 2013.04.16
void genWarnersHalf(Lit& a, Lit& b, Lit& carry, Lit& sum, int comp,
           Solver& S, vec<Lit>& lits) {
  // carry
  lits.clear();
  lits.push(~a); lits.push(~b); lits.push(carry);  S.addClause_(lits);
  // sum
  lits.clear();
  lits.push(a); lits.push(~b); lits.push(sum);  S.addClause_(lits);
  lits.clear();
  lits.push(~a); lits.push(b); lits.push(sum);  S.addClause_(lits);
  //
  if (comp == 1 || comp == 2) {
    lits.clear();
    lits.push(carry); lits.push(sum); lits.push(~a); S.addClause_(lits);
    lits.clear();
    lits.push(carry); lits.push(sum); lits.push(~b); S.addClause_(lits);
  }
  if (comp == 2) {
    lits.clear();
    lits.push(~carry); lits.push(~sum); S.addClause_(lits);
    lits.clear();
    lits.push(~carry); lits.push(sum); lits.push(a); S.addClause_(lits);
    lits.clear();
    lits.push(~carry); lits.push(sum); lits.push(b); S.addClause_(lits);
  }
  // koshi 2013.05.31
  if (comp == 10 || comp == 11) { // [Warners 1996]
    // carry
    lits.clear(); lits.push(a); lits.push(~carry); S.addClause_(lits);
    lits.clear(); lits.push(b); lits.push(~carry); S.addClause_(lits);
    // sum
    lits.clear();
    lits.push(~a); lits.push(~b); lits.push(~sum);  S.addClause_(lits);
    lits.clear();
    lits.push(a); lits.push(b); lits.push(~sum);  S.addClause_(lits);
  }
}

// koshi 2013.04.16
void genWarnersFull(Lit& a, Lit& b, Lit& c, Lit& carry, Lit& sum, int comp,
           Solver& S, vec<Lit>& lits) {
  // carry
  lits.clear();
  lits.push(~a); lits.push(~b); lits.push(carry); S.addClause_(lits);
  lits.clear();
  lits.push(~a); lits.push(~c); lits.push(carry); S.addClause_(lits);
  lits.clear();
  lits.push(~b); lits.push(~c); lits.push(carry); S.addClause_(lits);
  // sum
  lits.clear();
  lits.push(a); lits.push(b); lits.push(~c); lits.push(sum);
  S.addClause_(lits);
  lits.clear();
  lits.push(a); lits.push(~b); lits.push(c); lits.push(sum);
  S.addClause_(lits);
  lits.clear();
  lits.push(~a); lits.push(b); lits.push(c); lits.push(sum);
  S.addClause_(lits);
  lits.clear();
  lits.push(~a); lits.push(~b); lits.push(~c); lits.push(sum);
  S.addClause_(lits);
  if (comp == 1 || comp == 2) {
    lits.clear();
    lits.push(carry); lits.push(sum); lits.push(~a); S.addClause_(lits);
    lits.clear();
    lits.push(carry); lits.push(sum); lits.push(~b); S.addClause_(lits);
    lits.clear();
    lits.push(carry); lits.push(sum); lits.push(~c); S.addClause_(lits);
  }
  if (comp == 2) {
    lits.clear();
    lits.push(~carry); lits.push(~sum); lits.push(a); S.addClause_(lits);
    lits.clear();
    lits.push(~carry); lits.push(~sum); lits.push(b); S.addClause_(lits);
    lits.clear();
    lits.push(~carry); lits.push(~sum); lits.push(c); S.addClause_(lits);
  }
  // koshi 2013.05.31
  if (comp == 10 || comp == 11) {// [Warners 1996]
    // carry
    lits.clear();
    lits.push(a); lits.push(b); lits.push(~carry); S.addClause_(lits);
    lits.clear();
    lits.push(a); lits.push(c); lits.push(~carry); S.addClause_(lits);
    lits.clear();
    lits.push(b); lits.push(c); lits.push(~carry); S.addClause_(lits);
    // sum
    lits.clear();
    lits.push(a); lits.push(b); lits.push(c); lits.push(~sum);
    S.addClause_(lits);
    lits.clear();
    lits.push(~a); lits.push(~b); lits.push(c); lits.push(~sum);
    S.addClause_(lits);
    lits.clear();
    lits.push(~a); lits.push(b); lits.push(~c); lits.push(~sum);
    S.addClause_(lits);
    lits.clear();
    lits.push(a); lits.push(~b); lits.push(~c); lits.push(~sum);
    S.addClause_(lits);
  }
}



// koshi 2013.03.25 
// Parallel counter
// koshi 2013.04.16, 2013.05.23
void genWarners(vec<long long int>& weights, vec<Lit>& blockings,
    long long int max, int k, 
    int comp, Solver& S, const Lit zero,
    vec<Lit>& lits, vec<Lit>& linkingVar) {

  linkingVar.clear();
  bool dvar = (comp == 11) ? false : true;

  if (weights.size() == 1) {
    long long int weight = weights[0];
    vec<bool> pn;
    pn.clear(); 
    while (weight > 0) {
      if (weight%2 == 0) pn.push(false);
      else pn.push(true);
      weight /= 2;
    }
    for(int i = 0; i < pn.size(); i++) {
      if (pn[i]) linkingVar.push(blockings[0]);
      else linkingVar.push(zero);
    }
    pn.clear();
  } else if (weights.size() > 1) {
    long long int weightL = 0; long long int weightR = 0;
    vec<long long int> weightsL, weightsR;
    vec<Lit> blockingsL, blockingsR;

    long long int half = max/2;
    wbsplit(half,weightL,weightR, weights,blockings,
      weightsL,blockingsL, weightsR,blockingsR);

    vec<Lit> alpha;
    vec<Lit> beta;
    Lit sum = mkLit(S.newVar(true,dvar));
    Lit carry = mkLit(S.newVar(true,dvar));
    genWarners(weightsL, blockingsL, weightL,k, comp, S, zero, lits,alpha);
    genWarners(weightsR, blockingsR, weightR,k, comp, S, zero, lits,beta);
    weightsL.clear(); weightsR.clear();
    blockingsL.clear(); blockingsR.clear();

    bool lessthan = (alpha.size() < beta.size());
    vec<Lit> &smalls = lessthan ? alpha : beta;
    vec<Lit> &larges = lessthan ? beta : alpha;
    assert(smalls.size() <= larges.size());

    genWarnersHalf(smalls[0],larges[0], carry,sum, comp, S,lits);
    linkingVar.push(sum);

    int i = 1;
    Lit carryN;
    for(; i < smalls.size(); i++) {
      sum = mkLit(S.newVar(true,dvar));
      carryN = mkLit(S.newVar(true,dvar));
      genWarnersFull(smalls[i],larges[i],carry, carryN,sum, comp, S,lits);
      linkingVar.push(sum);
      carry = carryN;
    }
    for(; i < larges.size(); i++) {
      sum = mkLit(S.newVar(true,dvar));
      carryN = mkLit(S.newVar(true,dvar));
      genWarnersHalf(larges[i],carry, carryN,sum, comp, S,lits);
      linkingVar.push(sum);
      carry = carryN;
    }
    linkingVar.push(carry);
    alpha.clear();beta.clear();
  }
  int lsize = linkingVar.size();
  for (int i = k; i < lsize; i++) { // koshi 2013.05.27
    //    printf("shrink: k = %d, lsize = %d\n",k,lsize);
    lits.clear();
    lits.push(~linkingVar[i]);
    S.addClause_(lits);
  }
  for (int i = k; i < lsize; i++) linkingVar.shrink(1); // koshi 2013.05.27
}

// koshi 2013.06.28
void genWarners0(vec<long long int>& weights, vec<Lit>& blockings,
     long long int max,long long int k, int comp, Solver& S,
      vec<Lit>& lits, vec<Lit>& linkingVar) {

  int logk = 1;
  while ((k >>= 1) > 0) logk++;
  Lit zero = mkLit(S.newVar());
  lits.clear();
  lits.push(~zero);
  S.addClause_(lits);
  genWarners(weights,blockings, max,logk, comp, S, zero,lits,linkingVar);
}

#define wbsplit(half,wL,wR, ws,bs, wsL,bsL, wsR,bsR) \
  wsL.clear(); bsL.clear(); wsR.clear(); bsR.clear(); \
  int ii = 0; \
  int wsSizeHalf = ws.size()/2; \
  for(; ii < wsSizeHalf; ii++) { \
    wsL.push(ws[ii]); \
    bsL.push(bs[ii]); \
    wL += ws[ii]; \
  } \
  for(; ii < ws.size(); ii++) { \
    wsR.push(ws[ii]); \
    bsR.push(bs[ii]); \
    wR += ws[ii]; \
  }
