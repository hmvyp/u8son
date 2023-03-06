/*
 *
 * u8son tokenizer
 */


typedef enum u8son_tok_type_t{
  u8son_tok_invalid = -1,
  u8son_tok_string=1,
  u8son_tok_delim,
  u8son_tok_eof,
}u8son_tok_type_t;

typedef struct u8son_tok_t{
  char* alldata;
  int limit;
  int next_pos;

  u8son_tok_type_t toktype;
  char delim; // if toktype is delim
  char* str; // if toktype is string

  char* static_errstring;

}u8son_tok_t;


static void
u8son_tok_init(u8son_tok_t* tok,   char* alldata, int limit){
  u8son_tok_t t = {alldata, limit};
  *tok = t;
}

static int
u8son_tok_error(u8son_tok_t* tok, char* err){
  tok->toktype = u8son_tok_invalid;
  tok->static_errstring = err;
  return tok->toktype;
}

static int
u8son_next_tok(u8son_tok_t* tok){

  if(tok->next_pos == tok->limit) {
    tok->toktype = u8son_tok_eof;
    return tok->toktype;
  }

  if(tok->next_pos > tok->limit - 2) {
    return u8son_tok_error(tok, "unexpected end of data (at least 2 characters expected here)");
    // (because valid source always ends with delimiter that occupies 2 bytes)
  }
  char c0 = tok->alldata[tok->next_pos];
  char c1 = tok->alldata[tok->next_pos +1];
  if(c0 == 0){
    if(c1 == 0){ // empty string case
      tok->toktype = u8son_tok_string;
      tok->str = tok->alldata + tok->next_pos++;
    }else{ // delimiter case
      tok->toktype = u8son_tok_delim;
      tok->delim = c1;
      tok->next_pos += 2;
    }
  }else{ // nonempty string case
    char* s = tok->alldata + tok->next_pos;
    if(memchr(s, 0, tok->limit - tok->next_pos) == NULL){ // try to find terminating 0
      return u8son_tok_error(tok, "String extends beyond the data buffer");
    }
    tok->toktype = u8son_tok_string;
    tok->str = s;
    tok->next_pos += strlen(s); // point to terminating '\0' (assuming buffer ends with '\0'!!!)
  }
  return tok->toktype;
}


typedef struct u8son_delim_as_string_t{
  char d[2];
}u8son_delim_as_string_t;

static u8son_delim_as_string_t
u8son_tok_delim_as_string(u8son_tok_t* tok){
  u8son_delim_as_string_t buf = {{tok->delim}};
  return buf;
}

