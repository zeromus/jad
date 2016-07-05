#ifndef JADVAC_H_
#define JADVAC_H_

#ifdef  __cplusplus
extern "C" {
#endif

struct jadContext;
struct jadStream;
struct jadAllocator;

int jadvacOpenFile_cue(jadContext* jad, jadStream* stream, jadAllocator* allocator);

#ifdef  __cplusplus
}
#endif

#endif //JADVAC_H_
