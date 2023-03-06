#include "../u8son/u8son.h"

#include <stdio.h>
#include <stdlib.h>


void printerror(u8son_parser_t* p){
  printf("\n Pos: %d Error: %s %s", p->error.pos,  p->error.static_text,  p->error.var_text);
}

// char s[] = "\0{a\0:AAA\0,b\0:BBB\0}";
char s[] =
    "\0{"
      "a\0:AAA\0,"
      "obj\0:\0{x\0:123\0}\0,"
      "b\0:BBB\0,"
      "MyArray\0:\0[Aa\0,Bb\0,Cc\0,Dd\0]\0,"
      "MyEmptyArray\0:\0[\0]\0,"
      "my_empty_string\0:\0\0,"
      "MyArrayWithEmptyString\0:\0[\0\0]"
    "\0}"; //
int len = sizeof(s)-1;

typedef struct strbuf_t {
  char s[32];
}strbuf_t;

static strbuf_t
int2strbuf(int k){
  strbuf_t buf = {0};
  sprintf(buf.s, "%d", k);
  return buf;
}

char* print_item_value(u8son_current_item_t* im, int last_in_the_path){
  if(im->type == u8son_string){
    return im->string_data;
  }
  // else some container type^

  if(last_in_the_path) {
    switch(im->type){
    case u8son_object:
      return(im->parsing_status == u8son_enter_container)? " { " : " } ";
    case u8son_array:
      return (im->parsing_status == u8son_enter_container)? " [ " : " ] ";
    default:
      return "***print error***";
    }
  }else{
    return "   ";
  }
}


int main(void) {
  u8son_parser_t pa;
  u8son_init(&pa, s, len);

  for(int i=0; i< 100; ++i){
    int res = u8son_next(&pa);
    if(res <0){
      printerror(&pa);
      break;
    }

    printf("\n");

    if(res == u8son_end){
      printf("--eof--");
      break;
    }


    for(int i=0; i <= pa.current_level; ++i){
      u8son_current_item_t* im = pa.path + i;
      u8son_current_item_t* par = (i == 0)? NULL : ((pa.path + i - 1)); // parent
      printf("  %s : %s",
          //im->parsing_status,
          ((i == 0)? "root" : (par->type == u8son_object)? im->key.skey : int2strbuf(im->key.nkey).s),
          print_item_value(im, i == pa.current_level)
      );
    }
  }
  printf("\n");

  return 0;
}


/*
ToDo:

What to do with empty input?
functions to access current level, path elements and error struct
run a set of tests at once
stringify test result and compare with test source (test automation)
test error handling

test folder and compile guards...

 */
