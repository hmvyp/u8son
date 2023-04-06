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

typedef enum u8son_parse_event_type_t{
  u8son_error = -1,
  // zero means uninitialized event
  u8son_data = 1,
  u8son_enter_container,
  u8son_leave_container,
  u8son_end
} u8son_parse_event_type_t;



typedef union u8son_key_t {
    const char* skey; // key for object field
    int nkey; // index for array element
} u8son_key_t;

typedef struct u8son_current_item_t {
  u8son_key_t key; // string or number depending on parent_type
  u8son_type_t type; // string, object or array
  int cur_child_index;
  const char* string_data; // only for u8son_string type

  u8son_parse_event_type_t parsing_status;
  void* user; // to associate the item with some user object
} u8son_current_item_t;

typedef struct u8son_path_t {
  int current_level;
  u8son_current_item_t path[u8son_MAX_LEVELS];
} u8son_path_t;

typedef struct u8son_error_t {
  int pos; // in source
  const char* static_text;
  char var_text[u8son_MAX_DYN_ERROR];
} u8son_error_t;

typedef struct u8son_parser_t{
  u8son_tok_t tok;

  int current_level;
  u8son_current_item_t path[u8son_MAX_LEVELS];

  u8son_error_t error;
}u8son_parser_t;



#ifdef __cplusplus
extern "C" {
#endif
//................................................................. private functions

static void
u8son_clear_item_except_key(u8son_current_item_t* it){
  u8son_current_item_t tmp = {};
  tmp.key = it->key;
  *it = tmp;
}


static u8son_current_item_t*
u8son_get_current(u8son_parser_t* p){
  int lev = p->current_level;
  return &p->path[lev];
}

static u8son_current_item_t*
u8son_get_parent(u8son_parser_t* p){
  int lev = p->current_level;
  if(lev == 0){
    return NULL;
  }else{
    return &p->path[lev - 1];
  }
}


static int u8son_reterror(u8son_parser_t* p, const char* static_errstr, const char* var_errstr){
  p->error.static_text = static_errstr;
  if(strlen(var_errstr) < sizeof(p->error.var_text)){
    strcpy(p->error.var_text, var_errstr);
  }
  p->error.pos = p->tok.next_pos;
  u8son_get_current(p)->parsing_status = u8son_error;
  // printf("\n Pos: %d Err: %s %s", p->tok.next_pos, static_errstr, var_errstr);
  return u8son_error;
}


static int u8son_rettokerror(u8son_parser_t* p){
  return u8son_reterror(p, p->tok.static_errstring, "");
}


static int
u8son_parse_value(u8son_parser_t* p){
  u8son_tok_t* tok = & p->tok;

  u8son_current_item_t* im = u8son_get_current(p);

  u8son_clear_item_except_key(im);

  if(tok->toktype == u8son_tok_delim){
    int odelim = tok->delim;
    if(odelim =='{'){
      im->type = u8son_object;
    }else if(odelim =='['){//  '['
      im->type = u8son_array;
    }else{
      return u8son_reterror(p, "Invalid delimiter: { or [ expected", "");
    }

    im->parsing_status = u8son_enter_container;
    im->cur_child_index = 0;
  }else{ //string
    im->parsing_status = u8son_data;
    im->type = u8son_string;
    im->string_data = tok->str;
  }

  if(u8son_next_tok(tok) < 0){ // read ahead (expecting delimiter after value)
    return u8son_rettokerror(p);
  };

  return im->parsing_status;

}

static int
u8son_parse_next(u8son_parser_t* p){
  u8son_tok_t* tok = & p->tok;

  u8son_current_item_t* im = u8son_get_current(p);
  u8son_current_item_t* par = u8son_get_parent(p);

  if(im->parsing_status == u8son_leave_container && p->current_level == 0){
    im->parsing_status = u8son_end;
    return im->parsing_status;
  }

  switch(im->parsing_status){
  case u8son_enter_container:
    if(p->current_level >= u8son_MAX_LEVELS - 1){
      return u8son_reterror(p, "Maximum object depth exceeded", "");
    }

    ++(p->current_level);
    im = u8son_get_current(p); // new parent
    par = u8son_get_parent(p);
    // no break, pass through:
  case u8son_data:
  case u8son_leave_container:
  {
      if(tok->toktype == u8son_tok_delim){ // then should be comma or closing bracket
        const char d = tok->delim;
        if(d == ','){
          // nothing to do, proceed with next element of array or object
          if(u8son_next_tok(tok) < 1){ // read ahead
            return u8son_rettokerror(p);
          }
        }else if((d == '}' && par->type == u8son_object) || (d == ']' && par->type == u8son_array)){
          // if closing bracket encountered...
          if(p->current_level <= 0){
            return u8son_reterror(p, "Internal parser error (jumping above root)", "");
          }
          --(p->current_level);
          im = u8son_get_current(p);
          im->parsing_status = u8son_leave_container;

          if(u8son_next_tok(tok) <1){ // read ahead (note, u8son_tok_eof is not an error)
            return u8son_rettokerror(p);
          }
          return im->parsing_status; // == u8son_leave_container
        }else{
          return u8son_reterror(p, "Invalid delimiter found: ", u8son_tok_delim_as_string(tok).d);
        }

      }else if(par->cur_child_index != 0 ){
        return u8son_reterror(p, "Delimiter expected ", "");
      }

      if(par->type == u8son_object) { // if parent is an object then parse the key
        if(tok->toktype != u8son_tok_string){

          return u8son_reterror(p, "Key string expected. Found delimiter: ", u8son_tok_delim_as_string(&p->tok).d);
        }

        im->key.skey = tok->str; // store the key in the element structure

        if(!(u8son_next_tok(tok) > 0 && tok->toktype == u8son_tok_delim  && tok->delim == ':')){
          return u8son_reterror(p, ": expected here", "");
        }
        //
        if(u8son_next_tok(tok) <1){ // read ahead (value)
          return u8son_rettokerror(p);
        }

      }else{ // if parent is array
          im->key.nkey = par->cur_child_index; // just store the index in the element structure
      }

      if(par != NULL){
        ++(par->cur_child_index);
      }

      return u8son_parse_value(p); // parse object or array element value
  }
  case u8son_end:
      return u8son_end;
  case u8son_error:
      return u8son_error;
 // default:
    //return u8son_end;


  }

  return -1000; //impossible? (calm gcc)
}

//............................................................... interface functions:

static void
u8son_init(u8son_parser_t* p, const char* src, int src_limit){
  u8son_parser_t pa = {};
  u8son_tok_init( &pa.tok, src, src_limit);
  *p = pa;

}

static int
u8son_next(u8son_parser_t* p){
  if(p->current_level == 0 && u8son_get_current(p)->parsing_status == 0){
    // if first call, try to parse root object
    int tres = u8son_next_tok(&p->tok); // one token shall be always read ahead
    if(tres < 0){
      return u8son_rettokerror(p);
    }else if(p->tok.toktype == u8son_tok_eof){
      return u8son_end;
    }else if(p->tok.toktype != u8son_tok_delim){  // the root must be object or array (not string)
      return u8son_reterror(p, "Delimiter { or [ expected ", "");
    }
    return u8son_parse_value(p); // start parsing root object
  }else{
    return u8son_parse_next(p);
  }
}


#ifdef __cplusplus
}
#endif

#endif // include guard
