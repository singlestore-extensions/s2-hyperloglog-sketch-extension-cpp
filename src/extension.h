#ifndef __BINDINGS_EXTENSION_H
#define __BINDINGS_EXTENSION_H
#ifdef __cplusplus
extern "C"
{
  #endif
  
  #include <stdint.h>
  #include <stdbool.h>
  
  typedef struct {
    char *ptr;
    size_t len;
  } extension_string_t;
  
  void extension_string_set(extension_string_t *ret, const char *s);
  void extension_string_dup(extension_string_t *ret, const char *s);
  void extension_string_free(extension_string_t *ret);
  typedef int32_t extension_state_t;
  typedef struct {
    uint8_t *ptr;
    size_t len;
  } extension_list_u8_t;
  void extension_list_u8_free(extension_list_u8_t *ptr);
  double extension_hll_cardinality(extension_list_u8_t *data);
  double extension_hll_cardinality_emptyisnull(extension_list_u8_t *data);
  void extension_hll_union(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  void extension_hll_union_emptyisnull(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  uint64_t extension_hll_hash(extension_list_u8_t *data);
  uint64_t extension_hll_hash_emptyisnull(extension_list_u8_t *data);
  void extension_hll_print(extension_list_u8_t *data, extension_string_t *ret0);
  void extension_hll_print_emptyisnull(extension_list_u8_t *data, extension_string_t *ret0);
  extension_state_t extension_hll_empty(void);
  extension_state_t extension_hll_add(extension_state_t state, extension_list_u8_t *input);
  extension_state_t extension_hll_add_emptyisnull(extension_state_t state, extension_list_u8_t *input);
  extension_state_t extension_hll_add_hash(extension_state_t state, uint64_t input);
  extension_state_t extension_hll_add_hash_emptyisnull(extension_state_t state, uint64_t input);
  extension_state_t extension_hll_union_agg(extension_state_t state, extension_list_u8_t *input);
  extension_state_t extension_hll_union_agg_emptyisnull(extension_state_t state, extension_list_u8_t *input);
  extension_state_t extension_hll_union_merge(extension_state_t left, extension_state_t right);
  void extension_hll_serialize(extension_state_t state, extension_list_u8_t *ret0);
  void extension_hll_serialize_compact(extension_state_t state, extension_list_u8_t *ret0);
  extension_state_t extension_hll_deserialize(extension_list_u8_t *data);
  extension_state_t extension_hll_to_dense(extension_state_t state);
  uint32_t extension_hll_is_dense(extension_state_t state);
  uint32_t extension_hll_is_sparse(extension_state_t state);
  #ifdef __cplusplus
}
#endif
#endif
