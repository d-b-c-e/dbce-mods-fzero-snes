#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef FZERO_ORDINARY_R1
int FzeroOrdinaryOpen(const char *config);
int FzeroOrdinaryActive(void);
int FzeroOrdinaryAlive(void);
const char *FzeroOrdinaryRoot(void);
int FzeroOrdinaryConfigure(const char *config);
void FzeroOrdinaryInput(unsigned frame);
void FzeroOrdinaryObserve(unsigned frame,uint32_t word,const uint8_t *ram,size_t size,unsigned controllers);
int FzeroOrdinaryHostKey(unsigned frame);
int FzeroOrdinaryPanelKey(unsigned row);
void FzeroOrdinaryPanelState(int open,int show_fps,int saved);
void FzeroOrdinaryFinish(void);
#else
static inline int FzeroOrdinaryActive(void){return 0;}
static inline int FzeroOrdinaryAlive(void){return 1;}
static inline const char *FzeroOrdinaryRoot(void){return "";}
static inline int FzeroOrdinaryOpen(const char *config){(void)config;return 0;}
static inline int FzeroOrdinaryConfigure(const char *config){(void)config;return 0;}
static inline void FzeroOrdinaryInput(unsigned frame){(void)frame;}
static inline void FzeroOrdinaryObserve(unsigned f,uint32_t w,const uint8_t *r,size_t n,unsigned c){(void)f;(void)w;(void)r;(void)n;(void)c;}
static inline int FzeroOrdinaryHostKey(unsigned f){(void)f;return 0;}
static inline int FzeroOrdinaryPanelKey(unsigned row){(void)row;return 0;}
static inline void FzeroOrdinaryPanelState(int o,int f,int s){(void)o;(void)f;(void)s;}
static inline void FzeroOrdinaryFinish(void){}
#endif
