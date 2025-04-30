#include "mylib/common.hpp"
#include "mylib/graph.hpp"
#include "mylib/graphsimplify.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cassert>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

#include "tdzdd/DdSpec.hpp"
#include "tdzdd/DdStructure.hpp"
#include "tdzdd/DdEval.hpp"
#include "tdzdd/spec/FrontierBasedSearch.hpp"
#include "tdzdd/util/Graph.hpp"
#include "scaleth.hpp"

class ProbEval : public tdzdd::DdEval<ProbEval, double> {
private:
  std::vector<double> prob_list_;
  
public:
  ProbEval(const std::vector<double>& prob_list) : prob_list_(prob_list) {}
  
  void evalTerminal(double& p, bool one) const { p = one ? 1.0 : 0.0; }
  
  void evalNode(double& p, int level,
                tdzdd::DdValues<double, 2> const& values) const {
    double pc = prob_list_[prob_list_.size() - level];
    p = values.get(0) * (1 - pc) + values.get(1) * pc;
  }
};

class ProbEvalRev : public tdzdd::DdEval<ProbEvalRev, double> {
private:
  std::vector<double> prob_list_;
  
public:
  ProbEvalRev(const std::vector<double>& prob_list) : prob_list_(prob_list) {}
  
  void evalTerminal(double& p, bool one) const { p = one ? 0.0 : 1.0; }
  
  void evalNode(double& p, int level,
                tdzdd::DdValues<double, 2> const& values) const {
    double pc = prob_list_[prob_list_.size() - level];
    p = values.get(0) * (1 - pc) + values.get(1) * pc;
  }
};

void print_usage(char *fil){
  fprintf(stderr, "Usage: %s [graph_file] [probability_file] [server_file] [order_file] [mode] [value] <client_file>\n", fil);
}

int main(int argc, char **argv){
  if(argc < 7){
    fprintf(stderr, "ERROR: too few arguments.\n");
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  
  Graph G;
  int n, m;
  std::vector<double> pi;
  std::unordered_set<int> srcs;
  std::unordered_set<int> clts;
  
  {
    Graph H;
    if(!H.readfromFile(argv[1])){
      fprintf(stderr, "ERROR: reading graph file %s failed.\n", argv[1]);
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    
    n = H.numV();
    m = H.numE();
    
    std::vector<double> prob(m);
    pi.resize(m);
    {
      FILE *fp;
      if((fp = fopen(argv[2], "r")) == NULL){
        fprintf(stderr, "ERROR: reading probability file %s failed.\n", argv[2]);
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
      }
      
      for(size_t i=0; i<m; ++i){
        fscanf(fp, "%lf", &prob[i]);
      }
      fclose(fp);
    }
    
    if(!G.readfromFile(argv[4])){
      fprintf(stderr, "ERROR: reading order file %s failed.\n", argv[5]);
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    
    for(size_t i=0; i<m; ++i){
      pi[i] = prob[H.etovar(G.e[i].first, G.e[i].second)];
    }
  }
  {
    FILE *fp;
    if((fp = fopen(argv[3], "r")) == NULL){
      fprintf(stderr, "ERROR: reading source file %s failed.\n", argv[3]);
      exit(EXIT_FAILURE);
    }
    int src;
    while(fscanf(fp, "%d", &src) != EOF){
      srcs.emplace(src);
    }
    fclose(fp);
    if(argc >= 8){
      if((fp = fopen(argv[7], "r")) == NULL){
        fprintf(stderr, "ERROR: reading client file %s failed.\n", argv[6]);
        exit(EXIT_FAILURE);
      }
      int clt;
      while(fscanf(fp, "%d", &clt) != EOF){
        if(!srcs.count(clt)) clts.emplace(clt);
      }
      fclose(fp);
    }else{
      for(int v=1; v<=n; ++v){
        if(!srcs.count(v)) clts.emplace(v);
      }
    }
  }
  int mode = atoi(argv[5]);
  int k = 0;
  double thp;
  if(mode == 2){
    thp = atof(argv[6]);
    if(thp < 0.0 || thp > 1.0){
      fprintf(stderr, "ERROR: value must be between 0.0 and 1.0.\n");
      exit(EXIT_FAILURE);
    }
  }else{
    k = atoi(argv[6]);
    if(k <= 0){
      fprintf(stderr, "ERROR: value must be positive.\n");
      exit(EXIT_FAILURE);
    }
    if(k > n) k = n+1;
  }
  
  auto cstart = std::chrono::system_clock::now();
  
  G.buildFrontiers();
  double unrel = 0.0;
  
  if(mode == 0){
    if(clts.size() >= k){
      ScaleThreshold ST(G, srcs, clts, k);
      tdzdd::DdStructure<2> DD(ST);
      DD.useMultiProcessors(false);
      unrel = DD.evaluate(ProbEvalRev(pi));
    }
    printf("%.15lf\n", unrel);
  }else if(mode == 1){
    for(int kk = 1; kk <= k; ++kk){
      if(clts.size() >= kk){
        ScaleThreshold ST(G, srcs, clts, kk);
        tdzdd::DdStructure<2> DD(ST);
        DD.useMultiProcessors(false);
        unrel = DD.evaluate(ProbEvalRev(pi));
      }
      printf("%d %.15lf\n", kk, unrel);
    }
  }else{
    int kk = 0;
    do{
      ++kk;
      if(clts.size() >= kk){
        ScaleThreshold ST(G, srcs, clts, kk);
        tdzdd::DdStructure<2> DD(ST);
        DD.useMultiProcessors(false);
        unrel = DD.evaluate(ProbEvalRev(pi));
      }
      printf("%d %.15lf\n", kk, unrel);
    }while(unrel > thp);
  }
  
  auto cend = std::chrono::system_clock::now();
  double ctime = std::chrono::duration_cast<std::chrono::milliseconds>(cend-cstart).count();
  
  fprintf(stderr, "calc time: %.6lf ms\n", ctime);
  
  return 0;
}