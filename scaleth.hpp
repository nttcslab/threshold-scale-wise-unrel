#ifndef REL_SCALETH_HPP
#define REL_SCALETH_HPP

#include "mylib/common.hpp"
#include "mylib/graph.hpp"

#include <array>
#include <vector>
#include <utility>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include "tdzdd/DdSpec.hpp"

using SpecBaseType = uint8_t;
constexpr SpecBaseType BASE_MAX = 255;

class ScaleThreshold: public tdzdd::PodArrayDdSpec<ScaleThreshold, SpecBaseType, 2>{
public:
  const Graph& G;
  std::unordered_set<int> svrs;
  std::unordered_set<int> clts;
  std::vector<std::vector<int>> svr_ent;
  std::vector<std::vector<int>> clt_ent;
  std::vector<std::vector<int>> not_ent;
  std::vector<int> sss;
  int k;
  int fsize;
  int m;
  int svrlast;
  
  /*
  array[0]: #(affordable leaves)
  array[1]: #(colors)
  array[2..fsize+1]        : comp (BASE_MAX for no counterpart)
  array[fsize+2..fsize*2+2]: vnum
  */
  
  ScaleThreshold(const Graph& _G, const std::unordered_set<int> _svrs, const std::unordered_set<int> _clts, int _k)
  : G(_G), svrs(_svrs), clts(_clts), k(_k)
  {
    //G.buildFrontiers();
    fsize = G.maxFroSize();
    m = G.numE();
    int cltcnt = 0;
    
    svr_ent.resize(m);
    clt_ent.resize(m);
    not_ent.resize(m);
    sss.resize(m);
    for(int i=0; i<m; ++i){
      for(const auto& pos : G.fent[i]){
        if(svrs.count(G.mfros[i][pos])){
          svr_ent[i].emplace_back(pos);
          svrlast = i;
        }else if(clts.count(G.mfros[i][pos])){
          clt_ent[i].emplace_back(pos);
          ++cltcnt;
        }
        else not_ent[i].emplace_back(pos);
      }
    }
    sss[0] = cltcnt - static_cast<int>(clt_ent[0].size());
    for(int i=1; i<m; ++i){
      sss[i] = sss[i-1] - static_cast<int>(clt_ent[i].size());
    }
    
    setArraySize(fsize*2+3);
  }
  
public:
  int getRoot(SpecBaseType* _state) const{
    _state[0] = static_cast<SpecBaseType>(k);
    _state[1] = 1;
    for(int i=2; i<fsize+2; ++i){
      _state[i] = BASE_MAX;
    }
    for(int i=fsize+2; i<fsize*2+3; ++i){
      _state[i] = 0;
    }
    return m;
  }
  
  int getChild(SpecBaseType* _state, int lvl, int val) const{
    int i = m - lvl;
    SpecBaseType* ptnum = &_state[0];
    SpecBaseType* pcnum = &_state[1];
    SpecBaseType* pcomp = &_state[2];
    SpecBaseType* pnumv = &_state[fsize+2];
    size_t ll = G.fros[i+1].size();
    
    for(const auto& pos : svr_ent[i]){
      pcomp[pos] = 0;
    }
    for(const auto& pos : clt_ent[i]){
      pcomp[pos] = *pcnum;
      pnumv[(*pcnum)++] = 1;
    }
    for(const auto& pos : not_ent[i]){
      pcomp[pos] = (*pcnum)++;
    }
    
    std::vector<SpecBaseType> renum(*pcnum, BASE_MAX);
    renum[0] = 0;
    SpecBaseType cat_from = pcomp[G.vpos[i].first];
    SpecBaseType cat_to   = pcomp[G.vpos[i].second];
    for(const auto& pos : G.flve[i]){
      pcomp[pos] = BASE_MAX;
    }
    SpecBaseType cc_new = 1;
    std::vector<SpecBaseType> prevnumv(fsize);
    memcpy(prevnumv.data(), pnumv, sizeof(SpecBaseType) * fsize);
    memset(pnumv, 0, sizeof(SpecBaseType) * fsize);
    
    if(!val){ // do not take edge
      for(size_t pos=0; pos<ll; ++pos){
        SpecBaseType* pval = &pcomp[pos];
        if(*pval == BASE_MAX) continue;
        if(renum[*pval] == BASE_MAX) renum[*pval] = cc_new++;
        *pval = renum[*pval];
      }
      int tn_lve = 0;
      for(SpecBaseType c=0; c<*pcnum; ++c){
        if(renum[c] != BASE_MAX) pnumv[renum[c]] = prevnumv[c];
        else                     tn_lve += static_cast<int>(prevnumv[c]);
      }
      if(static_cast<int>(*ptnum) <= tn_lve) return 0;
      *pcnum = cc_new;
      if(tn_lve > 0){
        *ptnum = *ptnum - static_cast<SpecBaseType>(tn_lve);
        for(SpecBaseType c=0; c<cc_new; ++c){
          pnumv[c] = std::min(pnumv[c], *ptnum);
        }
      }
    }else{    // take edge
      if(cat_from == 0) renum[cat_to]   = 0;
      if(cat_to   == 0) renum[cat_from] = 0;
      for(size_t pos=0; pos<ll; ++pos){
        SpecBaseType* pval = &pcomp[pos];
        if(*pval == BASE_MAX) continue;
        if(renum[*pval] == BASE_MAX){
          renum[*pval] = cc_new++;
          if(*pval == cat_from)    renum[cat_to]   = renum[*pval];
          else if(*pval == cat_to) renum[cat_from] = renum[*pval];
        }
        *pval = renum[*pval];
      }
      int tn_lve = 0;
      int tn_tot = 0;
      for(SpecBaseType c=0; c<*pcnum; ++c){
        if(renum[c] != 0){
          tn_tot += static_cast<int>(prevnumv[c]);
          if(renum[c] != BASE_MAX){
            int tmp = static_cast<int>(pnumv[renum[c]]) + static_cast<int>(prevnumv[c]);
            if(tmp > static_cast<int>(*ptnum)) pnumv[renum[c]] = *ptnum;
            else                               pnumv[renum[c]] = static_cast<SpecBaseType>(tmp);
          }else tn_lve += static_cast<int>(prevnumv[c]);
        }
      }
      if(static_cast<int>(*ptnum) <= tn_lve) return 0;
      if(sss[i] + tn_tot < static_cast<int>(*ptnum)){
        return -1;
      }
      *pcnum = cc_new;
      if(tn_lve > 0){
        *ptnum = *ptnum - static_cast<SpecBaseType>(tn_lve);
        for(SpecBaseType c=0; c<cc_new; ++c){
          pnumv[c] = std::min(pnumv[c], *ptnum);
        }
      }
    }
    if(i >= svrlast){
      bool prune = true;
      for(size_t pos=0; pos<ll; ++pos){
        if(pcomp[pos] == 0){
          prune = false;
          break;
        }
      }
      if(prune) return 0;
    }
    return lvl-1;
  }
};

#endif // REL_SCALETH_HPP
