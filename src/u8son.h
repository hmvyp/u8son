#ifndef u8son_h
#define u8son_h

#include <string.h>
#include "u8son_tk.h"

#ifndef u8son_MAX_LEVELS
# define u8son_MAX_LEVELS 50
#endif

#define u8son_MAX_DYN_ERROR 64

typedef enum u8son_type_t {
  u8son_above_root = 0, // to designate "parent" of root object
  u8son_object=1,
  u8son_array,
  u8son_string
} u8son_type_t;

typedef union u8son_key_t {
    unsigned char* skey; // key for object field
    int nkey; // index for array element
} u8son_key_t;

typedef struct u8son_current_item_t {
  //int level;
  u8son_key_t key; // string or number depending on parent_type
  u8son_type_t type; // string, object or array
  int cur_child_index; // for arrays only
  unsigned char* string_data; // only for u8son_string type
  void* user; // to associate the item with some user object
} u8son_current_item_t;

typedef struct u8son_path_t {
  int current_level;
  u8son_current_item_t path[u8son_MAX_LEVELS];
} u8son_path_t;

typedef struct u8son_error_t {
  int pos; // in source
  unsigned char* static_text;
  char var_text[u8son_MAX_DYN_ERROR];
} u8son_error_t;

typedef enum u8son_parse_event_type_t{
  u8son_error = -1,
  // zero means uninitialized event
  u8son_data = 1,
  u8son_enter_container,
  u8son_leave_container,
  u8son_end
} u8son_parse_event_type_t;

typedef struct u8son_parse_event_t{
  u8son_parse_event_type_t event;
  int current_level;
  u8son_current_item_t* path; // path[current_level] points to current element
  u8son_error_t* error;
} u8son_parse_event_t;


typedef struct u8son_parser_t{
  unsigned char* src;
  int src_len;
  int pos; // in src

  u8son_current_item_t path[u8son_MAX_LEVELS];

  u8son_parse_event_t cur_event;
  u8son_error_t error;
}u8son_parser_t;



#ifdef __cplusplus
extern "C" {
#endif
//................................................................. private functions

/*
static int
u8son_delim(u8son_parser_t* p, unsigned char* delim_set){
  unsigned char* cur = & p->src[p->pos];
  if(
      p->pos < p->src_len - 1
      && cur[0] == '\0'
      && strchr(delim_set, cur[1] != NULL)
  ){
    p->pos += 2;
    return cur[1];
  }else{
    p->error->static_text = "Expected delimeter prefixed with NUL character";
    strcpy(p->error->var_text, "Delimiter expected: ");
    strcat(p->error->var_text, delim_set);
    p->error->pos = p->pos;
    p->cur_event.event = u8son_error;
    return -1;
  }
}

static int // bool
u8son_need_space(u8son_parser_t* p, int n, unsigned char* static_err){
  if(p->pos > p->src_len - n){
    p->error.static_text = "Unexpected end of u8son data";
    p->error.pos = p->pos;
    p->cur_event.event = u8son_error;
    return 0;
  }else{
    return 1;
  }
}
*/

static u8son_current_item_t*
u8son_get_current(u8son_parser_t* p){
  int lev = p->cur_event.current_level;
  return &p->path[lev];
}

static u8son_current_item_t*
u8son_get_parent(u8son_parser_t* p){
  int lev = p->cur_event.current_level;
  if(lev == 0){
    return NULL;
  }else{
    return &p->path[lev - 1];
  }
}

static u8son_type_t
u8son_get_parent_type(u8son_parser_t* p){
  u8son_current_item_t* par =  u8son_get_parent(p);
  return (par == NULL)? u8son_above_root : par->type;
}

static u8son_parse_event_t*
u8son_parse_value(u8son_parser_t* p){
  u8son_current_item_t* im = u8son_get_current(p);
  u8son_type_t  ptyp = u8son_get_parent_type(p);
  char* expected_delims = ((ptyp == u8son_object)? ",}" : ",]");

  int pos = p->pos;
  if(u8sonneed_space(p, 2)) {
    return &p->cur_event;
  }


  if(p->src[pos] == '\0'){ // empty string or opening delimiter
    if(p->src[pos+1] == '\0'){ // empty string
      im->type = u8son_string;
      im->string_data = p->src + pos;
      p->cur_event.event = u8son_data;
      ++pos;
    }else{ // not an empty string; opening delimiter expected
      int odelim = u8son_delim(p, "{[");
      if(odelim < 0){ // syntax error
        return &p->cur_event;
      }

      if(odelim =='{'){
        im->type = u8son_object;
      }else{//  '['
        im->type = u8son_array;
      }

      p->cur_event.event = u8son_enter_container;
      return &p->cur_event.event;
    }
  }else{ // nonempty string;
    unsigned char sdata = p->src + pos;
    im->type = u8son_string;
    im->string_data = sdata;
    p->cur_event.event = u8son_data;

    p->pos += strlen(sdata);

    return &p->cur_event.event;
    /*

    { // delimiter expected (comma or closing)
      int delim = u8son_delim(p, expected_delims);
      if(delim < 0){ // syntax error
        return &p->cur_event;
      }
    }
    */
  }
}

//............................................................... interface functions:

static void
u8son_init(u8son_parser_t* p, unsigned char* src, int src_len){
  u8son_parser_t pa = {src, src_len};
  *p = pa;
  p->cur_event.path = p->path;
}

static u8son_parse_event_t*
u8son_parse_next(u8son_parser_t* p){
  if(p->cur_event.event == 0){ // if first time invocation
    if(p->src_len == 0){
      p->cur_event->event = u8son_end;
      return &p->cur_event;
    }

    int dm = u8son_delim( p, "{[");
    if(dm < 0){
      return &p->cur_event; // error
    }

    p->cur_event.event = u8son_enter_container;
    {
      u8son_current_item_t* im = u8son_get_current(p);
      im->type = (dm == '{' )? u8son_object=1 : u8son_array;
      p->pos += 2;
      return &p->cur_event;
    }
  }else{
    u8son_parse_event_type_t prev_ev = p->cur_event.event;

    switch(prev_ev){
      u8son_enter_container:
        ++(p->cur_event.current_level); // ToDo: check bounds !!! duck!!!!
        break;
      u8son_leave_container:
        // --(p->cur_event.current_level); // ToDo: check bounds !!! duck!!!!
        break;
      default:
    }

    {
      u8son_current_item_t* im = u8son_get_current(p);
      u8son_current_item_t*  pim =  u8son_get_parent(p);

      if(pim->type == u8son_object){
         char* key = p->src + p->pos;

         // duck!!! check bounds!!!
         p->pos += strlen(key) + ((key == '\0')? 1 : 0); // skip empty string if key is empty
         int colon =  u8son_delim( p, ":");
         if(colon < 0){
           return &p->cur_event; // error
         }

         im->key.skey = key;

      }else{ // array
           im->key.nkey = (pim->cur_child_index) ++;
      }

      // common for objects and arrays:

      return u8son_parse_value(p);
    }
  }


}


#ifdef __cplusplus
}
#endif

#endif // include guard
