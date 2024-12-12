#include <stdlib.h>
#include <extension.h>

__attribute__((weak, export_name("canonical_abi_realloc")))
void *canonical_abi_realloc(
void *ptr,
size_t orig_size,
size_t org_align,
size_t new_size
) {
  void *ret = realloc(ptr, new_size);
  if (!ret)
  abort();
  return ret;
}

__attribute__((weak, export_name("canonical_abi_free")))
void canonical_abi_free(
void *ptr,
size_t size,
size_t align
) {
  free(ptr);
}
#include <string.h>

void extension_string_set(extension_string_t *ret, const char *s) {
  ret->ptr = (char*) s;
  ret->len = strlen(s);
}

void extension_string_dup(extension_string_t *ret, const char *s) {
  ret->len = strlen(s);
  ret->ptr = reinterpret_cast<char *>(canonical_abi_realloc(NULL, 0, 1, ret->len));
  memcpy(ret->ptr, s, ret->len);
}

void extension_string_free(extension_string_t *ret) {
  canonical_abi_free(ret->ptr, ret->len, 1);
  ret->ptr = NULL;
  ret->len = 0;
}
void extension_list_u8_free(extension_list_u8_t *ptr) {
  canonical_abi_free(ptr->ptr, ptr->len * 1, 1);
}

__attribute__((aligned(4)))
static uint8_t RET_AREA[8];
__attribute__((export_name("hll-cardinality")))
double __wasm_export_extension_hll_cardinality(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  double ret = extension_hll_cardinality(&arg1);
  return ret;
}
__attribute__((export_name("hll-cardinality-emptyisnull")))
double __wasm_export_extension_hll_cardinality_emptyisnull(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  double ret = extension_hll_cardinality_emptyisnull(&arg1);
  return ret;
}
__attribute__((export_name("hll-union")))
int32_t __wasm_export_extension_hll_union(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_hll_union(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("hll-union-emptyisnull")))
int32_t __wasm_export_extension_hll_union_emptyisnull(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_hll_union_emptyisnull(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("hll-hash")))
int64_t __wasm_export_extension_hll_hash(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  uint64_t ret = extension_hll_hash(&arg1);
  return (int64_t) (ret);
}
__attribute__((export_name("hll-hash-emptyisnull")))
int64_t __wasm_export_extension_hll_hash_emptyisnull(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  uint64_t ret = extension_hll_hash_emptyisnull(&arg1);
  return (int64_t) (ret);
}
__attribute__((export_name("hll-print")))
int32_t __wasm_export_extension_hll_print(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_string_t ret;
  extension_hll_print(&arg1, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("hll-print-emptyisnull")))
int32_t __wasm_export_extension_hll_print_emptyisnull(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_string_t ret;
  extension_hll_print_emptyisnull(&arg1, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("hll-empty")))
int32_t __wasm_export_extension_hll_empty(void) {
  extension_state_t ret = extension_hll_empty();
  return ret;
}
__attribute__((export_name("hll-add")))
int32_t __wasm_export_extension_hll_add(int32_t arg, int32_t arg0, int32_t arg1) {
  extension_list_u8_t arg2 = (extension_list_u8_t) { (uint8_t*)(arg0), (size_t)(arg1) };
  extension_state_t ret = extension_hll_add(arg, &arg2);
  return ret;
}
__attribute__((export_name("hll-add-emptyisnull")))
int32_t __wasm_export_extension_hll_add_emptyisnull(int32_t arg, int32_t arg0, int32_t arg1) {
  extension_list_u8_t arg2 = (extension_list_u8_t) { (uint8_t*)(arg0), (size_t)(arg1) };
  extension_state_t ret = extension_hll_add_emptyisnull(arg, &arg2);
  return ret;
}
__attribute__((export_name("hll-add-hash")))
int32_t __wasm_export_extension_hll_add_hash(int32_t arg, int64_t arg0) {
  extension_state_t ret = extension_hll_add_hash(arg, (uint64_t) (arg0));
  return ret;
}
__attribute__((export_name("hll-add-hash-emptyisnull")))
int32_t __wasm_export_extension_hll_add_hash_emptyisnull(int32_t arg, int64_t arg0) {
  extension_state_t ret = extension_hll_add_hash_emptyisnull(arg, (uint64_t) (arg0));
  return ret;
}
__attribute__((export_name("hll-union-agg")))
int32_t __wasm_export_extension_hll_union_agg(int32_t arg, int32_t arg0, int32_t arg1) {
  extension_list_u8_t arg2 = (extension_list_u8_t) { (uint8_t*)(arg0), (size_t)(arg1) };
  extension_state_t ret = extension_hll_union_agg(arg, &arg2);
  return ret;
}
__attribute__((export_name("hll-union-agg-emptyisnull")))
int32_t __wasm_export_extension_hll_union_agg_emptyisnull(int32_t arg, int32_t arg0, int32_t arg1) {
  extension_list_u8_t arg2 = (extension_list_u8_t) { (uint8_t*)(arg0), (size_t)(arg1) };
  extension_state_t ret = extension_hll_union_agg_emptyisnull(arg, &arg2);
  return ret;
}
__attribute__((export_name("hll-union-merge")))
int32_t __wasm_export_extension_hll_union_merge(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_hll_union_merge(arg, arg0);
  return ret;
}
__attribute__((export_name("hll-serialize")))
int32_t __wasm_export_extension_hll_serialize(int32_t arg) {
  extension_list_u8_t ret;
  extension_hll_serialize(arg, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("hll-serialize-compact")))
int32_t __wasm_export_extension_hll_serialize_compact(int32_t arg) {
  extension_list_u8_t ret;
  extension_hll_serialize_compact(arg, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("hll-deserialize")))
int32_t __wasm_export_extension_hll_deserialize(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t ret = extension_hll_deserialize(&arg1);
  return ret;
}
__attribute__((export_name("hll-to-dense")))
int32_t __wasm_export_extension_hll_to_dense(int32_t arg) {
  extension_state_t ret = extension_hll_to_dense(arg);
  return ret;
}
__attribute__((export_name("hll-is-dense")))
int32_t __wasm_export_extension_hll_is_dense(int32_t arg) {
  uint32_t ret = extension_hll_is_dense(arg);
  return (int32_t) (ret);
}
__attribute__((export_name("hll-is-sparse")))
int32_t __wasm_export_extension_hll_is_sparse(int32_t arg) {
  uint32_t ret = extension_hll_is_sparse(arg);
  return (int32_t) (ret);
}
