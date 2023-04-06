#include "../u8son/u8son.h"

#include <stdio.h>
#include <stdlib.h>


static inline void
printerror(u8son_parser_t* p){
  printf("\n Pos: %d Error: %s %s", p->error.pos,  p->error.static_text,  p->error.var_text);
}

typedef struct test_t{
  const char* test_name;
  const char* input;
  int len;
}test_t;

const char COMPLEX[] =
    "\0{"
      "a\0:AAA\0,"
      "obj\0:\0{x\0:123\0}\0,"
      "b\0:BBB\0,"
      "MyArray\0:\0[Aa\0,Bb\0,Cc\0,Dd\0]\0,"
      "MyEmptyArray\0:\0[\0]\0,"
      "my_empty_string\0:\0\0,"
      "MyArrayWithEmptyString\0:\0[\0\0]"
    "\0}"; //

const char EMPTY_ARRAY[] = {"\0[\0]"};
const char EMPTY_OBJECT[] = {"\0{\0}"};
const char EMPTY_INPUT[] = {""};

#define ONE_TEST(testname) {#testname, testname, sizeof(testname) - 1}

test_t all_tests[] = {
    ONE_TEST(COMPLEX),
    ONE_TEST(EMPTY_ARRAY),
    ONE_TEST(EMPTY_OBJECT),
    ONE_TEST(EMPTY_INPUT)
};


typedef struct strbuf_t {
  char s[32];
}strbuf_t;

static inline strbuf_t
int2strbuf(int k){
  strbuf_t buf = {0};
  sprintf(buf.s, "%d", k);
  return buf;
}


static inline const char*
print_item_value(u8son_current_item_t* im, int last_in_the_path){
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


static inline int
test_one(const char* test_name, const char* s, int len) {
  u8son_parser_t pa;
  u8son_init(&pa, s, len);

  for(int i=0; i< 100; ++i){
    int res = u8son_next(&pa);
    if(res < 0){
      printerror(&pa);
      return -1;
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


static inline int
test_many(test_t* all_tests, int ntests){
  int i;
  int some_err = 0;
  for(i = 0; i< ntests; ++i){
    test_t* ts = &all_tests[i];
    printf("\n --------- Starting test %s  input length: %d -------", ts->test_name, ts->len);
    int res = test_one(ts->test_name, ts->input, ts->len);
    some_err = some_err || !!res;
    if(res != 0){
      printf("\n test %s FAILED \n", ts->test_name);
      // return res;
    }
    printf("\n test %s passed \n", ts->test_name);
  }

  if(some_err){
    printf("\n *** Some tests FAILED ***\n");
  }else{
    printf("\n ... all tests passed\n");
  }

  return 0;
}


static inline int
test_all(){
  return test_many(all_tests, sizeof(all_tests)/sizeof(all_tests[0]));
}

/*
What to do with empty input? Ok, (all zeroes in the level struct)

ToDo:

functions to access current level, path elements and error struct
stringify test result and compare with test source (test automation)
test folder and compile guards...

 */
