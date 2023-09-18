#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10
#define bool int
#define true 1
#define false 0

bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t';
}

bool is_quote(char c) {
  return c == '"';
}

bool is_number(char c) {
  return c >= '0' && c <= '9' || c == '-';
}

typedef struct range {
  size_t length;
  size_t i;
} range;

typedef struct string_literal {
  char* value;
  int start;
  int end;
} string_literal;

typedef enum token_type {
  OPEN_BRACE = 1,
  CLOSE_BRACE = 2,
  OPEN_BRACKET = 3,
  CLOSE_BRACKET = 4,
  COLON = 5,
  COMMA = 6,
  INTEGER_LITERAL = 7,
  FLOAT_LITERAL = 8,
  STRING_LITERAL = 9,
  NULL_LITERAL = 10
} token_type;

typedef struct token_ref {
  token_type type;
  union value {
    string_literal* string;
    int number;
    double number_f;
  } value;
  int start;
  int end;
} token;

char* token_type_to_string(token_type t) {
  switch (t) {
    case OPEN_BRACE:
      return "{";
    case CLOSE_BRACE:
      return "}";
    case OPEN_BRACKET:
      return "["; 
    case CLOSE_BRACKET:
      return "]";
    case COLON:
      return ":";
    case COMMA:
      return ",";
    case INTEGER_LITERAL:
      return "NUMBER";
    case STRING_LITERAL:
      return "STRING";
    default:
      return "UNKNOWN";
  }
}

void advance(range* r, size_t amount) {
  r->i += amount;
}

range new_range(size_t length) {
  return (range){.length = length, .i = 0};
}

void assert_token(token t, token_type tt) {
  if (t.type != tt) {
    printf("Expected token type %s, got %s\n", token_type_to_string(tt), token_type_to_string(t.type));
    exit(1);
  }
}


struct json;

typedef struct json json;

typedef struct json_string {
  token* token;
  char* value;
} json_string;

typedef struct json_number {
  token* token;
  union {
    int int_;
    double float_;
  } value;
} json_number;

typedef struct json_null {
  token* token;
} json_null;

typedef struct json_kv_pair {
  json_string* key;
  json* value;
} json_kv_pair;

typedef struct json_object {
  json_kv_pair* pairs;
  size_t length;
} json_object;

typedef struct json_array {
  json** values;
  size_t length;
} json_array;

typedef enum json_tag {
  OBJECT,
  ARRAY,
  STRING,
  INT,
  FLOAT,
  NULL_,
} json_tag;

typedef struct json {
  json_tag tag;
  union {
    json_object* object;
    json_array* array;
    json_string* string;
    json_number* number;
    json_null* null_;
  } value;
} json;


void add_index(token* t, size_t index) {
  t->start = index;
  t->end = index + 1;
}

token* make_token(token_type type, range* r) {
  token* t = malloc(sizeof(struct token_ref));
  t->type = type;
  add_index(t, r->i);
  return t;
}

token* gobble_text(char* stream, range* r) {
  // gobble the quote
  advance(r, 1);

  size_t start = r->i;
  while (stream[r->i] != '\0' && !is_quote(stream[r->i])) {
    advance(r, 1);
  }
  size_t end = r->i;
  // gobble the quote
  advance(r, 1);

  token* t = make_token(STRING_LITERAL, r);
  string_literal* s = malloc(sizeof(string_literal));
  s->value = malloc(sizeof(char) * (end - start));
  memcpy(s->value, stream + start, end - start);
  t->value.string = s;
  return t;
}

token* gobble_number(char* stream, range* r) {
  size_t start = r->i;
  bool is_integer = true;
  while (stream[r->i] != '\0' && is_number(stream[r->i])) {
    advance(r, 1);
  } 
  if (stream[r->i] == '.') {
    is_integer = false;
    advance(r, 1);
    while (stream[r->i] != '\0' && is_number(stream[r->i])) {
      advance(r, 1);
    }
  }
  char* buffer = malloc(sizeof(char) * (r->i - start));
  memcpy(buffer, stream + start, r->i - start);
  buffer[r->i - start] = '\0';

  token* t;

  if (is_integer) {
    t = make_token(INTEGER_LITERAL, r);
    t->value.number = atoi(buffer);
  } else {
    t = make_token(FLOAT_LITERAL, r);
    t->value.number_f = atof(buffer);
  }
  return t;
}


void lex(char* stream, token** tokens, size_t* token_index) {
  size_t length = strlen(stream);
  range r = new_range(length);
  while (r.i < length) {
    char c = stream[r.i];
    if (is_whitespace(c)) { 
      advance(&r, 1);
    } else if (c == '{') {
      token* t = make_token(OPEN_BRACE, &r);
      tokens[(*token_index)++] = t;
      advance(&r, 1);
    } else if (c == '}') {
      token* t = make_token(CLOSE_BRACE, &r);
      tokens[(*token_index)++] = t;
      advance(&r, 1);
    } else if (c == '[') {
      token* t = make_token(OPEN_BRACKET, &r);
      tokens[(*token_index)++] = t;
      advance(&r, 1);
    } else if (c == ']') {
      token* t = make_token(CLOSE_BRACKET, &r);
      tokens[(*token_index)++] = t;
      advance(&r, 1);
    } else if (c == ':') {
      token* t = make_token(COLON, &r);
      tokens[(*token_index)++] = t;
      advance(&r, 1);
    } else if (c == ',') {
      token* t = make_token(COMMA, &r);;
      tokens[(*token_index)++] = t;
      advance(&r, 1);
    } else if (is_quote(c)) {
      token* t = gobble_text(stream, &r);
      tokens[(*token_index)++] = t;
    } else if (is_number(c)) {
      token* t = gobble_number(stream, &r);
      tokens[(*token_index)++] = t;
    } else if (c == 'n') {
      token* t = make_token(NULL_LITERAL, &r);
      tokens[(*token_index)++] = t;
      advance(&r, 4);
    } else {
      printf("Unknown character: %c\n", c);
      advance(&r, 1);
    }
  }
}

char* read_file(char* filename) { 
  char* source = NULL;
  FILE *fp = fopen(filename, "r");
  if (fp != NULL) {
      /* Go to the end of the file. */
      if (fseek(fp, 0L, SEEK_END) == 0) {
          /* Get the size of the file. */
          long bufsize = ftell(fp);
          if (bufsize == -1) { /* Error */ }

          /* Allocate our buffer to that size. */
          source = malloc(sizeof(char) * (bufsize + 1));

          /* Go back to the start of the file. */
          if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

          /* Read the entire file into memory. */
          size_t newLen = fread(source, sizeof(char), bufsize, fp);
          if ( ferror( fp ) != 0 ) {
              fputs("Error reading file", stderr);
          } else {
              source[newLen++] = '\0'; /* Just to be safe. */
          }
      }
      fclose(fp);
  }
  return source;
}


json* parse_json(token** tokens, range* r);

json_string* parse_string(token* token, range* r) {
  json_string* string = malloc(sizeof(json_string));
  string->token = token;
  string->value = token->value.string->value;
  advance(r, 1);
  return string;
}

json_number* parse_number(token* token, range* r) {
  json_number* number = malloc(sizeof(json_number));
  number->token = token;
  if (token->type == INTEGER_LITERAL) {
    number->value.int_ = token->value.number;
  } else {
    number->value.float_ = token->value.number_f;
  }
  advance(r, 1);
  return number;
}

json_null* parse_null(token* token, range* r) {
  json_null* null_ = malloc(sizeof(json_null));
  null_->token = token;
  advance(r, 1);
  return null_;
}


json_kv_pair parse_object_kv(token** tokens, range* r) {
  json_kv_pair pair;
  
  pair.key = parse_string(tokens[r->i], r);
  // parsing :
  assert_token(*tokens[r->i], COLON);
  advance(r, 1);

  pair.value = parse_json(tokens, r);
  return pair;
}

json_array* parse_array(token** tokens, range* r) {
  json_array* array = malloc(sizeof(json_array));
  array->values = malloc(sizeof(json*) * 100);
  array->length = 0;

  assert_token(*tokens[r->i], OPEN_BRACKET);
  advance(r, 1);
  for (;;) {
    json* value = parse_json(tokens, r);
    array->values[array->length++] = value;
    // allowing trailing commas
    if (tokens[r->i]->type == COMMA) {
      advance(r, 1);
    }
    
    if (tokens[r->i]->type == CLOSE_BRACKET) {
      advance(r, 1);
      break;
    }
  }

  return array;
}

json_object* parse_object(token** tokens, range* r) {
  json_object* object = malloc(sizeof(json_object));
  object->length = 0;
  // TODO: use a list instead
  object->pairs = malloc(sizeof(json_kv_pair) * 100);
  assert_token(*tokens[r->i], OPEN_BRACE);
  advance(r, 1);
  for (;;)  {
    json_kv_pair pair = parse_object_kv(tokens, r);
    object->pairs[object->length++] = pair;
    // allowing trailing commas
    if (tokens[r->i]->type == COMMA) {
      advance(r, 1);
    }
    
    if (tokens[r->i]->type == CLOSE_BRACE) {
      advance(r, 1);
      break;
    }
  }
  return object;
}

json* parse_json(token** tokens, range* r) {
  token* token = tokens[r->i];
  token_type type = tokens[r->i]->type;
  if (type == OPEN_BRACE) {
    json* j = malloc(sizeof(json));
    j->tag = OBJECT;
    j->value.object = parse_object(tokens, r);
    return j;
  } else if (type == OPEN_BRACKET) {
    json* j = malloc(sizeof(json));
    j->value.array = parse_array(tokens, r);
    j->tag = ARRAY;
    return j;
  } else if (type == STRING_LITERAL) {
    json_string* str = parse_string(token, r);
    json* j = malloc(sizeof(json));
    j->tag = STRING;
    j->value.string = str;
    return j;
  } else if (type == INTEGER_LITERAL || type == FLOAT_LITERAL) {
    json_number* number = parse_number(token, r);
    json* j = malloc(sizeof(json));
    if (type == INTEGER_LITERAL) {
      j->tag = INT;
    } else {
      j->tag = FLOAT;
    }
    j->value.number = number;
    return j;
  } else if (type == NULL_LITERAL) {
    json_null* null_ = parse_null(token, r);
    json* j = malloc(sizeof(json));
    j->tag = NULL_;
    j->value.null_ = null_;
    return j;
  } else {
    printf("Unknown token type: %s\n", token_type_to_string(type));
    return NULL;
  }
}

void pretty_print_json(json j, int indent, int indentation_size) {
  if (j.tag == OBJECT) {
    printf("{\n");
    for (size_t i = 0; i < j.value.object->length; i++) {
      json_kv_pair pair = j.value.object->pairs[i];
      printf("%*s", indent, "");
      printf("  \"%s\": ", pair.key->value);
      pretty_print_json(*pair.value, indent + indentation_size, indentation_size);
      if (i != j.value.object->length - 1) {
        printf(",");
      }
      printf("\n");
    }
    printf("%*s}", indent, "");
  } else if (j.tag == ARRAY) {
    printf("[\n");
    for (size_t i = 0; i < j.value.array->length; i++) {
      json value = *j.value.array->values[i];
      printf("%*s", indent + 2, "");
      pretty_print_json(value, indent + indentation_size, indentation_size);
      if (i != j.value.array->length - 1) {
        printf(",");
      }
      printf("\n");
    }
    printf("%*s]", indent, "");
  } else if (j.tag == STRING) {
    printf("\"%s\"", j.value.string->value);
  } else if (j.tag == INT) {
    printf("%d", j.value.number->value.int_);
  } else if (j.tag == FLOAT) {
    printf("%f", j.value.number->value.float_);
  } else if (j.tag == NULL_) {
    printf("null");
  }
}

void pprint(json j, int indentation_size) {
  pretty_print_json(j, 0, indentation_size);
}

const int INDENTATION = 2;

int main() {
  char* buffer = read_file("./test.json");

  token** tokens = malloc(100 * sizeof(struct token_ref*));
  size_t token_index = 0;

  lex(buffer, tokens, &token_index);

  range r = new_range(token_index);
  json* j = parse_json(tokens, &r);
  
  pprint(*j, INDENTATION);
}
